#include "img_upload.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "cJSON.h"
#include <inttypes.h>

static const char *TAG = "img_upload";
static char server_url_global[256] = {0};
static char auth_header[128] = {0};

// 修改 boundary 以匹配 curl 示例
#define BOUNDARY "------------------------d74496d66958873e"

// 响应缓冲区
static char response_buffer[1024];
static int response_len = 0;

// HTTP 事件处理回调
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
            // 直接打印服务器返回的原始数据
            ESP_LOGI(TAG, "Response data: %.*s", evt->data_len, (char*)evt->data);
            
            // 同时也保存到缓冲区用于后续解析
            if (response_len + evt->data_len < sizeof(response_buffer)) {
                memcpy(response_buffer + response_len, evt->data, evt->data_len);
                response_len += evt->data_len;
                response_buffer[response_len] = 0; // 确保字符串结束
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            // 打印完整的原始响应数据
            ESP_LOGI(TAG, "Raw response: %s", response_buffer);
            
            // 解析 JSON 响应
            if (response_len > 0) {
                cJSON *root = cJSON_Parse(response_buffer);
                if (root) {
                    cJSON *error = cJSON_GetObjectItem(root, "error");
                    if (error) {
                        if (error->valueint == 0) {
                            // 上传成功
                            cJSON *img_url = cJSON_GetObjectItem(root, "img_url");
                            if (img_url) {
                                ESP_LOGI(TAG, "Upload successful, image URL: %s", img_url->valuestring);
                            }
                        } else {
                            // 上传失败
                            cJSON *showmsg = cJSON_GetObjectItem(root, "showmsg");
                            if (showmsg) {
                                ESP_LOGE(TAG, "Upload failed: %s", showmsg->valuestring);
                            }
                        }
                    }
                    cJSON_Delete(root);
                } else {
                    ESP_LOGE(TAG, "Failed to parse JSON response");
                }
            }
            // 清空响应缓冲区
            response_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

// 初始化上传模块
esp_err_t img_upload_init(const char *server_url) {
    if (strlen(server_url) >= sizeof(server_url_global)) {
        ESP_LOGE(TAG, "Server URL too long");
        return ESP_ERR_INVALID_ARG;
    }
    strcpy(server_url_global, server_url);
    
    // 设置 Authorization header，与 curl 示例保持一致
    snprintf(auth_header, sizeof(auth_header), 
             "secret {5c627423c152a8717eb659107ba7549c}");
    
    return ESP_OK;
}

// 检查数据是否是有效的JPEG
static bool is_valid_jpeg(const uint8_t *data, size_t len) {
    if (len < 4) return false;
    
    // 检查JPEG文件头 (FFD8)
    if (data[0] != 0xFF || data[1] != 0xD8) return false;
    
    // 检查JPEG文件尾 (FFD9)
    if (data[len-2] != 0xFF || data[len-1] != 0xD9) return false;
    
    return true;
}

// 上传图片数据
esp_err_t img_upload_send(const uint8_t *data, size_t len) {
    if (!data || len == 0) {
        ESP_LOGE(TAG, "Invalid input data");
        return ESP_ERR_INVALID_ARG;
    }

    if (!is_valid_jpeg(data, len)) {
        ESP_LOGE(TAG, "Invalid JPEG format");
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(server_url_global) == 0) {
        ESP_LOGE(TAG, "Server URL not set");
        return ESP_ERR_INVALID_STATE;
    }

    esp_http_client_config_t config = {
        .url = server_url_global,
        .event_handler = _http_event_handler,
        .max_redirection_count = 5,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_FAIL;

    // 设置请求头，完全匹配 curl 示例的格式
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", 
        "multipart/form-data; boundary=" BOUNDARY);
    esp_http_client_set_header(client, "Expect", "");
    
    // 构建 multipart 请求体
    const char *header_format = 
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"upload\"; filename=\"image.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n"
        "\r\n";

    char header[512];
    int header_len = snprintf(header, sizeof(header), header_format, BOUNDARY);

    char footer[64];
    int footer_len = snprintf(footer, sizeof(footer), "\r\n--%s--\r\n", BOUNDARY);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    
    // 计算总长度并打开连接
    int total_len = header_len + len + footer_len;
    esp_err_t err = esp_http_client_open(client, total_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open connection: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // 发送请求头部
    int written = esp_http_client_write(client, header, header_len);
    if (written != header_len) {
        ESP_LOGE(TAG, "Failed to write header");
        err = ESP_FAIL;
        goto cleanup;
    }

    // 发送图片数据
    written = esp_http_client_write(client, (const char*)data, len);
    if (written != len) {
        ESP_LOGE(TAG, "Failed to write image data");
        err = ESP_FAIL;
        goto cleanup;
    }

    // 发送结尾
    written = esp_http_client_write(client, footer, footer_len);
    if (written != footer_len) {
        ESP_LOGE(TAG, "Failed to write footer");
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

// 用于测试的有效JPEG数据
static const uint8_t valid_jpeg_data[] = {

    
0xFF,
0xD8,
0xFF,
0xE0,
0x00,
0x10,
0x4A,
0x46,
0x49,
0x46,
0x00,
0x01,

    
0x01,
0x00,
0x00,
0x01,
0x00,
0x01,
0x00,
0x00,
0xFF,
0xE1,
0x00,
0x62,

    
0x45,
0x78,
0x69,
0x66,
0x00,
0x00,
0x4D,
0x4D,
0x00,
0x2A,
0x00,
0x00,

    
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,

    
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,

    
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x7F,
0xFF,
0xD9,

};

// 图片上传任务
void img_upload_task(void *pvParameters) {
    while (1) {
        esp_err_t ret = img_upload_send(valid_jpeg_data, sizeof(valid_jpeg_data));
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Upload successful");
        } else {
            ESP_LOGE(TAG, "Upload failed");
        }
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}