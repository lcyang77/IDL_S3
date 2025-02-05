/************************************************
 * File: net_sta.c (modified)
 ************************************************/
#include "net_sta.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "net_uart_comm.h"
#include "get_time.h"
#include "cc_event.h"    // 事件机制头文件
#include "gs_wifi.h"     // GS_WIFI_EVENT 定义在此文件中
#include "gs_mqtt.h"     // GS_MQTT_EVENT 定义在此文件中

static const char *TAG = "net_sta";

// 当前联网状态
static net_status_t current_status = NET_STATUS_NOT_CONFIGURED;

// 备份定时器（仅作为事件机制的补充）
static TimerHandle_t timer_5s = NULL;
static TimerHandle_t timer_12s = NULL;

/**
 * @brief 5秒备份定时器回调函数
 *
 * 如果在5秒内没有收到Wi-Fi相关事件达到“已连接路由器”状态，则打印日志。
 */
static void timer_5s_callback(TimerHandle_t xTimer)
{
    if (current_status < NET_STATUS_CONNECTED_ROUTER) {
        ESP_LOGE(TAG, "Backup: Network did not reach CONNECTED_ROUTER within 5 seconds. Current status: 0x%02X", current_status);
    }
}

/**
 * @brief 12秒备份定时器回调函数
 *
 * 如果在12秒内没有收到MQTT相关事件达到“已连接云服务器”状态，则打印日志。
 */
static void timer_12s_callback(TimerHandle_t xTimer)
{
    if (current_status < NET_STATUS_CONNECTED_SERVER) {
        ESP_LOGE(TAG, "Backup: Network did not reach CONNECTED_SERVER within 12 seconds. Current status: 0x%02X", current_status);
    }
}

/**
 * @brief 联网状态事件处理回调函数
 *
 * 监听来自GS_WIFI_EVENT和GS_MQTT_EVENT的事件，并根据事件类型调用net_sta_update_status()更新状态。
 */
static void net_sta_event_handler(void *handler_args, cc_event_base_t base_event, int32_t id, void *event_data)
{
    if (base_event == GS_WIFI_EVENT) {
        switch (id) {
            case GS_WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Event: Wi-Fi connected (STA_CONNECTED)");
                // Wi-Fi已连接，进入“正在连接路由器”状态
                net_sta_update_status(NET_STATUS_CONNECTING_ROUTER);
                break;
            case GS_WIFI_EVENT_STA_GOT_IP:
                ESP_LOGI(TAG, "Event: Wi-Fi got IP (STA_GOT_IP)");
                net_sta_update_status(NET_STATUS_CONNECTED_ROUTER);
                break;
            case GS_WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Event: Wi-Fi disconnected (STA_DISCONNECTED)");
                net_sta_update_status(NET_STATUS_NOT_CONFIGURED);
                break;
            default:
                break;
        }
    } else if (base_event == GS_MQTT_EVENT) {
        switch (id) {
            case GS_MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "Event: MQTT connected (MQTT_EVENT_CONNECTED)");
                net_sta_update_status(NET_STATUS_CONNECTED_SERVER);
                break;
            case GS_MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "Event: MQTT disconnected (MQTT_EVENT_DISCONNECTED)");
                // 当MQTT断开后，如果Wi-Fi依然正常，则回退到已连接路由器状态
                if (current_status >= NET_STATUS_CONNECTED_ROUTER) {
                    net_sta_update_status(NET_STATUS_CONNECTED_ROUTER);
                }
                break;
            default:
                break;
        }
    }
}

/**
 * @brief 初始化联网状态管理模块
 *
 * 注册来自Wi-Fi和MQTT模块的事件回调。
 */
esp_err_t net_sta_init(void)
{
    current_status = NET_STATUS_NOT_CONFIGURED;
    ESP_LOGI(TAG, "Network STA initialized with status: 0x%02X", current_status);

    // 注册事件处理回调
    cc_event_register_handler(GS_WIFI_EVENT, net_sta_event_handler);
    cc_event_register_handler(GS_MQTT_EVENT, net_sta_event_handler);

    return ESP_OK;
}

