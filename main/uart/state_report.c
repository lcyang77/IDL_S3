#include "state_report.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "cc_hal_sys.h"
#include "get_time.h"

static const char *TAG = "state_report";

/* 重传参数定义 */
#define STATE_REPORT_TIMEOUT_MS         100    // 超时时间100ms
#define STATE_REPORT_MAX_RETRIES        3      // 最大重传次数
#define STATE_REPORT_RETX_TASK_DELAY_MS 50     // 重传任务周期50ms

/* 待重传状态上报项结构体 */
typedef struct state_report_item {
    uint16_t state_type;
    uint32_t state_value;
    uint32_t last_sent_time;    // 上次发送时间（毫秒）
    uint8_t retry_count;        // 已重传次数
    struct state_report_item *next;
} state_report_item_t;

/* 全局变量：待重传状态上报项链表、互斥信号量及重传任务句柄 */
static state_report_item_t *s_pending_list = NULL;
static SemaphoreHandle_t s_state_report_mutex = NULL;
static TaskHandle_t s_state_report_task_handle = NULL;

/* 内部函数声明：重传任务 */
static void state_report_retx_task(void *param);

/* 内部函数：根据状态类型和状态值构造数据包 */
static void create_state_report_packet(uart_packet_t *packet, uint16_t state_type, uint32_t state_value) {
    memset(packet, 0, sizeof(uart_packet_t));
    packet->header[0] = 0xAA;
    packet->header[1] = 0x55;
    packet->command = CMD_STATE_REPORT;
    /* 按小端序填入状态类型 */
    packet->data[0] = (uint8_t)(state_type & 0xFF);
    packet->data[1] = (uint8_t)((state_type >> 8) & 0xFF);
    /* 按小端序填入状态值 */
    packet->data[2] = (uint8_t)(state_value & 0xFF);
    packet->data[3] = (uint8_t)((state_value >> 8) & 0xFF);
    packet->data[4] = (uint8_t)((state_value >> 16) & 0xFF);
    packet->data[5] = (uint8_t)((state_value >> 24) & 0xFF);
    packet->checksum = uart_comm_calc_checksum((uint8_t *)packet, sizeof(uart_packet_t) - 1);
}

/* 内部函数：判断模块是否已联网（此处采用 get_time_get_utc() 不为0作为联网标志） */
static bool is_connected(void) {
    return (get_time_get_utc() != 0);
}

/* 内部函数：将状态上报项加入待重传链表 */
static void add_pending_item(uint16_t state_type, uint32_t state_value) {
    state_report_item_t *item = pvPortMalloc(sizeof(state_report_item_t));
    if (item == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for state report item");
        return;
    }
    item->state_type = state_type;
    item->state_value = state_value;
    item->retry_count = 0;
    item->last_sent_time = cc_hal_sys_get_ms();
    item->next = NULL;

    xSemaphoreTake(s_state_report_mutex, portMAX_DELAY);
    if (s_pending_list == NULL) {
        s_pending_list = item;
    } else {
        state_report_item_t *cur = s_pending_list;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = item;
    }
    xSemaphoreGive(s_state_report_mutex);
}

/* 内部函数：从待重传链表中移除第一个项（认为该项已收到ACK） */
static void remove_first_pending_item(void) {
    xSemaphoreTake(s_state_report_mutex, portMAX_DELAY);
    if (s_pending_list != NULL) {
        state_report_item_t *temp = s_pending_list;
        s_pending_list = s_pending_list->next;
        ESP_LOGI(TAG, "State report acknowledged, removed item: type=0x%04X, value=%u", temp->state_type, temp->state_value);
        vPortFree(temp);
    }
    xSemaphoreGive(s_state_report_mutex);
}

/* 外部接口：当收到状态上报ACK时调用，移除待重传项 */
void state_report_ack_handler(void) {
    ESP_LOGI(TAG, "Received state report ACK, processing pending list");
    remove_first_pending_item();
}

/* 主动上传状态接口：
   1. 将待上传项加入重传缓存链表；
   2. 若模块已联网，则立即发送；否则将缓存，待联网后由重传任务自动发送 */
esp_err_t state_report_upload(uint16_t state_type, uint32_t state_value) {
    ESP_LOGI(TAG, "Request to upload state: type=0x%04X, value=%u", state_type, state_value);

    add_pending_item(state_type, state_value);

    if (is_connected()) {
        uart_packet_t report_packet;
        create_state_report_packet(&report_packet, state_type, state_value);
        esp_err_t ret = uart_comm_send_packet(&report_packet);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send state report packet");
            return ret;
        }
        ESP_LOGI(TAG, "State report packet sent immediately");
    } else {
        ESP_LOGW(TAG, "Not connected, state report cached for later transmission");
    }

    /* TODO: 此处可以调用 MQTT 上报接口，将状态数据同时上传到云端 */
    // 例如：state_report_mqtt_upload(state_type, state_value);

    return ESP_OK;
}

