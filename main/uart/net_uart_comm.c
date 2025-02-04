#include "net_uart_comm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include "gs_device.h"    // 设备信息相关
#include "cc_hal_wifi.h"
#include "cc_event_reset.h"
#include "get_time.h"
#include "uart_config.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "state_report.h"  // 为了使用 CMD_STATE_REPORT 定义

static const char *TAG = "uart_comm";

// UART 事件队列句柄
static QueueHandle_t uart_event_queue = NULL;
// UART 事件任务句柄
static TaskHandle_t  uart_task_handle = NULL;

// 注册的数据包回调函数
static uart_packet_callback_t s_packet_callback = NULL;

// 清除数据互斥锁（用于 CMD_CLEAR_DATA 处理）
static SemaphoreHandle_t clear_data_mutex = NULL;

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

uint8_t uart_comm_calc_checksum(const uint8_t *data, size_t length)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

esp_err_t uart_comm_send_packet(const uart_packet_t *packet)
{
    if (!packet) {
        ESP_LOGE(TAG, "Invalid null packet pointer");
        return ESP_ERR_INVALID_ARG;
    }

    print_packet_details(packet, "Sending");

    int tx_len = sizeof(uart_packet_t);
    int ret = uart_write_bytes(UART_NUM, (const char *)packet, tx_len);
    if (ret < 0) {
        ESP_LOGE(TAG, "uart_write_bytes failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Successfully sent %d bytes", ret);
    return ESP_OK;
}

esp_err_t uart_comm_send_wifi_response(wifi_config_status_t status)
{
    uart_packet_t packet = {0};
    packet.header[0] = 0xAA;
    packet.header[1] = 0x55;
    packet.command = CMD_WIFI_RESPONSE;
    packet.data[0] = (uint8_t)status;
    for (int i = 1; i < 6; i++) {
        packet.data[i] = 0x00;
    }
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);
    ESP_LOGI(TAG, "Sending WiFi configuration response, status=0x%02X", status);
    return uart_comm_send_packet(&packet);
}

esp_err_t uart_comm_send_exit_config_ack(void)
{
    uart_packet_t packet = {0};
    packet.header[0] = 0xAA;
    packet.header[1] = 0x55;
    packet.command = CMD_EXIT_CONFIG_ACK;
    memset(packet.data, 0, sizeof(packet.data));
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);
    ESP_LOGI(TAG, "Sending Exit Config ACK (0x1B)");
    return uart_comm_send_packet(&packet);
}

esp_err_t uart_comm_send_network_status(bool connected)
{
    uart_packet_t packet = {0};
    packet.header[0] = 0xAA;
    packet.header[1] = 0x55;
    packet.command = CMD_NETWORK_STATUS;
    packet.data[0] = connected ? 0x04 : 0x03;
    uint32_t utc_time = get_time_get_utc();
    int8_t timezone = get_time_get_timezone();
    packet.data[1] = (uint8_t)(utc_time & 0xFF);
    packet.data[2] = (uint8_t)((utc_time >> 8) & 0xFF);
    packet.data[3] = (uint8_t)((utc_time >> 16) & 0xFF);
    packet.data[4] = (uint8_t)((utc_time >> 24) & 0xFF);
    packet.data[5] = (uint8_t)timezone;
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);
    ESP_LOGI(TAG, "Sending network status, connected=%d", connected);
    return uart_comm_send_packet(&packet);
}

esp_err_t uart_comm_send_network_time(uint32_t utc_time_sec, int8_t timezone_15min)
{
    uart_packet_t packet = {0};
    packet.header[0] = 0xAA;
    packet.header[1] = 0x55;
    packet.command = CMD_GET_NETWORK_TIME_RSP;
    packet.data[0] = (uint8_t)(utc_time_sec & 0xFF);
    packet.data[1] = (uint8_t)((utc_time_sec >> 8) & 0xFF);
    packet.data[2] = (uint8_t)((utc_time_sec >> 16) & 0xFF);
    packet.data[3] = (uint8_t)((utc_time_sec >> 24) & 0xFF);
    packet.data[4] = (uint8_t)timezone_15min;
    packet.data[5] = 0x00;
    packet.checksum = uart_comm_calc_checksum((uint8_t *)&packet, sizeof(uart_packet_t) - 1);
    ESP_LOGI(TAG, "Send CMD=0x11, UTC=%u, TZ=%d", (unsigned)utc_time_sec, (int)timezone_15min);
    return uart_comm_send_packet(&packet);
}

