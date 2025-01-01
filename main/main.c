// main.c
/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// ========== 公共头文件 ==========
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

// ========== ESP-IDF / FreeRTOS 相关头文件 ==========
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

// ========== 项目1 相关头文件 ==========
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

// ========== 图片上传、配网相关头文件 ==========
#include "img_upload.h"
#include "gs_bind.h"

// ========== UVC 摄像头独立模块 ==========
#include "uvc_camera.h"

// ========== 宏定义 & 全局变量 ==========
static const char *TAG = "app_main";

// 用于判断 MQTT “出生消息”是否都发送成功
static bool s_version_msg_ok = false;
static bool s_rssi_msg_ok    = false;

// ========== 网络循环任务：跑项目的事件循环等 ==========
static void network_task(void *arg)
{
    uint64_t last = cc_hal_sys_get_ms();
    while (1) {
        uint64_t now = cc_hal_sys_get_ms();
        uint16_t ms = now - last;
        last = now;

        // 跑项目1事件循环
        cc_event_run();
        cc_timer_run(CC_TIMMER_MS(ms));
        cc_http_run(ms);
        cc_tmr_task_run(ms);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ========== MQTT 出生消息回调：在成功后启动 UVC 摄像头 ==========
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

    // 当两条消息都成功发送，视为配网+MQTT已就绪，开始初始化UVC
    if (s_version_msg_ok && s_rssi_msg_ok) {
        ESP_LOGI(TAG, "MQTT birth messages all sent, start UVC camera now...");
        // 调用 UVC 模块的启动函数
        uvc_camera_start();
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

    // 2. 初始化项目1核心组件
    CC_LOGI(TAG, "=== cc_init from project ===");
    cc_hal_sys_init();
    cc_hal_kvs_init();
    cc_hal_wifi_init();
    cc_event_init();
    cc_timer_init();
    cc_tmr_task_init();

    // gs_main + product_init
    gs_init("1.21.0.0", "1.0.0");  // 内部会调用 gs_bind_init 等
    product_init();

    // 启动网络循环任务
    xTaskCreate(network_task, "Network Task", 4096, NULL, 5, NULL);

    // ========== 检查是否已配网，没有则启动配网 ========== 
    if (!gs_bind_get_bind_status()) {
        ESP_LOGI(TAG, "No WiFi config found, starting AP+BLE provisioning...");
        gs_bind_start_cfg_mode(GS_BIND_CFG_MODE_AP | GS_BIND_CFG_MODE_BLE);
    } else {
        ESP_LOGI(TAG, "Already bound, skip provisioning");
    }

    // 3. 注册 MQTT 出生消息回调
    gs_mqtt_register_birth_callback(birth_msg_callback);

    // 4. 初始化图片上传模块
    const char *server_url = "http://120.25.207.32:3466/upload/ajaxuploadfile.php";
    ret = img_upload_init(server_url);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "img_upload_init failed");
    } else {
        ESP_LOGI(TAG, "img_upload module initialized, URL: %s", server_url);
    }

    // 【注意】此时并未初始化 UVC，只有在 birth_msg_callback() 中确认配网+MQTT成功后，
    //       才调用 uvc_camera_start()

    // 主任务空转或执行其他逻辑
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