/* 发送状态上报应答（用于MCU上报状态时，WiFi回复应答） */
esp_err_t state_report_send_ack(void) {
    uart_packet_t ack_packet = {0};
    ack_packet.header[0] = 0xAA;
    ack_packet.header[1] = 0x55;
    ack_packet.command   = CMD_STATE_REPORT_ACK;
    memset(ack_packet.data, 0, sizeof(ack_packet.data));
    ack_packet.checksum  = uart_comm_calc_checksum((uint8_t *)&ack_packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Sending state report ACK");
    return uart_comm_send_packet(&ack_packet);
}

/* 处理接收到的MCU上报状态数据包 */
esp_err_t state_report_handle_uart_packet(const uart_packet_t *packet) {
    if (!packet) {
        ESP_LOGE(TAG, "Invalid packet pointer in state_report_handle_uart_packet");
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t state_type = packet->data[0] | (packet->data[1] << 8);
    uint32_t state_value = packet->data[2] |
                           (packet->data[3] << 8) |
                           (packet->data[4] << 16) |
                           (packet->data[5] << 24);
    ESP_LOGI(TAG, "Received state report from MCU: type=0x%04X, value=%u", state_type, state_value);

    // 根据实际需求进行处理，例如更新状态、记录日志等
    // 此处仅记录日志，实际应用中可扩展更多业务逻辑

    // 发送应答给MCU
    return state_report_send_ack();
}

/* 重传任务：周期性检查待重传链表，如果联网且超时未ACK则重传 */
static void state_report_retx_task(void *param) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(STATE_REPORT_RETX_TASK_DELAY_MS));

        if (!is_connected()) {
            continue;
        }

        uint32_t now = cc_hal_sys_get_ms();

        xSemaphoreTake(s_state_report_mutex, portMAX_DELAY);
        state_report_item_t *cur = s_pending_list;
        while (cur != NULL) {
            if ((now - cur->last_sent_time) >= STATE_REPORT_TIMEOUT_MS) {
                if (cur->retry_count < STATE_REPORT_MAX_RETRIES) {
                    uart_packet_t report_packet;
                    create_state_report_packet(&report_packet, cur->state_type, cur->state_value);
                    esp_err_t ret = uart_comm_send_packet(&report_packet);
                    if (ret == ESP_OK) {
                        cur->last_sent_time = now;
                        cur->retry_count++;
                        ESP_LOGI(TAG, "Retransmitted state report: type=0x%04X, value=%u, retry=%u",
                                 cur->state_type, cur->state_value, cur->retry_count);
                    } else {
                        ESP_LOGE(TAG, "Retransmission failed for state report: type=0x%04X, value=%u",
                                 cur->state_type, cur->state_value);
                    }
                } else {
                    ESP_LOGW(TAG, "Max retries reached for state report: type=0x%04X, value=%u, dropping message",
                             cur->state_type, cur->state_value);
                    break; // 跳出循环，后续统一清除已超时项
                }
            }
            cur = cur->next;
        }
        /* 清除链表中超出最大重传次数的项 */
        while (s_pending_list && s_pending_list->retry_count >= STATE_REPORT_MAX_RETRIES) {
            state_report_item_t *temp = s_pending_list;
            s_pending_list = s_pending_list->next;
            ESP_LOGW(TAG, "Dropping state report after max retries: type=0x%04X, value=%u",
                     temp->state_type, temp->state_value);
            vPortFree(temp);
        }
        xSemaphoreGive(s_state_report_mutex);
    }
}

/* 初始化状态上报模块：创建互斥锁及启动重传任务 */
esp_err_t state_report_init(void) {
    s_state_report_mutex = xSemaphoreCreateMutex();
    if (s_state_report_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create state report mutex");
        return ESP_FAIL;
    }
    BaseType_t ret = xTaskCreate(state_report_retx_task, "state_report_retx_task", 2048, NULL, 10, &s_state_report_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create state report retransmission task");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "State report module initialized");
    return ESP_OK;
}

/**
 * @brief 通过MQTT上报状态数据接口（接口函数，未实际实现上报逻辑）
 *
 * TODO: 在实际应用中，请完善该函数，调用相应的MQTT模块接口上报数据
 */
esp_err_t state_report_mqtt_upload(uint16_t state_type, uint32_t state_value)
{
    ESP_LOGI(TAG, "TODO: Implement MQTT upload for state: type=0x%04X, value=%u", state_type, state_value);
    // 此处返回ESP_OK，仅作占位，后续实现时请调用MQTT上报接口
    return ESP_OK;
}
