/************************************************
 * File: net_sta.h
 ************************************************/
#ifndef NET_STA_H
#define NET_STA_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// 联网状态枚举
typedef enum {
    NET_STATUS_NOT_CONFIGURED      = 0x01, // 未配置网络
    NET_STATUS_CONNECTING_ROUTER   = 0x02, // 正在连接路由器或基站
    NET_STATUS_CONNECTED_ROUTER    = 0x03, // 已连接路由器或基站
    NET_STATUS_CONNECTED_SERVER    = 0x04, // 已连接云服务器
} net_status_t;

// 初始化联网状态管理
esp_err_t net_sta_init(void);

// 获取当前联网状态
net_status_t net_sta_get_status(void);

// 更新联网状态并发送通知
esp_err_t net_sta_update_status(net_status_t status);

// 启动联网状态监控（定时判定），仅在收到配网指令后调用
void net_sta_start_monitor(void);

#endif // NET_STA_H
