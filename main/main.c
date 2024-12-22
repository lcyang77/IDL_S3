/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "usb_stream.h"
#ifdef CONFIG_ESP32_S3_USB_OTG
#include "bsp/esp-bsp.h"
#endif
#include "esp_camera.h"  // Added to define camera_fb_t and PIXFORMAT_JPEG

static const char *TAG = "uvc_demo";

/****************** configure the example working mode *******************************/
#define ENABLE_UVC_FRAME_RESOLUTION_ANY   0        /* Using any resolution found from the camera */
#define ENABLE_UVC_WIFI_XFER              1        /* transfer uvc frame to wifi http */

#define EXAMPLE_ESP_WIFI_SSID      "ASUS Wi-Fi7"
#define EXAMPLE_ESP_WIFI_PASS      "y2756096"

#define BIT0_FRAME_START     (0x01 << 0)
#define BIT1_NEW_FRAME_START (0x01 << 1)
#define BIT2_NEW_FRAME_END   (0x01 << 2)

static EventGroupHandle_t s_evt_handle;
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#if (ENABLE_UVC_FRAME_RESOLUTION_ANY)
#define DEMO_UVC_FRAME_WIDTH        FRAME_RESOLUTION_ANY
#define DEMO_UVC_FRAME_HEIGHT       FRAME_RESOLUTION_ANY
#else
#define DEMO_UVC_FRAME_WIDTH        1280
#define DEMO_UVC_FRAME_HEIGHT       720
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2
#define DEMO_UVC_XFER_BUFFER_SIZE (45 * 1024)
#else
#define DEMO_UVC_XFER_BUFFER_SIZE (1024 * 1024)
#endif

static camera_fb_t s_fb = {0};

camera_fb_t *esp_camera_fb_get()
{
    xEventGroupSetBits(s_evt_handle, BIT0_FRAME_START);
    xEventGroupWaitBits(s_evt_handle, BIT1_NEW_FRAME_START, true, true, portMAX_DELAY);
    return &s_fb;
}

void esp_camera_fb_return(camera_fb_t *fb)
{
    xEventGroupSetBits(s_evt_handle, BIT2_NEW_FRAME_END);
    return;
}

static void camera_frame_cb(uvc_frame_t *frame, void *ptr)
{
    ESP_LOGI(TAG, "uvc callback! frame_format = %d, seq = %"PRIu32", width = %"PRIu32", height = %"PRIu32", length = %u, ptr = %d",
             frame->frame_format, frame->sequence, frame->width, frame->height, frame->data_bytes, (int) ptr);
    if (!(xEventGroupGetBits(s_evt_handle) & BIT0_FRAME_START)) {
        return;
    }

    switch (frame->frame_format) {
    case UVC_FRAME_FORMAT_MJPEG:
        s_fb.buf = frame->data;
        s_fb.len = frame->data_bytes;
        s_fb.width = frame->width;
        s_fb.height = frame->height;
        s_fb.format = PIXFORMAT_JPEG;
        s_fb.timestamp.tv_sec = frame->sequence;
        xEventGroupSetBits(s_evt_handle, BIT1_NEW_FRAME_START);
        ESP_LOGV(TAG, "send frame = %"PRIu32"", frame->sequence);
        xEventGroupWaitBits(s_evt_handle, BIT2_NEW_FRAME_END, true, true, portMAX_DELAY);
        ESP_LOGV(TAG, "send frame done = %"PRIu32"", frame->sequence);
        break;
    default:
        ESP_LOGW(TAG, "Format not supported");
        assert(0);
        break;
    }
}

static void stream_state_changed_cb(usb_stream_state_t event, void *arg)
{
    switch (event) {
    case STREAM_CONNECTED: {
        size_t frame_size = 0;
        size_t frame_index = 0;
        uvc_frame_size_list_get(NULL, &frame_size, &frame_index);
        if (frame_size) {
            ESP_LOGI(TAG, "UVC: get frame list size = %u, current = %u", frame_size, frame_index);
            uvc_frame_size_t *uvc_frame_list = (uvc_frame_size_t *)malloc(frame_size * sizeof(uvc_frame_size_t));
            uvc_frame_size_list_get(uvc_frame_list, NULL, NULL);
            for (size_t i = 0; i < frame_size; i++) {
                ESP_LOGI(TAG, "\tframe[%u] = %ux%u", i, uvc_frame_list[i].width, uvc_frame_list[i].height);
            }
            free(uvc_frame_list);
        } else {
            ESP_LOGW(TAG, "UVC: get frame list size = %u", frame_size);
        }
        ESP_LOGI(TAG, "Device connected");
        break;
    }
    case STREAM_DISCONNECTED:
        ESP_LOGI(TAG, "Device disconnected");
        break;
    default:
        ESP_LOGE(TAG, "Unknown event");
        break;
    }
}

