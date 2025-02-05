#ifndef MSG_UPLOAD_H
#define MSG_UPLOAD_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "net_uart_comm.h"

// 初始化本模块
esp_err_t msg_upload_init(void);

// UART 回调中调用该函数统一处理 0x03、0x04 等指令
void msg_upload_uart_callback(const uart_packet_t *packet);

// MQTT 下行指令中，若检测到远程开锁命令，则调用此函数发送远程开锁指令
esp_err_t msg_upload_send_remote_unlock_cmd_to_lock(void);

#endif // MSG_UPLOAD_H
