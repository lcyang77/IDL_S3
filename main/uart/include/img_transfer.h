#ifndef IMG_TRANSFER_H
#define IMG_TRANSFER_H

#include "esp_err.h"
#include "net_uart_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化图传模块
 *
 * 调用此函数完成图传模块内部资源和状态的初始化，
 * 建议在 app_main() 中与其他模块初始化一起调用。
 *
 * @return ESP_OK 初始化成功，其他错误码表示失败。
 */
esp_err_t img_transfer_init(void);

/**
 * @brief 处理 MCU 发送的图传设置数据包（命令 0x1C）。
 *
 * 当 UART 接收到图传设置数据包后，调用此函数处理图传开启或关闭逻辑，
 * 同时回复应答（0x1D），若要求开启图传则异步启动图传任务完成采集、上传，
 * 并最终反馈图传结果（0x27）。
 *
 * @param packet 收到的图传设置数据包
 */
void img_transfer_handle_uart_packet(const uart_packet_t *packet);

#ifdef __cplusplus
}
#endif

#endif /* IMG_TRANSFER_H */
