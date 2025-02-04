/************************************************
 * File: main.c
 ************************************************/
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

// ESP-IDF / FreeRTOS 相关头文件
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

// 项目相关头文件
#include "cc_log.h"
#include "cc_hal_sys.h"
#include "cc_hal_kvs.h"
#include "cc_hal_wifi.h"
#include "cc_event.h"
#include "cc_timer.h"
#include "cc_tmr_task.h"
#include "cc_http.h"
#include "gs_main.h"
#include "product.h"
#include "gs_mqtt.h"
#include "img_upload.h"
#include "gs_bind.h"
#include "gs_device.h"
#include "uvc_camera.h"
#include "gs_wifi.h"

// UART 通信头文件
#include "net_uart_comm.h"

// 时间获取模块头文件
#include "get_time.h"

// 联网状态管理模块头文件
#include "net_sta.h"

// UART配置模块头文件
#include "uart_config.h"

// msg_upload 模块头文件
#include "msg_upload.h"

// unlock 模块头文件
#include "unlock.h"

// img_transfer 模块头文件
#include "img_transfer.h"

// 新增：状态上传模块头文件
#include "state_report.h"

static const char *TAG = "app_main";

// 判断 MQTT “出生消息”是否发送成功
static bool s_version_msg_ok = false;
static bool s_rssi_msg_ok    = false;

// 固件版本定义
#define FIRMWARE_VERSION_MAJOR 9
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0

/**
 * @brief 网络循环任务：跑项目事件循环、定时器等
 */
static void network_task(void *arg)
{
    uint64_t last = cc_hal_sys_get_ms();
    while (1) {
        uint64_t now = cc_hal_sys_get_ms();
        uint16_t ms = now - last;
        last = now;

        cc_event_run();
        cc_timer_run(CC_TIMMER_MS(ms));
        cc_http_run(ms);
        cc_tmr_task_run(ms);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief 当两条 MQTT 出生消息都成功后，将在这里启动一次异步任务
 */
static void mqtt_done_task(void *arg)
{
    ESP_LOGI(TAG, "MQTT birth messages done. Starting time update...");

    // 1) 启动时间更新
    esp_err_t err = get_time_start_update(); 
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start time update (err=0x%x)", err);
    }

    // 2) 阻塞等待(最多等5秒)，看看是否成功
    bool success = get_time_wait_done(5000);
    if (success) {
        ESP_LOGI(TAG, "Time update succeeded, valid UTC/timezone now.");
    } else {
        ESP_LOGW(TAG, "Time update failed or timed out, using old(0) value...");
    }

    // 3) 无论成功/失败，这里都认为已经连接云服务器 => 发送 0x23(0x04) 到 MCU
    net_sta_update_status(NET_STATUS_CONNECTED_SERVER);

    // 4) 启动摄像头
    uvc_camera_start();

    // 任务结束
    vTaskDelete(NULL);
}

/**
 * @brief MQTT 出生消息回调
 */
static void birth_msg_callback(uint8_t msg_type, uint8_t status)
{
    if (msg_type == 0) { // 版本消息
        if (status) {
            ESP_LOGI(TAG, "Version message sent successfully");
            s_version_msg_ok = true;
        } else {
            ESP_LOGE(TAG, "Version message send failed");
        }
    } else if (msg_type == 1) { // RSSI 消息
        if (status) {
            ESP_LOGI(TAG, "RSSI message sent successfully");
            s_rssi_msg_ok = true;
        } else {
            ESP_LOGE(TAG, "RSSI message send failed");
        }
    }

    // 当两条都成功后，就创建任务执行后续时间更新、状态更新和摄像头启动
    if (s_version_msg_ok && s_rssi_msg_ok) {
        ESP_LOGI(TAG, "MQTT birth messages all sent => create mqtt_done_task...");
        xTaskCreate(mqtt_done_task, "mqtt_done_task", 4096, NULL, 5, NULL);
    }
}

/**
 * @brief 根据是否已有 Wi-Fi 配置，决定连接或启动配网（仅在 CMD_WIFI_CONFIG 回调中调用）
 */
static void do_wifi_connect_or_config(void)
{
    bool have_config = gs_bind_get_bind_status();
    if (have_config) {
        ESP_LOGI(TAG, "[do_wifi_connect_or_config] Detected existing Wi-Fi config => connect now");
        gs_wifi_sta_start_connect();
        // 初始状态设置为 正在连接路由器（0x02）
        net_sta_update_status(NET_STATUS_CONNECTING_ROUTER);
        // 注意：当 Wi-Fi 连接成功后，应在 Wi-Fi 事件回调中调用 net_sta_update_status(NET_STATUS_CONNECTED_ROUTER)
    } else {
        ESP_LOGI(TAG, "[do_wifi_connect_or_config] No Wi-Fi config => start AP+BLE provisioning");
        gs_bind_start_cfg_mode(GS_BIND_CFG_MODE_AP | GS_BIND_CFG_MODE_BLE);
        net_sta_update_status(NET_STATUS_NOT_CONFIGURED);
    }
}

/**
 * @brief UART 数据包回调：处理各种指令（0x01/0x1A/0x03/0x04/0x12/0x1C/...）
 */
static void uart_packet_received(const uart_packet_t *packet)
{
    ESP_LOGI(TAG, "uart_packet_received: CMD=0x%02X", packet->command);

    switch (packet->command) {

    // 0x01: WiFi 配网指令
    case CMD_WIFI_CONFIG: {
        ESP_LOGI(TAG, "Got CMD_WIFI_CONFIG (0x01) => do_wifi_connect_or_config...");
        do_wifi_connect_or_config();
        // 启动联网状态监控定时器，只有在收到配网指令后才启动监控
#if FIRMWARE_VERSION_MAJOR >= 9
        net_sta_start_monitor();
#endif
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "WiFi connect/config done? -> send CMD_WIFI_RESPONSE (0x02) ack => success=WIFI_CONFIG_SUCCESS (0x00)");
        esp_err_t ret = uart_comm_send_wifi_response(WIFI_CONFIG_SUCCESS);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send WiFi configuration response");
        }
        break;
    }

    // 0x1A: 退出配网
    case CMD_EXIT_CONFIG: {
        ESP_LOGI(TAG, "Got CMD_EXIT_CONFIG (0x1A) => stop provisioning");
        gs_bind_stop_cfg_mode();
        esp_err_t ret = uart_comm_send_exit_config_ack();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send Exit Config ACK");
        } else {
            ESP_LOGI(TAG, "Exit Config ACK sent successfully, waiting 3 seconds...");
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
        break;
    }

    // 0x23: 网络状态（WiFi→MCU，通常可忽略）
    case CMD_NETWORK_STATUS: {
        ESP_LOGI(TAG, "Received CMD_NETWORK_STATUS=0x23 from MCU (usually ignored)");
        break;
    }

    // 0x03 / 0x04: 转交给 msg_upload 模块
    case 0x03:
    case 0x04:
        ESP_LOGI(TAG, "Got CMD=0x%02X => forward to msg_upload...", packet->command);
        msg_upload_uart_callback(packet);
        break;

    // 0x12: 远程开锁应答（MCU→WiFi）
    case 0x12:
        ESP_LOGI(TAG, "Got CMD=0x12 => unlock_handle_mcu_packet");
        unlock_handle_mcu_packet(packet);
        break;

    // 0x1C: 图传设置（MCU→WiFi），交由 img_transfer 模块处理
    case CMD_IMG_TRANSFER: {
        ESP_LOGI(TAG, "Got CMD_IMG_TRANSFER (0x1C) => forward to img_transfer");
        img_transfer_handle_uart_packet(packet);
        break;
    }

    // 新增：0x42 状态上传，由 state_report 模块处理
    case CMD_STATE_REPORT: {
        ESP_LOGI(TAG, "Got CMD_STATE_REPORT (0x42) => forward to state_report module");
        state_report_handle_uart_packet(packet);
        break;
    }

    default:
        ESP_LOGW(TAG, "Unknown cmd=0x%02X", packet->command);
        break;
    }
}