/**
 * @brief 获取当前联网状态
 */
net_status_t net_sta_get_status(void)
{
    return current_status;
}

/**
 * @brief 更新联网状态并发送通知
 *
 * 当状态变化时，同时取消相关备份定时器，并构造UART数据包发送网络状态通知。
 */
esp_err_t net_sta_update_status(net_status_t status)
{
    if (status != current_status) {
        current_status = status;
        ESP_LOGI(TAG, "Network status updated to: 0x%02X", current_status);

        // 当状态达到“已连接路由器”时，取消备份定时器timer_5s
        if (current_status >= NET_STATUS_CONNECTED_ROUTER && timer_5s != NULL) {
            if (xTimerStop(timer_5s, 0) == pdPASS) {
                ESP_LOGI(TAG, "Backup timer_5s stopped due to status update to 0x%02X", current_status);
            }
            if (xTimerDelete(timer_5s, 0) == pdPASS) {
                ESP_LOGI(TAG, "Backup timer_5s deleted");
            }
            timer_5s = NULL;
        }

        // 当状态达到“已连接云服务器”时，取消备份定时器timer_12s
        if (current_status >= NET_STATUS_CONNECTED_SERVER && timer_12s != NULL) {
            if (xTimerStop(timer_12s, 0) == pdPASS) {
                ESP_LOGI(TAG, "Backup timer_12s stopped due to status update to 0x%02X", current_status);
            }
            if (xTimerDelete(timer_12s, 0) == pdPASS) {
                ESP_LOGI(TAG, "Backup timer_12s deleted");
            }
            timer_12s = NULL;
        }

        // 构造并发送0x23网络状态通知数据包
        uart_packet_t packet = {
            .header = {0xAA, 0x55},
            .command = CMD_NETWORK_STATUS,
            .data = {0},
        };

        uint32_t utc_time = 0;
        int8_t timezone = 0;
        if (current_status == NET_STATUS_CONNECTED_SERVER) {
            utc_time = get_time_get_utc();
            timezone = get_time_get_timezone();
        }

        packet.data[0] = (uint8_t)current_status;
        packet.data[1] = (uint8_t)(utc_time & 0xFF);
        packet.data[2] = (uint8_t)((utc_time >> 8) & 0xFF);
        packet.data[3] = (uint8_t)((utc_time >> 16) & 0xFF);
        packet.data[4] = (uint8_t)((utc_time >> 24) & 0xFF);
        packet.data[5] = (uint8_t)timezone;

        packet.checksum = uart_comm_calc_checksum((uint8_t *)&packet, sizeof(uart_packet_t) - 1);

        esp_err_t ret = uart_comm_send_packet(&packet);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Sent network status notification.");
        } else {
            ESP_LOGE(TAG, "Failed to send network status notification.");
        }
    }
    return ESP_OK;
}

/**
 * @brief 启动联网状态监控（备份定时器）
 *
 * 如果实际事件因某种原因未能及时更新状态，则定时器可触发超时日志。
 */
void net_sta_start_monitor(void)
{
    // 创建5秒备份定时器
    if (timer_5s == NULL) {
        timer_5s = xTimerCreate("timer_5s", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, timer_5s_callback);
        if (timer_5s != NULL) {
            if (xTimerStart(timer_5s, 0) != pdPASS) {
                ESP_LOGE(TAG, "Failed to start backup timer_5s");
            } else {
                ESP_LOGI(TAG, "Backup timer_5s started");
            }
        } else {
            ESP_LOGE(TAG, "Failed to create backup timer_5s");
        }
    }

    // 创建12秒备份定时器
    if (timer_12s == NULL) {
        timer_12s = xTimerCreate("timer_12s", pdMS_TO_TICKS(12000), pdFALSE, (void *)0, timer_12s_callback);
        if (timer_12s != NULL) {
            if (xTimerStart(timer_12s, 0) != pdPASS) {
                ESP_LOGE(TAG, "Failed to start backup timer_12s");
            } else {
                ESP_LOGI(TAG, "Backup timer_12s started");
            }
        } else {
            ESP_LOGE(TAG, "Failed to create backup timer_12s");
        }
    }
}
