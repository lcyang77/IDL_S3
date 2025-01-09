#ifndef CC_EVENT_H
#define CC_EVENT_H

#include "esp_err.h"

/**
 * @brief 重置系统事件和状态
 * 
 * 该函数会：
 * 1. 重置 WiFi 连接状态
 * 2. 重置设备状态
 * 3. 重置状态机到初始状态
 * 4. 清除所有待处理的事件
 * 5. 重置系统标志位
 * 
 * @return esp_err_t ESP_OK 成功，其他值表示失败
 */
esp_err_t cc_event_reset(void);

#endif /* CC_EVENT_H */