esp_err_t uart_comm_send_device_info(const char *device_id, const uint8_t *mac)
{
    if (!device_id || !mac) {
        ESP_LOGE(TAG, "Invalid parameters for device info");
        return ESP_ERR_INVALID_ARG;
    }
    uart_device_info_packet_t packet = {0};
    packet.header[0] = 0xAA;
    packet.header[1] = 0x55;
    packet.command = CMD_GET_DEVICE_INFO_RSP;
    memset(packet.device_id, 0, sizeof(packet.device_id));
    size_t len = strlen(device_id);
    if (len > 12) {
        len = 12;
        ESP_LOGW(TAG, "Device ID length exceeds 12 bytes, truncating");
    }
    memcpy(packet.device_id, device_id, len);
    memcpy(packet.mac, mac, 6);
    memset(packet.reserved, 0, sizeof(packet.reserved));
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_device_info_packet_t) - 1);
    ESP_LOGI(TAG, "Sending device info response, ID=%s, MAC=%02X:%02X:%02X:%02X:%02X:%02X",
             device_id, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    print_raw_data("UART Data Sent", (uint8_t *)&packet, sizeof(uart_device_info_packet_t));
    int tx_len = sizeof(uart_device_info_packet_t);
    int ret = uart_write_bytes(UART_NUM, (const char *)&packet, tx_len);
    if (ret < 0) {
        ESP_LOGE(TAG, "uart_write_bytes failed: %d", ret);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Successfully sent %d bytes", ret);
    return ESP_OK;
}

esp_err_t uart_comm_send_clear_data_response(bool success)
{
    uart_packet_t packet = {0};
    packet.header[0] = 0xAA;
    packet.header[1] = 0x55;
    packet.command = CMD_WIFI_RESPONSE; // 使用 WiFi 响应命令
    packet.data[0] = success ? 0x00 : 0x02;
    for (int i = 1; i < 6; i++) {
        packet.data[i] = 0x00;
    }
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);
    ESP_LOGI(TAG, "Sending CMD_WIFI_RESPONSE (0x02) for CMD_CLEAR_DATA, success=%d", success);
    return uart_comm_send_packet(&packet);
}

static esp_err_t uart_comm_handle_clear_data(void)
{
    ESP_LOGI(TAG, "Handling CMD_CLEAR_DATA: Clearing NVS and resetting state");
    if (xSemaphoreTake(clear_data_mutex, (TickType_t)10) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take clear_data_mutex");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Disconnecting WiFi...");
    esp_err_t ret = cc_hal_wifi_sta_disconnect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disconnect WiFi: %d", ret);
        xSemaphoreGive(clear_data_mutex);
        return ret;
    }
    ESP_LOGI(TAG, "Erasing NVS...");
    ret = nvs_flash_erase();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase NVS: %d", ret);
        xSemaphoreGive(clear_data_mutex);
        return ret;
    }
    ESP_LOGI(TAG, "Re-initializing NVS...");
    ret = nvs_flash_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to re-initialize NVS: %d", ret);
        xSemaphoreGive(clear_data_mutex);
        return ret;
    }
    ESP_LOGI(TAG, "Resetting state machine...");
    cc_event_reset();
    ret = uart_comm_send_clear_data_response(true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send clear data success response");
    }
    ESP_LOGI(TAG, "Clear data completed, restarting device...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    xSemaphoreGive(clear_data_mutex);
    return ESP_OK;
}

