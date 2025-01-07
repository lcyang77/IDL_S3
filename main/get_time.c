#include "get_time.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "get_time";

// 保存最近一次成功获取的UTC时间和时区
static uint32_t s_utc_time  = 0; 
static int8_t   s_time_zone = 0;

// 用于事件回调模式存储响应体
#define GET_TIME_RESP_BUF_SIZE  1024
static char s_resp_buf[GET_TIME_RESP_BUF_SIZE];
static int  s_resp_len = 0;

// 回调函数
static get_time_callback_t s_get_time_callback = NULL;

// 解析响应体中的JSON: {"time":1234, "zone":"UTC +8"}
static void parse_time_json(const char *json_str)
{
    ESP_LOGI(TAG, "Parsing JSON: %s", json_str);

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return;
    }

    cJSON *time_item = cJSON_GetObjectItem(root, "time");
    cJSON *zone_item = cJSON_GetObjectItem(root, "zone");

    // 更新UTC时间
    if (cJSON_IsNumber(time_item)) {
        s_utc_time = (uint32_t) time_item->valuedouble;
    } else {
        ESP_LOGW(TAG, "No valid 'time' field, keep old s_utc_time=%u", (unsigned)s_utc_time);
    }

    // 解析时区字符串 "UTC +8" / "UTC -3.5" / ...
    if (cJSON_IsString(zone_item)) {
        const char *zstr = zone_item->valuestring;
        if (zstr) {
            // 跳过 "UTC"
            const char *p = strstr(zstr, "UTC");
            if (p) {
                p += 3; 
            } else {
                p = zstr;
            }

            float zone_float = 0.0f;
            int ret_sscanf = sscanf(p, "%f", &zone_float);
            if (ret_sscanf == 1) {
                // 例如 8.0 / -3.5 / 5.75
                float hour_part = (float)((int)zone_float); 
                float fraction  = zone_float - hour_part;
                int quarter     = (int)(fraction * 4.0f + (fraction >= 0 ? 0.5f : -0.5f));
                int hour_base   = (int)(hour_part * 10);
                int tz_val      = hour_base + (quarter * 3);
                s_time_zone     = (int8_t)tz_val;
            } else {
                ESP_LOGW(TAG, "No valid zone float in '%s'", p);
            }
        }
    } else {
        ESP_LOGW(TAG, "No valid 'zone' field, keep old s_time_zone=%d", (int)s_time_zone);
    }

    cJSON_Delete(root);

    ESP_LOGI(TAG, "Updated time => utc=%u, timezone=%d", (unsigned)s_utc_time, (int)s_time_zone);
}

/**
 * @brief HTTP事件回调：采用回调模式接收数据
 */
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        // 收到响应体数据的一部分
        if (!evt->user_data) {
            // 未指定 user_data 时，用全局静态缓冲
            int copy_len = evt->data_len;
            // 防止越界
            if (copy_len + s_resp_len >= GET_TIME_RESP_BUF_SIZE - 1) {
                copy_len = GET_TIME_RESP_BUF_SIZE - 1 - s_resp_len;
            }
            if (copy_len > 0) {
                memcpy(&s_resp_buf[s_resp_len], evt->data, copy_len);
                s_resp_len += copy_len;
                s_resp_buf[s_resp_len] = '\0';
            }

            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // 可按需调试
            // printf("Data chunk: %.*s\n", evt->data_len, (char*)evt->data);
        }
        break;

    case HTTP_EVENT_ON_FINISH:
        // 整个响应接收完毕
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH => total=%d bytes", s_resp_len);
        if (s_resp_len > 0) {
            // 解析
            parse_time_json(s_resp_buf);
            if (s_get_time_callback) {
                s_get_time_callback(true);
            }
        } else {
            ESP_LOGW(TAG, "No data in s_resp_buf => no JSON to parse");
            if (s_get_time_callback) {
                s_get_time_callback(false);
            }
        }
        break;

    case HTTP_EVENT_ERROR:
        ESP_LOGW(TAG, "HTTP_EVENT_ERROR");
        if (s_get_time_callback) {
            s_get_time_callback(false);
        }
        break;

    default:
        break;
    }
    return ESP_OK;
}

/**
 * @brief 进行异步 HTTP 请求，使用事件回调收集响应数据
 */
void get_time_update(void)
{
    // 调用前，清空缓冲
    s_resp_len = 0;
    memset(s_resp_buf, 0, sizeof(s_resp_buf));

    ESP_LOGI(TAG, "get_time_update: start fetch from server...");

    esp_http_client_config_t config = {
        .url           = "http://gaoshi.wdaoyun.cn/mqtt/getTime.php",
        .event_handler = _http_event_handler,
        .timeout_ms    = 3000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "esp_http_client_init failed");
        if (s_get_time_callback) {
            s_get_time_callback(false);
        }
        return;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        if (s_get_time_callback) {
            s_get_time_callback(false);
        }
    } else {
        ESP_LOGI(TAG, "HTTP request initiated, waiting for response...");
        // 事件回调中已经收集+解析完数据，并调用回调函数
    }

    esp_http_client_cleanup(client);
}

/**
 * @brief 注册时间更新完成的回调
 */
esp_err_t get_time_register_callback(get_time_callback_t callback)
{
    s_get_time_callback = callback;
    return ESP_OK;
}

uint32_t get_time_get_utc(void)
{
    return s_utc_time;
}

int8_t get_time_get_timezone(void)
{
    return s_time_zone;
}

/**
 * @brief 对外接口：初始化
 */
void get_time_init(void)
{
    s_utc_time  = 0;
    s_time_zone = 0;
    ESP_LOGI(TAG, "get_time_init done (utc=0, tz=0)");
}
