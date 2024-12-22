// img_upload.c
#include "img_upload.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "cJSON.h"

static const char *TAG = "img_upload";

// 用于存储服务器URL
static char server_url_global[256] = {0};
// 用于存储Authorization头
static char auth_header[128] = {0};

// multipart form-data 的 boundary
#define BOUNDARY "------------------------d74496d66958873e"

// 响应缓冲区
static char response_buffer[1024];
static int response_len = 0;

// HTTP 事件回调
static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // 打印服务器返回的部分数据
            ESP_LOGI(TAG, "Response data: %.*s", evt->data_len, (char*)evt->data);

            // 保存到本地缓冲区 (拼接)
            if (response_len + evt->data_len < sizeof(response_buffer)) {
                memcpy(response_buffer + response_len, evt->data, evt->data_len);
                response_len += evt->data_len;
                response_buffer[response_len] = '\0';
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            ESP_LOGI(TAG, "Raw response: %s", response_buffer);
            // 解析 JSON 响应
            if (response_len > 0) {
                cJSON *root = cJSON_Parse(response_buffer);
                if (root) {
                    cJSON *error = cJSON_GetObjectItem(root, "error");
                    if (error && (error->valueint == 0)) {
                        cJSON *img_url = cJSON_GetObjectItem(root, "img_url");
                        if (img_url) {
                            ESP_LOGI(TAG, "Upload OK, image URL: %s", img_url->valuestring);
                        }
                    } else {
                        // 上传失败
                        cJSON *showmsg = cJSON_GetObjectItem(root, "showmsg");
                        if (showmsg) {
                            ESP_LOGE(TAG, "Upload failed: %s", showmsg->valuestring);
                        }
                    }
                    cJSON_Delete(root);
                } else {
                    ESP_LOGE(TAG, "JSON parse failed");
                }
            }
            response_len = 0; // 清空缓冲区
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}

// 初始化：设置服务器 URL，并设置 Authorization header
esp_err_t img_upload_init(const char *server_url) {
    if (!server_url) {
        ESP_LOGE(TAG, "Server URL is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(server_url) >= sizeof(server_url_global)) {
        ESP_LOGE(TAG, "Server URL too long");
        return ESP_ERR_INVALID_ARG;
    }
    strcpy(server_url_global, server_url);

    // 构建 Authorization header (示例)
    snprintf(auth_header, sizeof(auth_header),
             "secret {5c627423c152a8717eb659107ba7549c}");

    ESP_LOGI(TAG, "img_upload_init: server_url=%s", server_url_global);
    ESP_LOGI(TAG, "img_upload_init: auth_header=%s", auth_header);

    return ESP_OK;
}

// 判断是否是合法 JPEG
static bool is_valid_jpeg(const uint8_t *data, size_t len) {
    if (len < 4) return false;
    // JPEG头 (FFD8)
    if (data[0] != 0xFF || data[1] != 0xD8) return false;
    // JPEG尾 (FFD9)
    if (data[len-2] != 0xFF || data[len-1] != 0xD9) return false;
    return true;
}

// 上传图片接口
esp_err_t img_upload_send(const uint8_t *data, size_t len) {
    if (!data || len == 0) {
        ESP_LOGE(TAG, "Invalid input data");
        return ESP_ERR_INVALID_ARG;
    }

    // 可选：检查JPEG格式
    if (!is_valid_jpeg(data, len)) {
        ESP_LOGE(TAG, "Invalid JPEG format");
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(server_url_global) == 0) {
        ESP_LOGE(TAG, "Server URL not set");
        return ESP_ERR_INVALID_STATE;
    }

    // 配置HTTP客户端
    esp_http_client_config_t config = {
        .url = server_url_global,
        .event_handler = _http_event_handler,
        .max_redirection_count = 5,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "esp_http_client_init failed");
        return ESP_FAIL;
    }

    // 设置HTTP头
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", 
        "multipart/form-data; boundary=" BOUNDARY);
    // 一般不需要 Expect: 100-continue
    esp_http_client_set_header(client, "Expect", "");

    // multipart 格式头部
    const char *header_format =
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"upload\"; filename=\"image.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n\r\n";

    char header[512];
    int header_len = snprintf(header, sizeof(header), header_format, BOUNDARY);

    // multipart 格式尾部
    char footer[64];
    int footer_len = snprintf(footer, sizeof(footer), "\r\n--%s--\r\n", BOUNDARY);

    esp_http_client_set_method(client, HTTP_METHOD_POST);

    // 打开连接
    int total_len = header_len + len + footer_len;
    esp_err_t err = esp_http_client_open(client, total_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    // 写入头部
    int written = esp_http_client_write(client, header, header_len);
    if (written != header_len) {
        ESP_LOGE(TAG, "Failed to write multipart header");
        err = ESP_FAIL;
        goto cleanup;
    }

    // 写入图像数据
    written = esp_http_client_write(client, (const char*)data, len);
    if (written != (int)len) {
        ESP_LOGE(TAG, "Failed to write image data");
        err = ESP_FAIL;
        goto cleanup;
    }

    // 写入尾部
    written = esp_http_client_write(client, footer, footer_len);
    if (written != footer_len) {
        ESP_LOGE(TAG, "Failed to write multipart footer");
        err = ESP_FAIL;
        goto cleanup;
    }

    // 获取响应
    int status = esp_http_client_fetch_headers(client);
    if (status < 0) {
        ESP_LOGE(TAG, "Failed to fetch headers");
        err = ESP_FAIL;
        goto cleanup;
    }

    int response_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP response code: %d", response_code);

    err = (response_code == 200) ? ESP_OK : ESP_FAIL;

cleanup:
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return err;
}
