#include "net_uart_comm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include "gs_device.h"  // 添加设备信息相关头文件
#include "cc_hal_wifi.h"
#include "cc_event_reset.h"

// 为了获取缓存时间,需要包含 get_time.h
#include "get_time.h"

// 引入必要的头文件以支持 NVS 和系统重启
#include "nvs_flash.h"
#include "esp_system.h"

static const char *TAG = "uart_comm";

// ====== UART 相关配置 ======
#define UART_NUM            UART_NUM_0
#define UART_TX_GPIO        43  // 请根据硬件情况修改
#define UART_RX_GPIO        44  // 请根据硬件情况修改
#define UART_BUFFER_SIZE    1024
#define UART_QUEUE_SIZE     20

// UART事件队列
static QueueHandle_t uart_event_queue = NULL;
// UART事件任务
static TaskHandle_t  uart_task_handle = NULL;

// 数据包回调
static uart_packet_callback_t s_packet_callback = NULL;

// 清除数据互斥锁
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
    // 填充其他数据为0x00
    for (int i = 1; i < 6; i++) {
        packet.data[i] = 0x00;
    }

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
 */
esp_err_t uart_comm_send_network_time(uint32_t utc_time_sec, int8_t timezone_15min)
{
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
 * @brief 发送设备信息应答
 */
esp_err_t uart_comm_send_device_info(const char *device_id, const uint8_t *mac)
{
    if (!device_id || !mac) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    uart_device_info_packet_t packet = {
        .header = {0xAA, 0x55},
        .command = CMD_GET_DEVICE_INFO_RSP,
        .device_id = {0}, // 初始化为0
        .mac = {0},
        .reserved = {0},
        .checksum = 0,
    };

    // 确保设备ID长度不超过12字节，并复制到数据包
    size_t device_id_len = strlen(device_id);
    if (device_id_len > 12) {
        device_id_len = 12;
        ESP_LOGW(TAG, "Device ID length exceeds 12 bytes, truncating");
    }
    memcpy(packet.device_id, device_id, device_id_len);
    if (device_id_len < 12) {
        memset(packet.device_id + device_id_len, 0x00, 12 - device_id_len); // 填充0x00
    }

    // 复制MAC地址(6字节) 
    memcpy(packet.mac, mac, 6);
    // 保留字节清零
    memset(packet.reserved, 0, sizeof(packet.reserved));

    // 计算校验和
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, 
                     sizeof(uart_device_info_packet_t) - 1);

    ESP_LOGI(TAG, "Sending device info response, ID=%s, MAC=%02X:%02X:%02X:%02X:%02X:%02X",
             device_id, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // 打印完整的UART数据（十六进制）
    print_raw_data("UART Data Sent", (uint8_t *)&packet, sizeof(uart_device_info_packet_t));

    // 写入UART
    int tx_len = sizeof(uart_device_info_packet_t);
    int ret = uart_write_bytes(UART_NUM, (const char *)&packet, tx_len);
    if (ret < 0) {
        ESP_LOGE(TAG, "uart_write_bytes failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Successfully sent %d bytes", ret);
    return ESP_OK;
}

/**
 * @brief 发送清除数据响应
 * @param success 执行结果，true 表示成功，false 表示失败
 * @return ESP_OK 成功，其他错误码失败
 */
esp_err_t uart_comm_send_clear_data_response(bool success)
{
    uart_packet_t packet = {
        .header  = {0xAA, 0x55},
        .command = CMD_WIFI_RESPONSE, // 使用已有的WiFi响应命令
        .data    = {0},
    };

    if (success) {
        packet.data[0] = 0x00; // 执行成功
    } else {
        packet.data[0] = 0x02; // 执行失败
    }

    // 填充其他数据为0x00
    for (int i = 1; i < 6; i++) {
        packet.data[i] = 0x00;
    }

    // 计算校验和
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Sending CMD_WIFI_RESPONSE (0x02) for CMD_CLEAR_DATA, success=%d", success);
    return uart_comm_send_packet(&packet);
}

/**
 * @brief 处理清除数据命令
 * @return ESP_OK 成功，其他错误码失败
 */
static esp_err_t uart_comm_handle_clear_data(void)
{
    ESP_LOGI(TAG, "Handling CMD_CLEAR_DATA: Clearing NVS and resetting state");

    // 获取互斥锁以防止并发清除操作
    if (xSemaphoreTake(clear_data_mutex, (TickType_t)10) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take clear_data_mutex");
        return ESP_FAIL;
    }

    // 1. 完全断开 WiFi 连接
    ESP_LOGI(TAG, "Disconnecting WiFi...");
    esp_err_t ret = cc_hal_wifi_sta_disconnect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disconnect WiFi: %d", ret);
        xSemaphoreGive(clear_data_mutex);
        return ret;
    }

    // 2. 清除 NVS
    ESP_LOGI(TAG, "Erasing NVS...");
    ret = nvs_flash_erase();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase NVS: %d", ret);
        xSemaphoreGive(clear_data_mutex);
        return ret;
    }

    // 3. 重新初始化 NVS
    ESP_LOGI(TAG, "Re-initializing NVS...");
    ret = nvs_flash_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to re-initialize NVS: %d", ret);
        xSemaphoreGive(clear_data_mutex);
        return ret;
    }

    // 4. 重置状态机（假设有相应的函数）
    ESP_LOGI(TAG, "Resetting state machine...");
    cc_event_reset(); // 假设有这个函数

    // 5. 发送成功响应
    ret = uart_comm_send_clear_data_response(true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send clear data success response");
        // 尽管发送失败，但继续重启
    }

    ESP_LOGI(TAG, "Clear data completed, restarting device...");

    // 6. 重启设备以应用更改
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    // 释放互斥锁（虽然设备即将重启）
    xSemaphoreGive(clear_data_mutex);

    return ESP_OK;
}

