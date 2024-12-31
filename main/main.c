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

// ========== 项目1 相关头文件 ==========
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

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

// ========== 项目1: 图片上传功能 + 配网 (gs_bind) ==========
#include "img_upload.h"  // img_upload_init() / img_upload_send()
#include "gs_bind.h"     // gs_bind_get_bind_status(), gs_bind_start_cfg_mode()

// ========== UVC (项目2) 相关头文件 ==========
#include "usb_stream.h"
#include "esp_camera.h"  // camera_fb_t, PIXFORMAT_JPEG
#ifdef CONFIG_ESP32_S3_USB_OTG
#include "bsp/esp-bsp.h"
#endif

// ========== 宏定义 ==========
static const char *TAG = "uvc_merge_example";

// 任务周期等定义
#define LOOP_INTERVAL   10
#define UVC_CAPTURE_UPLOAD_PERIOD_MS   (5000)

// UVC 分辨率和缓冲区大小
#define DEMO_UVC_FRAME_WIDTH   1280
#define DEMO_UVC_FRAME_HEIGHT  720

#ifdef CONFIG_IDF_TARGET_ESP32S2
#define DEMO_UVC_XFER_BUFFER_SIZE (45 * 1024)
#else
#define DEMO_UVC_XFER_BUFFER_SIZE (1024 * 1024)
#endif

// 事件组：用于帧同步
#define BIT0_FRAME_START     (0x01 << 0)
#define BIT1_NEW_FRAME_START (0x01 << 1)
#define BIT2_NEW_FRAME_END   (0x01 << 2)

// ========== 全局/静态变量区 ==========

// 用于帧获取/回调的事件组和帧缓冲
static EventGroupHandle_t s_evt_handle = NULL;
static camera_fb_t s_fb = {0};

// 用于判断 MQTT “出生消息”是否都发送成功
static bool s_version_msg_ok = false;
static bool s_rssi_msg_ok    = false;

// ========== UVC 回调：填充帧数据 ==========
static void camera_frame_cb(uvc_frame_t *frame, void *ptr)
{
    // 若未设置开始采集，则忽略
    if (!(xEventGroupGetBits(s_evt_handle) & BIT0_FRAME_START)) {
        return;
    }

    // 仅支持 MJPEG (示例)
    if (frame->frame_format == UVC_FRAME_FORMAT_MJPEG) {
        s_fb.buf    = frame->data;
        s_fb.len    = frame->data_bytes;
        s_fb.width  = frame->width;
        s_fb.height = frame->height;
        s_fb.format = PIXFORMAT_JPEG;
        s_fb.timestamp.tv_sec = frame->sequence;

        // 通知有新帧到达
        xEventGroupSetBits(s_evt_handle, BIT1_NEW_FRAME_START);
        // 等待处理完再返回
        xEventGroupWaitBits(s_evt_handle, BIT2_NEW_FRAME_END, true, true, portMAX_DELAY);
    } else {
        ESP_LOGW(TAG, "Received unsupported frame format: %d", frame->frame_format);
    }
}

// ========== UVC状态回调 ==========
static void stream_state_changed_cb(usb_stream_state_t event, void *arg)
{
    switch (event) {
    case STREAM_CONNECTED:
        ESP_LOGI(TAG, "UVC Device connected");
        break;
    case STREAM_DISCONNECTED:
        ESP_LOGI(TAG, "UVC Device disconnected");
        break;
    default:
        ESP_LOGE(TAG, "Unknown UVC event");
        break;
    }
}

// ========== 摄像头帧获取接口 ==========
camera_fb_t *esp_camera_fb_get(void)
{
    xEventGroupSetBits(s_evt_handle, BIT0_FRAME_START);
    xEventGroupWaitBits(s_evt_handle, BIT1_NEW_FRAME_START, true, true, portMAX_DELAY);
    return &s_fb;
}

// ========== 释放帧 ==========
void esp_camera_fb_return(camera_fb_t *fb)
{
    (void)fb;
    xEventGroupSetBits(s_evt_handle, BIT2_NEW_FRAME_END);
}

