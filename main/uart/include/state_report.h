#ifndef STATE_REPORT_H
#define STATE_REPORT_H

#include "esp_err.h"
#include "net_uart_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_STATE_REPORT       0x42
#define CMD_STATE_REPORT_ACK   0x43

/**
 * @brief 处理接收到的状态上报数据包
 *
 * @param packet 指向接收到的数据包
 *
 * @return esp_err_t ESP_OK 表示处理成功
 */
esp_err_t state_report_handle_uart_packet(const uart_packet_t *packet);

/**
 * @brief 主动上传状态
 *
 * @param state_type 状态类型（2 字节，小端序），例如 0x1006 表示电量1
 * @param state_value 状态值（4 字节，小端序），例如 16% 表示为 0x10,0x00,0x00,0x00
 *
 * @return esp_err_t ESP_OK 表示发送成功
 */
esp_err_t state_report_upload(uint16_t state_type, uint32_t state_value);

/**
 * @brief 发送状态上报应答
 *
 * @return esp_err_t ESP_OK 表示发送成功
 */
esp_err_t state_report_send_ack(void);

/**
 * @brief 初始化状态上报模块（包括重传与缓存机制）
 *
 * @return esp_err_t ESP_OK 表示初始化成功
 */
esp_err_t state_report_init(void);

/**
 * @brief 处理收到的状态上报应答，移除待重传的消息
 */
void state_report_ack_handler(void);

/**
 * @brief 通过MQTT上报状态数据（接口函数，尚未实现实际上报逻辑）
 *
 * @param state_type 状态类型（2 字节，小端序）
 * @param state_value 状态值（4 字节，小端序）
 *
 * @return esp_err_t ESP_OK 表示上传成功（TODO: 实际实现中应返回相应错误码）
 */
esp_err_t state_report_mqtt_upload(uint16_t state_type, uint32_t state_value);

#ifdef __cplusplus
}
#endif

#endif // STATE_REPORT_H
