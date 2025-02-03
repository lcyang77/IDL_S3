#ifndef UART_CONFIG_H
#define UART_CONFIG_H

#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_err.h"

// UART 配置参数
#define UART_NUM            UART_NUM_0
#define UART_TX_GPIO        43  // 根据硬件情况修改
#define UART_RX_GPIO        44  // 根据硬件情况修改
#define UART_BUFFER_SIZE    1024
#define UART_QUEUE_SIZE     20

/**
 * @brief 初始化UART配置
 * 
 * @return esp_err_t ESP_OK 成功，其他错误码失败
 */
esp_err_t uart_config_init(void);

/**
 * @brief 获取UART事件队列
 * 
 * @return QueueHandle_t UART事件队列句柄
 */
QueueHandle_t uart_config_get_queue(void);

#endif // UART_CONFIG_H