// ========== 项目1: 网络循环任务 ==========
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

        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL));
    }
}

// ========== 定时抓取UVC并上传任务 ==========
static void uvc_capture_upload_task(void *pvParameters)
{
    while (1) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Failed to get frame from UVC camera");
        } else {
            ESP_LOGI(TAG, "UVC frame size: %u bytes", fb->len);
            // 上传
            esp_err_t ret = img_upload_send((const uint8_t *)fb->buf, fb->len);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "UVC frame upload success");
            } else {
                ESP_LOGE(TAG, "UVC frame upload failed");
            }
            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(UVC_CAPTURE_UPLOAD_PERIOD_MS));
    }
}

// ========== 在MQTT “出生消息”都发送成功后，初始化UVC摄像头 ==========

/**
 * @brief  在配网成功（两条MQTT出生消息都发送成功）后才真正上电并初始化UVC
 */
static void start_uvc_camera(void)
{
    // 1. 创建事件组（若尚未创建）
    if (s_evt_handle == NULL) {
        s_evt_handle = xEventGroupCreate();
        if (s_evt_handle == NULL) {
            ESP_LOGE(TAG, "Failed to create event group for UVC");
            return;
        }
    }

    // 2. 分配传输与帧缓冲内存
    uint8_t *xfer_buffer_a = (uint8_t *)malloc(DEMO_UVC_XFER_BUFFER_SIZE);
    uint8_t *xfer_buffer_b = (uint8_t *)malloc(DEMO_UVC_XFER_BUFFER_SIZE);
    uint8_t *frame_buffer  = (uint8_t *)malloc(DEMO_UVC_XFER_BUFFER_SIZE);
    assert(xfer_buffer_a && xfer_buffer_b && frame_buffer);

    // 3. 配置 UVC
    uvc_config_t uvc_config = {
        .frame_width       = DEMO_UVC_FRAME_WIDTH,
        .frame_height      = DEMO_UVC_FRAME_HEIGHT,
        .frame_interval    = FPS2INTERVAL(15),  // 15fps
        .xfer_buffer_size  = DEMO_UVC_XFER_BUFFER_SIZE,
        .xfer_buffer_a     = xfer_buffer_a,
        .xfer_buffer_b     = xfer_buffer_b,
        .frame_buffer_size = DEMO_UVC_XFER_BUFFER_SIZE,
        .frame_buffer      = frame_buffer,
        .frame_cb          = &camera_frame_cb,
        .frame_cb_arg      = NULL,
    };

    esp_err_t ret = uvc_streaming_config(&uvc_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uvc_streaming_config failed");
        return;
    }

    // 4. 注册UVC状态回调，启动并等待连接
    ESP_ERROR_CHECK(usb_streaming_state_register(&stream_state_changed_cb, NULL));
    ESP_ERROR_CHECK(usb_streaming_start());
    ESP_ERROR_CHECK(usb_streaming_connect_wait(portMAX_DELAY));

    // 5. 启动定时抓取并上传任务
    xTaskCreate(uvc_capture_upload_task, "uvc_capture_upload_task", 8192, NULL, 5, NULL);

    ESP_LOGI(TAG, "UVC camera initialization done.");
}

/**
 * @brief  MQTT 出生消息回调函数，判断版本与RSSI消息是否都已发送成功
 *
 * @param  msg_type: 0表示版本消息，1表示RSSI消息
 * @param  status:   1表示发送成功，0表示失败
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

    // 当两条消息都成功发送，视为配网+MQTT已就绪，开始初始化UVC
    if (s_version_msg_ok && s_rssi_msg_ok) {
        ESP_LOGI(TAG, "MQTT birth messages all sent, start UVC camera now...");
        start_uvc_camera();
    }
}

// ========== 主函数 ==========
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
    CC_LOGI(TAG, "=== cc_init from project1 ===");
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

    // 【注意】此时并未初始化 UVC，只有在 birth_msg_callback() 中确认配网+MQTT成功后，才调用 start_uvc_camera() 

    // 主任务空转或执行其他逻辑
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}