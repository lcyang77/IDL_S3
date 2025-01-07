/************************************************
 * File: uart_comm.c
 ************************************************/
#include "uart_comm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

// 为了获取缓存时间，需要包含 get_time.h
#include "get_time.h"

static const char *TAG = "uart_comm";

// ====== UART 相关配置 ======
#define UART_NUM            UART_NUM_0
#define UART_TX_GPIO        43 // 请根据硬件情况修改
#define UART_RX_GPIO        44 // 请根据硬件情况修改
#define UART_BUFFER_SIZE    1024
#define UART_QUEUE_SIZE     20

// UART事件队列
static QueueHandle_t uart_event_queue = NULL;
// UART事件任务
static TaskHandle_t  uart_task_handle = NULL;

// 数据包回调
static uart_packet_callback_t s_packet_callback = NULL;

/**
 * @brief 调试打印原始数据
 */
static void print_raw_data(const char* prefix, const uint8_t* data, size_t len)
{
    char buffer[256];
    int offset = 0;
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s: ", prefix);
    for (size_t i = 0; i < len && offset < (int)sizeof(buffer) - 3; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%02X ", data[i]);
    }
    ESP_LOGI(TAG, "%s", buffer);
}

/**
 * @brief 打印数据包详细信息
 */
static void print_packet_details(const uart_packet_t *packet, const char* prefix)
{
    ESP_LOGI(TAG, "====== %s Packet Details ======", prefix);
    ESP_LOGI(TAG, "Header: 0x%02X 0x%02X", packet->header[0], packet->header[1]);
    ESP_LOGI(TAG, "Command: 0x%02X", packet->command);
    ESP_LOGI(TAG, "Data: %02X %02X %02X %02X %02X %02X",
             packet->data[0], packet->data[1], packet->data[2],
             packet->data[3], packet->data[4], packet->data[5]);
    ESP_LOGI(TAG, "Checksum: 0x%02X", packet->checksum);
    ESP_LOGI(TAG, "==============================");
}

/**
 * @brief 计算校验和：简单求和取低8位
 */
uint8_t uart_comm_calc_checksum(const uint8_t *data, size_t length)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

/**
 * @brief 发送数据包
 */
