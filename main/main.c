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

// 新：时间获取模块头文件（改造后）
#include "get_time.h"

// 新：联网状态管理模块头文件
#include "net_sta.h"

static const char *TAG = "app_main";

// 判断 MQTT “出生消息”是否发送成功
static bool s_version_msg_ok = false;
static bool s_rssi_msg_ok    = false;

// 固件版本定义（在 product.h 中定义，此处为示例）
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
 * @brief 当两条MQTT出生消息都成功后，将在这里启动一次异步任务
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

    // 3) 无论成功/失败，这里都认为已经连接云服务器 => 发送0x23(0x04)到MCU
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
    } else if (msg_type == 1) { // RSSI消息
        if (status) {
            ESP_LOGI(TAG, "RSSI message sent successfully");
            s_rssi_msg_ok = true;
        } else {
            ESP_LOGE(TAG, "RSSI message send failed");
        }
    }

    // 当两条都成功后，就创建一个任务来做后续的时间获取 + 状态更新 + 摄像头启动
    if (s_version_msg_ok && s_rssi_msg_ok) {
        ESP_LOGI(TAG, "MQTT birth messages all sent => create mqtt_done_task...");
        xTaskCreate(mqtt_done_task, "mqtt_done_task", 4096, NULL, 5, NULL);
    }
}

/**
 * @brief 根据是否已有 Wi-Fi 配置，决定连接或启动配网(只在 UART 0x01 回调里调用)
 */
static void do_wifi_connect_or_config(void)
{
    bool have_config = gs_bind_get_bind_status();
    if (have_config) {
        ESP_LOGI(TAG, "[do_wifi_connect_or_config] Detected existing Wi-Fi config => connect now");
        gs_wifi_sta_start_connect();

        // 更新联网状态为正在连接路由器
        net_sta_update_status(NET_STATUS_CONNECTING_ROUTER);
    } else {
        ESP_LOGI(TAG, "[do_wifi_connect_or_config] No Wi-Fi config => start AP+BLE provisioning");
        gs_bind_start_cfg_mode(GS_BIND_CFG_MODE_AP | GS_BIND_CFG_MODE_BLE);

        // 更新联网状态为未配置网络
        net_sta_update_status(NET_STATUS_NOT_CONFIGURED);
    }
}

/**
 * @brief UART数据包回调：只处理部分指令(0x01/0x1A/...)
 */
static void uart_packet_received(const uart_packet_t *packet)
{
    ESP_LOGI(TAG, "uart_packet_received: CMD=0x%02X", packet->command);

    switch (packet->command) {
    case CMD_WIFI_CONFIG: {
        // 主板配网指令0x01 => 根据有无配置决定连接或配网
        ESP_LOGI(TAG, "Got CMD_WIFI_CONFIG (0x01) => do_wifi_connect_or_config...");
        do_wifi_connect_or_config();

        // 模拟异步操作，这里延时1秒后应答 0x02
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "WiFi connect/config done? -> send CMD_WIFI_RESPONSE (0x02) ack => success=WIFI_CONFIG_SUCCESS (0x00)");

        esp_err_t ret = uart_comm_send_wifi_response(WIFI_CONFIG_SUCCESS);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send WiFi configuration response");
        }
        break;
    }

    case CMD_EXIT_CONFIG: {
        // 主板发送退出配网指令0x1A
        ESP_LOGI(TAG, "Got CMD_EXIT_CONFIG (0x1A) => stop provisioning");
        gs_bind_stop_cfg_mode();

        // 发送应答 0x1B
        esp_err_t ret = uart_comm_send_exit_config_ack();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send Exit Config ACK");
        } else {
            ESP_LOGI(TAG, "Exit Config ACK sent successfully, waiting 3 seconds...");
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
        break;
    }

    case CMD_NETWORK_STATUS: {
        ESP_LOGI(TAG, "Received CMD_NETWORK_STATUS=0x23 from MCU? (usually WiFi->MCU, might ignore)");
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

    // 初始化 gs_main + product 等
    gs_init("1.21.0.0", "1.0.0");
    product_init();
    gs_device_init();

    // 3. 初始化 get_time 模块(改造后，内部自己管理事件组/重试等)
    get_time_init();

    // 4. 启动网络循环任务
    xTaskCreate(network_task, "Network Task", 4096, NULL, 5, NULL);

    // 5. 初始化 net_sta 模块
    if (FIRMWARE_VERSION_MAJOR >= 9) {
        ret = net_sta_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "net_sta_init failed");
        } else {
            ESP_LOGI(TAG, "net_sta_init succeeded");
            net_sta_start_monitor();
        }
    } else {
        ESP_LOGW(TAG, "Firmware version < 9, net_sta module disabled");
    }

    if (!gs_bind_get_bind_status()) {
        ESP_LOGI(TAG, "No saved Wi-Fi config => waiting for CMD=0x01 from MCU to start provisioning/connection...");
    } else {
        ESP_LOGI(TAG, "Wi-Fi config is saved => but will not connect until 0x01 from MCU...");
    }

    // 注册 MQTT 出生消息回调
    gs_mqtt_register_birth_callback(birth_msg_callback);

    // 初始化图片上传模块(可选)
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

    // 7. 主循环：空转
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
