/************************************************
 * File: net_sta.c
 ************************************************/
#include "net_sta.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "net_uart_comm.h"
#include "get_time.h"

static const char *TAG = "net_sta";

// 当前联网状态
static net_status_t current_status = NET_STATUS_NOT_CONFIGURED;

// 定时器句柄
static TimerHandle_t timer_5s = NULL;
static TimerHandle_t timer_12s = NULL;

// 定时器回调函数
static void timer_5s_callback(TimerHandle_t xTimer)
{
    if (current_status < NET_STATUS_CONNECTED_ROUTER) {
        ESP_LOGE(TAG, "Network connection failed after 5 seconds.");
        // 根据需求进行断电或其他处理
        // 示例：断电逻辑需根据具体硬件实现
    }
}

static void timer_12s_callback(TimerHandle_t xTimer)
{
    if (current_status < NET_STATUS_CONNECTED_SERVER) {
        ESP_LOGE(TAG, "Network connection failed after 12 seconds.");
        // 根据需求进行断电或其他处理
        // 示例：断电逻辑需根据具体硬件实现
    }
}

// 初始化联网状态管理
esp_err_t net_sta_init(void)
{
    current_status = NET_STATUS_NOT_CONFIGURED;
    ESP_LOGI(TAG, "Network STA initialized with status: 0x%02X", current_status);
    return ESP_OK;
}

// 获取当前联网状态
net_status_t net_sta_get_status(void)
{
    return current_status;
}

// 更新联网状态并发送通知
esp_err_t net_sta_update_status(net_status_t status)
{
    if (status != current_status) {
        current_status = status;
        ESP_LOGI(TAG, "Network status updated to: 0x%02X", current_status);

        // 获取UTC时间和时区
        uint32_t utc_time = get_time_get_utc();
        int8_t timezone = get_time_get_timezone();

        // 构建并发送0x23数据包
        uart_packet_t packet = {
            .header = {0xAA, 0x55},
            .command = CMD_NETWORK_STATUS,
            .data = {0},
        };

        packet.data[0] = (uint8_t)current_status;

        // 填充UTC时间（小端）
        packet.data[1] = (uint8_t)(utc_time & 0xFF);
        packet.data[2] = (uint8_t)((utc_time >> 8) & 0xFF);
        packet.data[3] = (uint8_t)((utc_time >> 16) & 0xFF);
        packet.data[4] = (uint8_t)((utc_time >> 24) & 0xFF);

        // 填充时区值
        packet.data[5] = (uint8_t)timezone;

        // 计算校验和
        packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);

        // 发送数据包
        esp_err_t ret = uart_comm_send_packet(&packet);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Sent network status notification.");
        } else {
            ESP_LOGE(TAG, "Failed to send network status notification.");
        }
    }
    return ESP_OK;
}

// 启动联网状态监控
void net_sta_start_monitor(void)
{
    // 创建5秒定时器
    timer_5s = xTimerCreate("timer_5s", pdMS_TO_TICKS(5000), pdFALSE, (void*)0, timer_5s_callback);
    if (timer_5s != NULL) {
        if (xTimerStart(timer_5s, 0) != pdPASS) {
            ESP_LOGE(TAG, "Failed to start timer_5s");
        } else {
            ESP_LOGI(TAG, "timer_5s started");
        }
    } else {
        ESP_LOGE(TAG, "Failed to create timer_5s");
    }

    // 创建12秒定时器
    timer_12s = xTimerCreate("timer_12s", pdMS_TO_TICKS(12000), pdFALSE, (void*)0, timer_12s_callback);
    if (timer_12s != NULL) {
        if (xTimerStart(timer_12s, 0) != pdPASS) {
            ESP_LOGE(TAG, "Failed to start timer_12s");
        } else {
            ESP_LOGI(TAG, "timer_12s started");
        }
    } else {
        ESP_LOGE(TAG, "Failed to create timer_12s");
    }
}
