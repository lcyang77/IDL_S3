/************************************************
 * File: get_time.h
 ************************************************/
#ifndef GET_TIME_H
#define GET_TIME_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 get_time 模块（内部创建事件组、清空缓存等）
 */
void get_time_init(void);

/**
 * @brief 启动一次“获取时间”的流程（HTTP 请求 + 重试）
 *
 * 函数会快速返回。如果想知道最后结果，请调用 get_time_wait_done() 阻塞等待。
 *
 * @return ESP_OK 表示成功发起请求；如果多次重试都失败，将返回错误码。
 */
esp_err_t get_time_start_update(void);

/**
 * @brief 阻塞等待“获取时间”完成(成功或超时)。
 *
 * @param timeout_ms 等待超时时间(ms)。超时后返回false。
 * @return true=本次更新成功(已获取到非零时间)，false=更新失败或超时。
 */
bool get_time_wait_done(uint32_t timeout_ms);

/**
 * @brief 获取最近缓存的UTC秒数
 * @return UTC秒数；若尚未获取成功，可能为0
 */
uint32_t get_time_get_utc(void);

/**
 * @brief 获取最近缓存的时区(单位0.1小时)
 * @return 时区值；若尚未获取成功，可能为0
 */
int8_t get_time_get_timezone(void);

#ifdef __cplusplus
}
#endif

#endif // GET_TIME_H
