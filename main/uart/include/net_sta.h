#ifndef NET_STA_H
#define NET_STA_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * 联网状态枚举：
 *   - NET_STATUS_NOT_CONFIGURED      : 未配置网络
 *   - NET_STATUS_CONNECTING_ROUTER   : 正在连接路由器或基站
 *   - NET_STATUS_CONNECTED_ROUTER    : 已连接路由器或基站
 *   - NET_STATUS_CONNECTED_SERVER    : 已连接云服务器
 */
typedef enum {
    NET_STATUS_NOT_CONFIGURED      = 0x01,
    NET_STATUS_CONNECTING_ROUTER   = 0x02,
    NET_STATUS_CONNECTED_ROUTER    = 0x03,
    NET_STATUS_CONNECTED_SERVER    = 0x04,
} net_status_t;

/**
 * @brief 初始化联网状态管理模块
 *
 * @return esp_err_t
 */
esp_err_t net_sta_init(void);

/**
 * @brief 获取当前联网状态
 *
 * @return net_status_t
 */
net_status_t net_sta_get_status(void);

/**
 * @brief 更新联网状态并发送通知给 MCU
 *
 * @param status 新的联网状态
 * @return esp_err_t
 */
esp_err_t net_sta_update_status(net_status_t status);

/**
 * @brief 启动联网状态监控（主要用于启动备份定时器，防止事件未能及时响应）
 */
void net_sta_start_monitor(void);

#endif // NET_STA_H
