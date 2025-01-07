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
 * @brief 初始化 get_time 模块，清空之前的UTC和时区缓存
 */
void get_time_init(void);

/**
 * @brief 从服务器获取网络时间并更新到本地缓存（通过HTTP）。
 *
 * @note 仅在已联网的情况下才能成功获取；
 *       如果HTTP失败或服务端响应异常，则本地缓存不会被更新(仍然是之前的值)。
 *
 * @note 内部使用“HTTP 事件回调”，在 HTTP_EVENT_ON_DATA 收集数据，
 *       在 HTTP_EVENT_ON_FINISH 时解析JSON并更新本地 s_utc_time & s_time_zone
 */
void get_time_update(void);

/**
 * @brief 获取当前缓存的UTC秒数
 * 
 * @return 当前UTC时间（自1970-01-01 00:00:00以来的秒数）。初始或失败时可能为0。
 */
uint32_t get_time_get_utc(void);

/**
 * @brief 获取当前缓存的时区（单位为“小时*10 + 刻(15分钟)”）
 * 
 * @return 取值范围 -128~127，例如UTC+8 => 80，UTC+5:45 => 53。
 *         初始或失败时可能为0。
 */
int8_t get_time_get_timezone(void);

// 注册时间更新完成的回调
typedef void (*get_time_callback_t)(bool success);
esp_err_t get_time_register_callback(get_time_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // GET_TIME_H
