#include "img_transfer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uvc_camera.h"    // 提供 esp_camera_fb_get()/esp_camera_fb_return() 接口
#include "img_upload.h"    // 提供 img_upload_send() 接口
#include "cc_hal_sys.h"    // 用于获取系统时间（判断超时）
#include "net_uart_comm.h"

static const char *TAG = "img_transfer";

// 定义图传任务超时（单位：毫秒）
#define IMG_TRANSFER_TIMEOUT_MS    3000

// 内部状态变量：记录图传是否已开启（后续可以用于禁止主动采集）
static bool s_img_transfer_enabled = false;

/**
 * @brief 计算数据缓冲区的简单 16 位累加校验和
 *
 * 累加所有字节后取低 16 位（低字节在前），用于图传结果数据包中“图片校验和”的填充。
 */
static uint16_t calc_data_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint16_t)(sum & 0xFFFF);
}

/**
 * @brief 内部：构造并发送图传结果数据包（命令 0x27）
 *
 * 数据格式：
 *  - data[0]：图传结果（0x00 成功，0x01 上传/校验失败，0x02 超时）
 *  - data[1-2]：图片大小（低字节在前）
 *  - data[3-4]：图片校验和（低字节在前）
 *  - data[5]：保留，填 0x00
 */
static void send_img_transfer_result(uint8_t result_code, uint16_t img_size, uint16_t img_checksum)
{
    uart_packet_t packet = {0};
    packet.header[0] = 0xAA;
    packet.header[1] = 0x55;
    packet.command   = 0x27;
    packet.data[0] = result_code;
    packet.data[1] = (uint8_t)(img_size & 0xFF);
    packet.data[2] = (uint8_t)((img_size >> 8) & 0xFF);
    packet.data[3] = (uint8_t)(img_checksum & 0xFF);
    packet.data[4] = (uint8_t)((img_checksum >> 8) & 0xFF);
    packet.data[5] = 0x00;
    packet.checksum = uart_comm_calc_checksum((uint8_t *)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Sending img transfer result: result=0x%02X, size=%u, checksum=0x%04X", result_code, img_size, img_checksum);
    esp_err_t err = uart_comm_send_packet(&packet);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send img transfer result, err=0x%x", err);
    }
}

/**
 * @brief 图传处理任务
 *
 * 任务步骤：
 *   1. 调用摄像头接口采集一帧图片；
 *   2. 计算图片大小与校验和；
 *   3. 调用 img_upload_send() 接口上传图片数据；
 *   4. 判断采集上传过程是否超时（超过 IMG_TRANSFER_TIMEOUT_MS 则视为超时），
 *      并最终发送图传结果数据包（命令 0x27）。
 */
static void img_transfer_task(void *arg)
{
    TickType_t start_tick = xTaskGetTickCount();

    // 采集一帧 JPEG 图片（内部采用同步方式）
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Failed to capture image from camera");
        send_img_transfer_result(0x02, 0, 0);  // 超时或采集失败
        vTaskDelete(NULL);
        return;
    }

    uint16_t img_size     = (uint16_t)fb->len;
    uint16_t img_checksum = calc_data_checksum((const uint8_t *)fb->buf, fb->len);
    ESP_LOGI(TAG, "Captured image: size=%u bytes, checksum=0x%04X", fb->len, img_checksum);

    // 上传图片数据
    esp_err_t ret = img_upload_send((const uint8_t *)fb->buf, fb->len);
    // 释放采集的帧
    esp_camera_fb_return(fb);

    uint8_t result_code = 0x00;
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Image upload failed, ret=0x%x", ret);
        result_code = 0x01;
    }

    // 判断采集上传是否超时
    TickType_t elapsed = xTaskGetTickCount() - start_tick;
    if (elapsed > pdMS_TO_TICKS(IMG_TRANSFER_TIMEOUT_MS)) {
        ESP_LOGW(TAG, "Image transfer timeout: elapsed %u ms", (unsigned)(elapsed * portTICK_PERIOD_MS));
        result_code = 0x02;
    }

    // 发送图传结果数据包（命令 0x27）
    send_img_transfer_result(result_code, img_size, img_checksum);
    vTaskDelete(NULL);
}

/**
 * @brief 处理 MCU 发送的图传设置命令（命令 0x1C）
 *
 * 根据数据区 data[0] 判断：
 *   - 0x00 表示开启图传：回复应答后异步启动图传任务；
 *   - 0x01 表示关闭图传：仅回复应答并置 s_img_transfer_enabled 为 false。
 */
void img_transfer_handle_uart_packet(const uart_packet_t *packet)
{
    if (!packet) {
        return;
    }

    uint8_t mode = packet->data[0];
    ESP_LOGI(TAG, "Handling img transfer command: mode=0x%02X", mode);

    // 构造并发送图传设置应答数据包（命令 0x1D），数据内容与请求一致，其余字节补 0
    uart_packet_t ack_packet = {0};
    ack_packet.header[0] = 0xAA;
    ack_packet.header[1] = 0x55;
    ack_packet.command   = 0x1D;
    ack_packet.data[0] = mode;
    for (int i = 1; i < 6; i++) {
        ack_packet.data[i] = 0x00;
    }
    ack_packet.checksum = uart_comm_calc_checksum((uint8_t *)&ack_packet, sizeof(uart_packet_t) - 1);
    esp_err_t err = uart_comm_send_packet(&ack_packet);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send img transfer ack (0x1D), err=0x%x", err);
    } else {
        ESP_LOGI(TAG, "Sent img transfer ack (0x1D)");
    }

    if (mode == 0x00) { // 开启图传
        s_img_transfer_enabled = true;
        if (xTaskCreate(img_transfer_task, "img_transfer_task", 4096, NULL, 5, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Failed to create img_transfer_task");
        }
    } else if (mode == 0x01) { // 关闭图传
        s_img_transfer_enabled = false;
        ESP_LOGI(TAG, "Image transfer disabled by command");
    } else {
        ESP_LOGW(TAG, "Unknown mode 0x%02X in img transfer command", mode);
    }
}

/**
 * @brief 初始化 img_transfer 模块
 */
esp_err_t img_transfer_init(void)
{
    ESP_LOGI(TAG, "img_transfer module initialized");
    s_img_transfer_enabled = false;
    return ESP_OK;
}