esp_err_t uart_comm_send_packet(const uart_packet_t *packet)
{
    if (!packet) {
        ESP_LOGE(TAG, "Invalid null packet pointer");
        return ESP_ERR_INVALID_ARG;
    }

    // 调试打印
    print_packet_details(packet, "Sending");

    // 写UART
    int tx_len = sizeof(uart_packet_t);
    int ret = uart_write_bytes(UART_NUM, (const char *)packet, tx_len);
    if (ret < 0) {
        ESP_LOGE(TAG, "uart_write_bytes failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Successfully sent %d bytes", ret);
    return ESP_OK;
}

/**
 * @brief 发送 WiFi 配置响应
 */
esp_err_t uart_comm_send_wifi_response(wifi_config_status_t status)
{
    uart_packet_t packet = {
        .header  = {0xAA, 0x55},
        .command = CMD_WIFI_RESPONSE,
        .data    = {0},
    };

    // data[0] 放 status
    packet.data[0] = (uint8_t)status;
    // data[1] 放协议版本或其它信息，演示填0x01
    packet.data[1] = 0x01;

    // 计算校验和
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Sending WiFi configuration response, status=0x%02X", status);
    return uart_comm_send_packet(&packet);
}

/**
 * @brief 发送配网退出应答
 */
esp_err_t uart_comm_send_exit_config_ack(void)
{
    uart_packet_t packet = {
        .header  = {0xAA, 0x55},
        .command = CMD_EXIT_CONFIG_ACK,
        .data    = {0},
    };

    // data区全0
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Sending Exit Config ACK (0x1B)");
    return uart_comm_send_packet(&packet);
}

/**
 * @brief 发送网络状态通知（保留原有实现）
 *
 * 此函数发送的仅是联网状态（1B），不包含UTC时间和时区值。
 * 新的联网状态通知由 net_sta 模块负责发送。
 */
esp_err_t uart_comm_send_network_status(bool connected)
{
    uart_packet_t packet = {
        .header  = {0xAA, 0x55},
        .command = CMD_NETWORK_STATUS,
        .data    = {0},
    };

    // 根据连接状态设置联网状态值
    packet.data[0] = connected ? 0x04 : 0x03; // 示例：已连接云服务器或仅连接路由器

    // 获取UTC时间和时区
    uint32_t utc_time = get_time_get_utc();
    int8_t timezone = get_time_get_timezone();

    // 填充UTC时间（小端）
    packet.data[1] = (uint8_t)(utc_time & 0xFF);
    packet.data[2] = (uint8_t)((utc_time >> 8) & 0xFF);
    packet.data[3] = (uint8_t)((utc_time >> 16) & 0xFF);
    packet.data[4] = (uint8_t)((utc_time >> 24) & 0xFF);

    // 填充时区值
    packet.data[5] = (uint8_t)timezone;

    // 计算校验和
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Sending network status, connected=%d", connected);
    return uart_comm_send_packet(&packet);
}

/**
 * @brief 发送网络时间应答
 *
 * @param utc_time_sec   当前UTC秒数(自1970-1-1起)
 * @param timezone_15min 时区(小时*10 + 15min刻数)
 */
esp_err_t uart_comm_send_network_time(uint32_t utc_time_sec, int8_t timezone_15min)
{
    /*
     * 协议要求：
     * CMD_GET_NETWORK_TIME_RSP (0x11)
     *   data[0..3] = UTC时间秒数(小端)
     *   data[4]    = 时区(1B)
     *   data[5]    = 保留
     */
    uart_packet_t packet = {
        .header  = {0xAA, 0x55},
        .command = CMD_GET_NETWORK_TIME_RSP,
        .data    = {0},
    };

    packet.data[0] = (uint8_t)(utc_time_sec & 0xFF);
    packet.data[1] = (uint8_t)((utc_time_sec >> 8) & 0xFF);
    packet.data[2] = (uint8_t)((utc_time_sec >> 16) & 0xFF);
    packet.data[3] = (uint8_t)((utc_time_sec >> 24) & 0xFF);

    packet.data[4] = (uint8_t)timezone_15min;
    // data[5] = 0

    packet.checksum = uart_comm_calc_checksum((uint8_t *)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Send CMD=0x11, UTC=%u, TZ=%d", (unsigned)utc_time_sec, (int)timezone_15min);
    return uart_comm_send_packet(&packet);
}

/**
 * @brief 解析完数据包后调用的回调
 */
static void uart_packet_received_internal(const uart_packet_t *packet)
{
    ESP_LOGI(TAG, "uart_packet_received: CMD=0x%02X", packet->command);

    switch (packet->command) {
    case CMD_WIFI_CONFIG: {
        ESP_LOGI(TAG, "Got CMD_WIFI_CONFIG (0x01)");
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    }

    case CMD_EXIT_CONFIG: {
        ESP_LOGI(TAG, "Got CMD_EXIT_CONFIG (0x1A)");
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    }

    case CMD_NETWORK_STATUS: {
        ESP_LOGI(TAG, "Got CMD_NETWORK_STATUS (0x23)");
        // 根据协议，此指令为WiFi->MCU主动发送的联网状态通知
        // 可根据需要处理或忽略
        // 示例：解析数据内容
        if (packet->data[0] == 0x01) {
            ESP_LOGI(TAG, "Network Status: Not Configured");
        } else if (packet->data[0] == 0x02) {
            ESP_LOGI(TAG, "Network Status: Connecting to Router/Base Station");
        } else if (packet->data[0] == 0x03) {
            ESP_LOGI(TAG, "Network Status: Connected to Router/Base Station");
        } else if (packet->data[0] == 0x04) {
            ESP_LOGI(TAG, "Network Status: Connected to Cloud Server");
        } else {
            ESP_LOGW(TAG, "Unknown Network Status: 0x%02X", packet->data[0]);
        }

        // 解析UTC时间和时区
        uint32_t utc_time_sec = packet->data[1] |
                                 (packet->data[2] << 8) |
                                 (packet->data[3] << 16) |
                                 (packet->data[4] << 24);
        int8_t timezone_15min = (int8_t)packet->data[5];

        ESP_LOGI(TAG, "Received UTC Time: %u, Timezone: %d", utc_time_sec, timezone_15min);

        // 根据协议，此处可用于校准时间等操作

        break;
    }

    case CMD_GET_NETWORK_TIME: {
        ESP_LOGI(TAG, "Got CMD_GET_NETWORK_TIME (0x10)");

        // 这里直接从 get_time 模块获取缓存值并发送应答
        uint32_t utc_sec  = get_time_get_utc();
        int8_t   tz_15min = get_time_get_timezone();
        ESP_LOGI(TAG, "Now cached time: UTC=%u, TimeZone=%d", (unsigned)utc_sec, (int)tz_15min);

        esp_err_t ret = uart_comm_send_network_time(utc_sec, tz_15min);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send network time");
        }
        break;
    }

    default:
        ESP_LOGW(TAG, "Unknown cmd=0x%02X", packet->command);
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    }
}

/**
 * @brief UART事件任务：读取硬件队列事件并解析为数据包
 */
static void uart_event_task(void *arg)
{
    uart_event_t event;
    uint8_t data_buffer[UART_BUFFER_SIZE];
    int state = 0;
    uart_packet_t packet;
    int len = 0;

    ESP_LOGI(TAG, "UART event task started");

    while (1) {
        if (xQueueReceive(uart_event_queue, &event, portMAX_DELAY)) {
            switch (event.type) {
            case UART_DATA:
                // 读取串口数据
                len = uart_read_bytes(UART_NUM, data_buffer, event.size, portMAX_DELAY);
                if (len > 0) {
                    print_raw_data("Raw data", data_buffer, len);

                    // 使用简易状态机逐字节解析
                    for (int i = 0; i < len; i++) {
                        uint8_t b = data_buffer[i];
                        switch (state) {
                        case 0: // 等待帧头0xAA
                            if (b == 0xAA) {
                                packet.header[0] = b;
                                state = 1;
                            }
                            break;
                        case 1: // 等待帧头0x55
                            if (b == 0x55) {
                                packet.header[1] = b;
                                state = 2;
                            } else {
                                state = 0;
                            }
                            break;
                        case 2: // 接收command
                            packet.command = b;
                            state = 3;
                            break;
                        case 3: // data[0]
                        case 4: // data[1]
                        case 5: // data[2]
                        case 6: // data[3]
                        case 7: // data[4]
                        case 8: // data[5]
                            packet.data[state - 3] = b;
                            state++;
                            break;
                        case 9: // checksum
                            packet.checksum = b;
                            {
                                uint8_t calc = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);
                                if (calc == packet.checksum) {
                                    ESP_LOGI(TAG, "Received valid packet");
                                    print_packet_details(&packet, "Received");
                                    // 调用内部回调分发
                                    uart_packet_received_internal(&packet);
                                } else {
                                    ESP_LOGE(TAG, "Checksum mismatch: calc=0x%02X, recv=0x%02X", calc, packet.checksum);
                                }
                            }
                            state = 0;
                            break;
                        default:
                            state = 0;
                            break;
                        }
                    }
                }
                break;

            case UART_FIFO_OVF:
                ESP_LOGE(TAG, "HW FIFO overflow");
                uart_flush_input(UART_NUM);
                xQueueReset(uart_event_queue);
                break;

            case UART_BUFFER_FULL:
                ESP_LOGE(TAG, "Ring buffer full");
                uart_flush_input(UART_NUM);
                xQueueReset(uart_event_queue);
                break;

            case UART_BREAK:
                ESP_LOGW(TAG, "UART Break");
                break;

            case UART_PARITY_ERR:
                ESP_LOGW(TAG, "UART Parity Error");
                break;

            case UART_FRAME_ERR:
                ESP_LOGW(TAG, "UART Frame Error");
                break;

            default:
                ESP_LOGW(TAG, "Unhandled UART event type: %d", event.type);
                break;
            }
        }
    }
}

/**
 * @brief 初始化 UART
 */
esp_err_t uart_comm_init(void)
{
    ESP_LOGI(TAG, "Initializing UART communication");

    uart_config_t uart_config = {
        .baud_rate  = 9600,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };

    esp_err_t ret = uart_param_config(UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %d", ret);
        return ret;
    }

    ret = uart_set_pin(UART_NUM, UART_TX_GPIO, UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %d", ret);
        return ret;
    }

    // 安装驱动
    ret = uart_driver_install(UART_NUM, UART_BUFFER_SIZE, UART_BUFFER_SIZE,
                              UART_QUEUE_SIZE, &uart_event_queue, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %d", ret);
        return ret;
    }

    // 创建事件任务
    BaseType_t xRet = xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, &uart_task_handle);
    if (xRet != pdPASS) {
        ESP_LOGE(TAG, "Failed to create uart_event_task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "UART communication initialized successfully");
    return ESP_OK;
}

/**
 * @brief 注册数据包回调
 */
esp_err_t uart_comm_register_callback(uart_packet_callback_t callback)
{
    s_packet_callback = callback;
    return ESP_OK;
}