void app_main(void)
{
    // 1. 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 初始化项目核心组件
    CC_LOGI(TAG, "=== cc_init from project ===");
    cc_hal_sys_init();
    cc_hal_kvs_init();
    cc_hal_wifi_init();
    cc_event_init();
    cc_timer_init();
    cc_tmr_task_init();

    gs_init("1.21.0.0", "1.0.0");
    product_init();
    gs_device_init();

    // 3. 初始化 get_time 模块
    get_time_init();

    // 4. 启动网络循环任务
    xTaskCreate(network_task, "Network Task", 4096, NULL, 5, NULL);

    // 5. 初始化 net_sta 模块（仅当固件版本 >= 9 时启用联网状态管理）
    if (FIRMWARE_VERSION_MAJOR >= 9) {
        ret = net_sta_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "net_sta_init failed");
        } else {
            ESP_LOGI(TAG, "net_sta_init succeeded");
            // 注意：联网状态监控定时器仅在收到配网指令后启动，不在此处自动启动
        }
    } else {
        ESP_LOGW(TAG, "Firmware version < 9, net_sta module disabled");
    }

    if (!gs_bind_get_bind_status()) {
        ESP_LOGI(TAG, "No saved Wi-Fi config => waiting for CMD=0x01 from MCU to start provisioning/connection...");
    } else {
        ESP_LOGI(TAG, "Wi-Fi config is saved => but will not connect until CMD=0x01 from MCU...");
    }

    // 注册 MQTT 出生消息回调
    gs_mqtt_register_birth_callback(birth_msg_callback);

    // 初始化图片上传模块
    const char *server_url = "http://120.25.207.32:3466/upload/ajaxuploadfile.php";
    ret = img_upload_init(server_url);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "img_upload_init failed");
    } else {
        ESP_LOGI(TAG, "img_upload module initialized, URL: %s", server_url);
    }

    // 6. 初始化 UART
    ret = uart_comm_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART communication init failed");
    } else {
        ESP_LOGI(TAG, "UART communication initialized");
    }
    uart_comm_register_callback(uart_packet_received);

    // 新增：初始化状态上报模块（包括重传与缓存机制）
    ret = state_report_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "state_report_init failed");
    } else {
        ESP_LOGI(TAG, "state_report module initialized");
    }

    // 初始化 msg_upload 模块
    ret = msg_upload_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "msg_upload_init failed: %d", ret);
    } else {
        ESP_LOGI(TAG, "msg_upload_init succeeded");
    }

    // 初始化 unlock 模块
    ret = unlock_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "unlock_init failed: %d", ret);
    } else {
        ESP_LOGI(TAG, "unlock_init succeeded");
    }

    // 初始化 img_transfer 模块
    ret = img_transfer_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "img_transfer_init failed: %d", ret);
    } else {
        ESP_LOGI(TAG, "img_transfer_init succeeded");
    }

    // 7. 主循环（空转）
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
