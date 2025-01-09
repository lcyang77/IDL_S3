#ifndef NET_UART_COMM_H
#define NET_UART_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// ---- 协议指令枚举 ----
typedef enum {
    CMD_WIFI_CONFIG           = 0x01, // WiFi配置
    CMD_WIFI_RESPONSE         = 0x02, // WiFi配置应答
    CMD_NETWORK_STATUS        = 0x23, // 联网状态通知
    CMD_EXIT_CONFIG           = 0x1A, // 配网退出
    CMD_EXIT_CONFIG_ACK       = 0x1B, // 配网退出应答令
    CMD_GET_NETWORK_TIME      = 0x10, // MCU->WiFi 获取网络时间
    CMD_GET_NETWORK_TIME_RSP  = 0x11, // WiFi->MCU 网络时间应答
    CMD_GET_DEVICE_INFO       = 0x06, // MCU->WiFi 获取设备信息 
    CMD_GET_DEVICE_INFO_RSP   = 0x07, // WiFi->MCU 设备信息应答
    CMD_CLEAR_DATA            = 0x05, // 清除数据，恢复出厂设置
} uart_command_t;

// WiFi配置结果状态
typedef enum {
    WIFI_CONFIG_SUCCESS       = 0x00, // 配网成功
    WIFI_CONFIG_TIMEOUT       = 0x01, // 配网超时
    WIFI_CONFIG_FAILED        = 0x02, // 配网失败
} wifi_config_status_t;

// 设备类型
typedef enum {
    DEVICE_TYPE_NORMAL        = 0x00,
    DEVICE_TYPE_MANAGER       = 0x02,
    DEVICE_TYPE_MANAGER_NO_BTN= 0x03,
} device_type_t;

// ---- 数据包结构 ----
typedef struct {
    uint8_t header[2]; // 0xAA, 0x55
    uint8_t command;   // 指令
    uint8_t data[6];   // 数据区(6字节)
    uint8_t checksum;  // 校验和
} uart_packet_t;

// ---- 设备信息数据包结构 ---- 
typedef struct {
    uint8_t header[2];     // 0xAA, 0x55
    uint8_t command;       // 指令 
    uint8_t device_id[12]; // 设备ID
    uint8_t mac[6];        // MAC地址
    uint8_t reserved[6];   // 保留字节
    uint8_t checksum;      // 校验和
} uart_device_info_packet_t;

// ---- 回调函数类型 ----
typedef void (*uart_packet_callback_t)(const uart_packet_t *packet);

// ---- 对外接口函数 ----
esp_err_t uart_comm_init(void);
esp_err_t uart_comm_register_callback(uart_packet_callback_t callback);

// 计算校验和
uint8_t uart_comm_calc_checksum(const uint8_t *data, size_t length);

// 发送数据包
esp_err_t uart_comm_send_packet(const uart_packet_t *packet);

// 发送WiFi配置响应
esp_err_t uart_comm_send_wifi_response(wifi_config_status_t status);

// 发送网络状态通知（保留原有实现）
esp_err_t uart_comm_send_network_status(bool connected);

// 发送配网退出应答
esp_err_t uart_comm_send_exit_config_ack(void);

// 发送网络时间应答
esp_err_t uart_comm_send_network_time(uint32_t utc_time_sec, int8_t timezone_15min);

// 发送设备信息
esp_err_t uart_comm_send_device_info(const char *device_id, const uint8_t *mac);

// 新增：发送清除数据响应
esp_err_t uart_comm_send_clear_data_response(bool success);

#endif // NET_UART_COMM_H