static void uart_packet_received_internal(const uart_packet_t *packet)
{
    ESP_LOGI(TAG, "uart_packet_received_internal: CMD=0x%02X", packet->command);
    switch (packet->command) {
    case CMD_WIFI_CONFIG:
        ESP_LOGI(TAG, "Got CMD_WIFI_CONFIG (0x01)");
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    case CMD_EXIT_CONFIG:
        ESP_LOGI(TAG, "Got CMD_EXIT_CONFIG (0x1A)");
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    case CMD_NETWORK_STATUS:
        ESP_LOGI(TAG, "Got CMD_NETWORK_STATUS (0x23)");
        {
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
            uint32_t utc_time_sec = packet->data[1] |
                                    (packet->data[2] << 8) |
                                    (packet->data[3] << 16) |
                                    (packet->data[4] << 24);
            int8_t timezone_15min = (int8_t)packet->data[5];
            ESP_LOGI(TAG, "Received UTC Time: %u, Timezone: %d", utc_time_sec, timezone_15min);
        }
        break;
    case CMD_GET_NETWORK_TIME:
        ESP_LOGI(TAG, "Got CMD_GET_NETWORK_TIME (0x10)");
        {
            uint32_t utc_sec  = get_time_get_utc();
            int8_t tz_15min = get_time_get_timezone();
            ESP_LOGI(TAG, "Now cached time: UTC=%u, TimeZone=%d", (unsigned)utc_sec, (int)tz_15min);
            esp_err_t ret = uart_comm_send_network_time(utc_sec, tz_15min);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to send network time");
            }
        }
        break;
    case CMD_GET_DEVICE_INFO:
        ESP_LOGI(TAG, "Got CMD_GET_DEVICE_INFO (0x06)");
        {
            char device_id[13] = {0};
            uint8_t mac[6] = {0};
            if (gs_device_get_product_key(device_id) != CC_OK) {
                ESP_LOGE(TAG, "Failed to get device ID");
                return;
            }
            if (cl_hal_wifi_sta_get_mac(mac) != CC_OK) {
                ESP_LOGE(TAG, "Failed to get WiFi MAC");
                return;
            }
            ESP_LOGI(TAG, "Retrieved Device ID: %s", device_id);
            ESP_LOGI(TAG, "Retrieved MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            esp_err_t ret = uart_comm_send_device_info(device_id, mac);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to send device info");
            }
        }
        break;
    case CMD_CLEAR_DATA:
        ESP_LOGI(TAG, "Got CMD_CLEAR_DATA (0x05)");
        {
            esp_err_t ret = uart_comm_handle_clear_data();
            if (ret != ESP_OK) {
                uart_comm_send_clear_data_response(false);
            }
        }
        break;
    case CMD_IMG_TRANSFER:
        ESP_LOGI(TAG, "Got CMD_IMG_TRANSFER (0x1C)");
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    // 新增：状态上报相关分支
    case CMD_STATE_REPORT:
        ESP_LOGI(TAG, "Got CMD_STATE_REPORT (0x42) => forward to state_report module");
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    case CMD_STATE_REPORT_ACK:
        ESP_LOGI(TAG, "Received CMD_STATE_REPORT_ACK (0x43)");
        /* 当收到状态上报应答时，调用 state_report 模块的ACK处理函数，
           以移除待重传的消息 */
        state_report_ack_handler();
        break;
    default:
        ESP_LOGW(TAG, "Unknown cmd=0x%02X", packet->command);
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    }
}

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
                len = uart_read_bytes(UART_NUM, data_buffer, event.size, portMAX_DELAY);
                if (len > 0) {
                    print_raw_data("Raw data", data_buffer, len);
                    for (int i = 0; i < len; i++) {
                        uint8_t b = data_buffer[i];
                        switch (state) {
                        case 0:
                            if (b == 0xAA) {
                                packet.header[0] = b;
                                state = 1;
                            }
                            break;
                        case 1:
                            if (b == 0x55) {
                                packet.header[1] = b;
                                state = 2;
                            } else {
                                state = 0;
                            }
                            break;
                        case 2:
                            packet.command = b;
                            state = 3;
                            break;
                        case 3: case 4: case 5: case 6: case 7: case 8:
                            packet.data[state - 3] = b;
                            state++;
                            break;
                        case 9:
                            packet.checksum = b;
                            {
                                uint8_t calc = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);
                                if (calc == packet.checksum) {
                                    ESP_LOGI(TAG, "Received valid packet");
                                    print_packet_details(&packet, "Received");
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

static void init_clear_data_mutex(void)
{
    clear_data_mutex = xSemaphoreCreateMutex();
    if (clear_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create clear_data_mutex");
    } else {
        ESP_LOGI(TAG, "clear_data_mutex created successfully");
    }
}

esp_err_t uart_comm_init(void)
{
    ESP_LOGI(TAG, "Initializing UART communication");
    esp_err_t ret = uart_config_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_config_init failed: %d", ret);
        return ret;
    }
    uart_event_queue = uart_config_get_queue();
    if (uart_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to get UART event queue");
        return ESP_FAIL;
    }
    BaseType_t xRet = xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, &uart_task_handle);
    if (xRet != pdPASS) {
        ESP_LOGE(TAG, "Failed to create uart_event_task");
        return ESP_FAIL;
    }
    init_clear_data_mutex();
    ESP_LOGI(TAG, "UART communication initialized successfully");
    return ESP_OK;
}

esp_err_t uart_comm_register_callback(uart_packet_callback_t callback)
{
    s_packet_callback = callback;
    return ESP_OK;
}
