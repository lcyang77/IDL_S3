// uvc_camera.c
/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"

#include "usb_stream.h"
#include "esp_camera.h" // camera_fb_t, PIXFORMAT_JPEG
#include "img_upload.h" // img_upload_send()

#include "uvc_camera.h"

static const char *TAG = "uvc_camera_module";

// ========== 事件位，用于帧同步 ==========
#define BIT0_FRAME_START     (1 << 0)
#define BIT1_NEW_FRAME_START (1 << 1)
#define BIT2_NEW_FRAME_END   (1 << 2)

// ========== UVC 分辨率、缓冲区大小配置 ==========
#define DEMO_UVC_FRAME_WIDTH   1280
#define DEMO_UVC_FRAME_HEIGHT  720

#ifdef CONFIG_IDF_TARGET_ESP32S2
#define DEMO_UVC_XFER_BUFFER_SIZE (45 * 1024)
#else
#define DEMO_UVC_XFER_BUFFER_SIZE (1024 * 1024)
#endif

// ========== 抓取并上传的周期(ms) ==========
#define UVC_CAPTURE_UPLOAD_PERIOD_MS   (5000)

// ========== 静态全局变量：事件组 & 帧缓冲 ==========
static EventGroupHandle_t s_evt_handle = NULL;
static camera_fb_t s_fb = {0};  // 用于暂存当前帧

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
        xEventGroupWaitBits(s_evt_handle, BIT2_NEW_FRAME_END, pdTRUE, pdTRUE, portMAX_DELAY);
    } else {
        ESP_LOGW(TAG, "Received unsupported frame format: %d", frame->frame_format);
    }
}

// ========== UVC 状态回调 ==========
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

// ========== 采集帧接口：获取一帧 ==========
camera_fb_t *esp_camera_fb_get(void)
{
    // 标记开始采集
    xEventGroupSetBits(s_evt_handle, BIT0_FRAME_START);
    // 等待新帧到达
    xEventGroupWaitBits(s_evt_handle, BIT1_NEW_FRAME_START, pdTRUE, pdTRUE, portMAX_DELAY);
    return &s_fb;
}

// ========== 采集帧接口：释放一帧 ==========
void esp_camera_fb_return(camera_fb_t *fb)
{
    // 这里只是通知可以采集下一帧，实际不释放任何内存
    (void)fb;
    xEventGroupSetBits(s_evt_handle, BIT2_NEW_FRAME_END);
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

// ========== 启动UVC摄像头：对外暴露的接口 ==========
void uvc_camera_start(void)
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
        .frame_cb          = camera_frame_cb,
        .frame_cb_arg      = NULL,
    };

    esp_err_t ret = uvc_streaming_config(&uvc_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uvc_streaming_config failed");
        return;
    }

    // 4. 注册UVC状态回调，启动并等待连接
    ESP_ERROR_CHECK(usb_streaming_state_register(stream_state_changed_cb, NULL));
    ESP_ERROR_CHECK(usb_streaming_start());
    ESP_ERROR_CHECK(usb_streaming_connect_wait(portMAX_DELAY));

    // 5. 启动定时抓取并上传任务
    xTaskCreate(uvc_capture_upload_task, "uvc_capture_upload_task", 8192, NULL, 5, NULL);

    ESP_LOGI(TAG, "UVC camera initialization done.");
}
