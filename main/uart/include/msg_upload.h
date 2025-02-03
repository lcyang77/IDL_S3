#ifndef MSG_UPLOAD_H
#define MSG_UPLOAD_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "net_uart_comm.h"

// 初始化本模块
esp_err_t msg_upload_init(void);

// UART回调中, 调用此函数统一处理 0x03,0x04 等指令
void msg_upload_uart_callback(const uart_packet_t *packet);

// MQTT下行指令中, 若检测到远程开锁命令 => 调用此函数
// 以给门锁发送 0x12 "远程开锁应答" (或者 0x13, 具体看项目协议)
esp_err_t msg_upload_send_remote_unlock_cmd_to_lock(void);

#endif // MSG_UPLOAD_H
