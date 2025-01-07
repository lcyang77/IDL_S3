/************************************************
 * File: get_time.c
 ************************************************/
#include "get_time.h"

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "cJSON.h"

static const char *TAG = "get_time";

/*======================
 * 内部全局变量
 *=====================*/

// 存放最新UTC和时区
static uint32_t s_utc_time  = 0;
static int8_t   s_time_zone = 0;

// HTTP响应缓冲
#define RESP_BUF_SIZE 1024
static char s_resp_buf[RESP_BUF_SIZE];
static int  s_resp_len = 0;

// 事件组 & 事件位
static EventGroupHandle_t s_time_event_group = NULL;
#define TIME_UPDATE_DONE_BIT    BIT0
#define TIME_UPDATE_SUCCESS_BIT BIT1

// 最大重试次数
static const int MAX_RETRIES = 3;
static int s_retry_count = 0;

/*======================
 * 内部函数声明
 *=====================*/
static esp_err_t do_http_request(void);
static esp_err_t _http_event_handler(esp_http_client_event_t *evt);
static void parse_time_json(const char *json_str);

/*======================
 * 对外接口实现
 *=====================*/

/**
 * @brief 初始化
 */
void get_time_init(void)
{
    // 初始化缓存
    s_utc_time  = 0;
    s_time_zone = 0;

    // 创建事件组
    s_time_event_group = xEventGroupCreate();
    if (!s_time_event_group) {
        ESP_LOGE(TAG, "Failed to create event group for time!");
    }

    ESP_LOGI(TAG, "get_time_init done");
}

/**
 * @brief 启动一次获取时间的流程
 *
 * 可能执行最多3次重试。函数本身阻塞执行HTTP请求(esp_http_client_perform)，
 * 但不等待解析完成(由事件回调负责解析)。
 */
esp_err_t get_time_start_update(void)
{
    // 每次开始前清除事件位
    xEventGroupClearBits(s_time_event_group, 
                         TIME_UPDATE_DONE_BIT | TIME_UPDATE_SUCCESS_BIT);

    s_retry_count = 0;
    return do_http_request();
}

/**
 * @brief 阻塞等待“获取时间”完成(成功或失败)
 */
bool get_time_wait_done(uint32_t timeout_ms)
{
    EventBits_t bits = xEventGroupWaitBits(
        s_time_event_group,
        TIME_UPDATE_DONE_BIT | TIME_UPDATE_SUCCESS_BIT,
        pdTRUE,    // 读取后清除
        pdFALSE,   // 任意一个位即可返回
        pdMS_TO_TICKS(timeout_ms)
    );

    // 如果包含SUCCESS_BIT则表示成功
    return (bits & TIME_UPDATE_SUCCESS_BIT) != 0;
}

/**
 * @brief 获取UTC
 */
uint32_t get_time_get_utc(void)
{
    return s_utc_time;
}

/**
 * @brief 获取时区
 */
int8_t get_time_get_timezone(void)
{
    return s_time_zone;
}

/*======================
 * 内部函数实现
 *=====================*/

/**
 * @brief 真正执行 HTTP 请求并带重试
 */
static esp_err_t do_http_request(void)
{
    esp_err_t err;
    // 先清空响应缓存
    s_resp_len = 0;
    memset(s_resp_buf, 0, sizeof(s_resp_buf));

    ESP_LOGI(TAG, "do_http_request: attempt=%d/%d", s_retry_count+1, MAX_RETRIES);

    // 配置 HTTP 客户端
    esp_http_client_config_t config = {
        .url           = "http://gaoshi.wdaoyun.cn/mqtt/getTime.php",
        .event_handler = _http_event_handler,
        .timeout_ms    = 3000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "esp_http_client_init failed");
        return ESP_FAIL;
    }

    // 同步执行请求
    err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);

        // 若失败则重试(最多3次)
        s_retry_count++;
        if (s_retry_count < MAX_RETRIES) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            return do_http_request();
        } else {
            // 超过最大重试
            return err;
        }
    }

    // 成功发送并收到响应(阻塞到HTTP完毕)
    esp_http_client_cleanup(client);
    return ESP_OK;
}

/**
 * @brief HTTP事件回调：收集数据并在FINISH时解析
 */
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA: {
        int copy_len = evt->data_len;
        if (s_resp_len + copy_len >= RESP_BUF_SIZE) {
            copy_len = RESP_BUF_SIZE - 1 - s_resp_len;
        }
        if (copy_len > 0) {
            memcpy(&s_resp_buf[s_resp_len], evt->data, copy_len);
            s_resp_len += copy_len;
            s_resp_buf[s_resp_len] = '\0';
        }
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    }
    case HTTP_EVENT_ON_FINISH: {
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH => total=%d bytes", s_resp_len);
        if (s_resp_len > 0) {
            parse_time_json(s_resp_buf);
            xEventGroupSetBits(s_time_event_group,
                               TIME_UPDATE_DONE_BIT | TIME_UPDATE_SUCCESS_BIT);
        } else {
            // 无数据，不算成功
            xEventGroupSetBits(s_time_event_group, TIME_UPDATE_DONE_BIT);
        }
        break;
    }
    case HTTP_EVENT_ERROR: {
        ESP_LOGW(TAG, "HTTP_EVENT_ERROR");
        xEventGroupSetBits(s_time_event_group, TIME_UPDATE_DONE_BIT);
        break;
    }
    default:
        break;
    }
    return ESP_OK;
}

/**
 * @brief 解析 JSON 响应
 * 形如 {"time":1234, "zone":"UTC +8"}
 */
static void parse_time_json(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return;
    }

    cJSON *time_item = cJSON_GetObjectItem(root, "time");
    cJSON *zone_item = cJSON_GetObjectItem(root, "zone");

    // UTC时间
    if (cJSON_IsNumber(time_item)) {
        s_utc_time = (uint32_t) time_item->valuedouble;
    }

    // 时区
    if (cJSON_IsString(zone_item)) {
        const char *p = strstr(zone_item->valuestring, "UTC");
        if (p) p += 3; // 跳过"UTC"
        else p = zone_item->valuestring;

        float zone_float = 0.0f;
        if (sscanf(p, "%f", &zone_float) == 1) {
            float hour_part = (float)((int)zone_float);
            float fraction  = zone_float - hour_part;
            int quarter     = (int)(fraction*4.0f + (fraction>=0? 0.5f:-0.5f));
            int hour_base   = (int)(hour_part*10);
            s_time_zone     = (int8_t)(hour_base + (quarter*3));
        }
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Updated time => utc=%u, timezone=%d",
             (unsigned)s_utc_time, (int)s_time_zone);
}