/**
 * @brief 解析完数据包后调用的回调
 */
static void uart_packet_received_internal(const uart_packet_t *packet)
{
    ESP_LOGI(TAG, "uart_packet_received_internal: CMD=0x%02X", packet->command);

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

    case CMD_GET_DEVICE_INFO: {
        ESP_LOGI(TAG, "Got CMD_GET_DEVICE_INFO (0x06)");

        char device_id[13] = {0}; // 12字节ID + 1字节结束符
        uint8_t mac[6] = {0};

        // 获取设备ID
        if (gs_device_get_product_key(device_id) != CC_OK) {
            ESP_LOGE(TAG, "Failed to get device ID");
            return;
        }

        // 获取MAC地址
        if (cl_hal_wifi_sta_get_mac(mac) != CC_OK) { // 修改函数名为 cl_hal_wifi_sta_get_mac
            ESP_LOGE(TAG, "Failed to get WiFi MAC");
            return;
        }

        ESP_LOGI(TAG, "Retrieved Device ID: %s", device_id);
        ESP_LOGI(TAG, "Retrieved MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        // 发送应答
        esp_err_t ret = uart_comm_send_device_info(device_id, mac);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send device info");
        }
        break;
    }

    case CMD_CLEAR_DATA: {
        ESP_LOGI(TAG, "Got CMD_CLEAR_DATA (0x05)");
        // 调用清除数据的处理函数
        esp_err_t ret = uart_comm_handle_clear_data();
        if (ret == ESP_OK) {
            // 已在 handle_clear_data 中发送响应
        } else {
            // 发送失败响应
            uart_comm_send_clear_data_response(false);
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
 * @brief 初始化清除数据互斥锁
 */
static void init_clear_data_mutex(void)
{
    clear_data_mutex = xSemaphoreCreateMutex();
    if (clear_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create clear_data_mutex");
    } else {
        ESP_LOGI(TAG, "clear_data_mutex created successfully");
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

   // 初始化清除数据互斥锁
   init_clear_data_mutex();

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
