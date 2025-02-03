#include "uart_config.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "uart_config";

// UART事件队列句柄
static QueueHandle_t uart_event_queue_handle = NULL;

esp_err_t uart_config_init(void)
{
    // 配置 UART 参数
    uart_config_t uart_config = {
        .baud_rate = 9600,  // 根据需求设置波特率
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    esp_err_t ret = uart_param_config(UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %d", ret);
        return ret;
    }

    // 设置 UART 引脚
    ret = uart_set_pin(UART_NUM, UART_TX_GPIO, UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %d", ret);
        return ret;
    }

    // 安装 UART 驱动
    ret = uart_driver_install(UART_NUM, UART_BUFFER_SIZE, UART_BUFFER_SIZE, UART_QUEUE_SIZE, &uart_event_queue_handle, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "UART configured successfully on port %d", UART_NUM);
    return ESP_OK;
}

QueueHandle_t uart_config_get_queue(void)
{
    return uart_event_queue_handle;
}
