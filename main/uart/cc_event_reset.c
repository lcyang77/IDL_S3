#include "cc_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "cc_hal_wifi.h"
#include "gs_device.h"

static const char *TAG = "cc_event";

// 状态机当前状态
typedef enum {
    CC_STATE_INIT = 0,
    CC_STATE_WIFI_CONFIG,
    CC_STATE_CONNECTING,
    CC_STATE_CONNECTED,
    CC_STATE_CLOUD_CONNECTED,
    CC_STATE_ERROR
} cc_state_t;

// 全局状态变量
static struct {
    cc_state_t current_state;
    SemaphoreHandle_t state_mutex;
    bool is_initialized;
} g_cc_state = {
    .current_state = CC_STATE_INIT,
    .state_mutex = NULL,
    .is_initialized = false
};

// 初始化状态机
static esp_err_t cc_event_init_if_needed(void)
{
    if (!g_cc_state.is_initialized) {
        // 创建互斥锁
        g_cc_state.state_mutex = xSemaphoreCreateMutex();
        if (g_cc_state.state_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create state mutex");
            return ESP_ERR_NO_MEM;
        }
        g_cc_state.is_initialized = true;
        g_cc_state.current_state = CC_STATE_INIT;
    }
    return ESP_OK;
}

/**
 * @brief 重置系统事件和状态
 * 
 * @return esp_err_t ESP_OK 成功，其他值表示失败
 */
esp_err_t cc_event_reset(void)
{
    ESP_LOGI(TAG, "Starting system event reset");
    
    // 确保状态机已初始化
    esp_err_t ret = cc_event_init_if_needed();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize state machine");
        return ret;
    }

    // 获取状态互斥锁
    if (xSemaphoreTake(g_cc_state.state_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take state mutex");
        return ESP_ERR_TIMEOUT;
    }

    // 1. 重置 WiFi 状态
    ESP_LOGI(TAG, "Resetting WiFi state...");
    cc_hal_wifi_sta_disconnect();  // 断开 WiFi 连接
    
    // 2. 重置设备状态
    ESP_LOGI(TAG, "Resetting device state...");
    cc_event_reset();  // 重置设备状态（需要在 gs_device.h 中实现）

    // 3. 重置状态机状态
    ESP_LOGI(TAG, "Resetting state machine...");
    g_cc_state.current_state = CC_STATE_INIT;

    // 4. 清除任何待处理的事件或回调
    ESP_LOGI(TAG, "Clearing pending events...");
    // 在这里添加清除事件队列的代码（如果有的话）

    // 5. 重置所有标志位
    g_cc_state.is_initialized = true;  // 保持初始化状态，因为我们仍然需要使用

    // 释放互斥锁
    xSemaphoreGive(g_cc_state.state_mutex);

    ESP_LOGI(TAG, "System event reset completed successfully");
    return ESP_OK;
}

/**
 * @brief 获取当前状态（用于调试）
 */
cc_state_t cc_event_get_state(void)
{
    if (g_cc_state.state_mutex != NULL) {
        if (xSemaphoreTake(g_cc_state.state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            cc_state_t state = g_cc_state.current_state;
            xSemaphoreGive(g_cc_state.state_mutex);
            return state;
        }
    }
    return CC_STATE_ERROR;
}

/**
 * @brief 设置新状态（内部使用）
 */
esp_err_t cc_event_set_state(cc_state_t new_state)
{
    if (g_cc_state.state_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_cc_state.state_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "State transition: %d -> %d", g_cc_state.current_state, new_state);
    g_cc_state.current_state = new_state;
    
    xSemaphoreGive(g_cc_state.state_mutex);
    return ESP_OK;
}