// WiFi Event Handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// HTTP Server Handlers
static esp_err_t stream_handler(httpd_req_t *req){
    esp_err_t res = ESP_OK;
    char part_buf[64];  // Changed to char array instead of pointer

    res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=123456789000000000000987654321");
    if(res != ESP_OK){
        return res;
    }

    while(true) {
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        
        size_t hlen = snprintf(part_buf, sizeof(part_buf),
            "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
            fb->len);

        res = httpd_resp_send_chunk(req, part_buf, hlen);
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, "\r\n--123456789000000000000987654321\r\n", 37);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
    }
    return res;
}

static esp_err_t capture_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    
    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    res = httpd_resp_set_type(req, "image/jpeg");
    if(res == ESP_OK){
        res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    }

    if(res == ESP_OK){
        res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    }

    esp_camera_fb_return(fb);
    return res;
}

void app_wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t stream_httpd = NULL;

    httpd_uri_t stream = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t capture = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = capture_handler,
        .user_ctx  = NULL
    };

    ESP_LOGI(TAG, "Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream);
        httpd_register_uri_handler(stream_httpd, &capture);
    }
}

void app_main(void)
{
#ifdef CONFIG_ESP32_S3_USB_OTG
    bsp_usb_mode_select_host();
    bsp_usb_host_power_mode(BSP_USB_HOST_POWER_MODE_USB_DEV, true);
#endif
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("httpd_txrx", ESP_LOG_INFO);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_evt_handle = xEventGroupCreate();
    if (s_evt_handle == NULL) {
        ESP_LOGE(TAG, "line-%u event group create failed", __LINE__);
        assert(0);
    }

    // Initialize WiFi
    app_wifi_init();

    // Wait for WiFi connection
    xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    // Start HTTP Server
    start_webserver();

    /* malloc double buffer for usb payload, xfer_buffer_size >= frame_buffer_size*/
    uint8_t *xfer_buffer_a = (uint8_t *)malloc(DEMO_UVC_XFER_BUFFER_SIZE);
    assert(xfer_buffer_a != NULL);
    uint8_t *xfer_buffer_b = (uint8_t *)malloc(DEMO_UVC_XFER_BUFFER_SIZE);
    assert(xfer_buffer_b != NULL);

    /* malloc frame buffer for a jpeg frame*/
    uint8_t *frame_buffer = (uint8_t *)malloc(DEMO_UVC_XFER_BUFFER_SIZE);
    assert(frame_buffer != NULL);

    uvc_config_t uvc_config = {
        /* match the any resolution of current camera (first frame size as default) */
        .frame_width = DEMO_UVC_FRAME_WIDTH,
        .frame_height = DEMO_UVC_FRAME_HEIGHT,
        .frame_interval = FPS2INTERVAL(15),
        .xfer_buffer_size = DEMO_UVC_XFER_BUFFER_SIZE,
        .xfer_buffer_a = xfer_buffer_a,
        .xfer_buffer_b = xfer_buffer_b,
        .frame_buffer_size = DEMO_UVC_XFER_BUFFER_SIZE,
        .frame_buffer = frame_buffer,
        .frame_cb = &camera_frame_cb,
        .frame_cb_arg = NULL,
    };

    /* config to enable uvc function */
    ret = uvc_streaming_config(&uvc_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uvc streaming config failed");
    }

    /* register the state callback to get connect/disconnect event */
    ESP_ERROR_CHECK(usb_streaming_state_register(&stream_state_changed_cb, NULL));
    
    /* start usb streaming */
    ESP_ERROR_CHECK(usb_streaming_start());
    ESP_ERROR_CHECK(usb_streaming_connect_wait(portMAX_DELAY));

    while (1) {
        vTaskDelay(100);
    }
}

