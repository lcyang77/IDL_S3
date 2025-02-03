/**
 * @file unlock.h
 * @brief 远程开锁模块头文件
 *
 * 提供远程开锁相关API，包括WiFi->MCU(0x13)和MCU->WiFi(0x12)指令处理。
 */

#ifndef UNLOCK_H
#define UNLOCK_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "net_uart_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化远程开锁模块
 * 
 * - 创建定时器
 * - 初始化内部状态
 * 
 * @return esp_err_t
 *         - ESP_OK: 初始化成功
 *         - 其它: 失败
 */
esp_err_t unlock_init(void);

/**
 * @brief 由MQTT/云端下发远程开锁时，调用此函数发送 0x13 给门锁MCU
 * 
 * @param[in] user_type   用户类型(目前只会是手机用户0x05)
 * @param[in] user_id     用户编号(小端2字节)，上报门锁已打开时需要带回
 * @return esp_err_t
 *         - ESP_OK: 成功发送
 *         - 其它: 发送失败 (UART或内部状态问题)
 */
esp_err_t unlock_send_remote_unlock_to_mcu(uint8_t user_type, uint16_t user_id);

/**
 * @brief 在 UART 回调中，当收到MCU发来的指令(0x12等)时，调用此函数
 * 
 * @param[in] packet  已解析好的 UART 数据包
 */
void unlock_handle_mcu_packet(const uart_packet_t *packet);

/**
 * @brief 查询当前是否正在进行一次远程开锁流程
 *
 * 如果正在等待MCU回复(0x12)，则返回true，否则false。
 */
bool unlock_is_in_progress(void);

#ifdef __cplusplus
}
#endif

#endif // UNLOCK_H
