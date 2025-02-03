/**
 * @file unlock.c
 * @brief 远程开锁模块实现
 *
 * 实现 WiFi->MCU(0x13) 和 MCU->WiFi(0x12) 指令的处理，以及超时管理等。
 */

#include "unlock.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "net_uart_comm.h"  // for uart_comm_send_packet(), uart_comm_calc_checksum()
#include "cc_hal_sys.h"     // if you need random, ms-tick etc
#include "cc_hal_os.h"      // if you need semaphores, etc
#include "gs_mqtt.h"        // if you want to publish results to cloud
//#include "msg_upload.h"    // if you want to notify the msg_upload module

static const char *TAG = "unlock";

/* -------------------- 配置宏 -------------------- */

// 超时时间 (毫秒)。若在此时间内，MCU没回 0x12，视为远程开锁失败
#define UNLOCK_RESPONSE_TIMEOUT_MS   (15000) // 15s 你可自行修改

// 如果需要支持多次重发 0x13，可定义 RE_SEND_ATTEMPTS 等，这里先不实现

/* -------------------- 数据结构和状态 -------------------- */

// 标记是否处于“等待MCU回包0x12”的状态
static bool s_unlock_in_progress = false;

// 记录当前下发给MCU的 user_type & user_id（如果你想在回包时检查或日志中使用）
static uint8_t s_current_user_type = 0;
static uint16_t s_current_user_id  = 0;

// 如果需要记录下发时间戳，也可加
// static uint32_t s_send_time_ms = 0;

/**
 * @brief 用 FreeRTOS定时器来管理超时
 */
static TimerHandle_t s_unlock_timer = NULL;

/* -------------------- 静态函数声明 -------------------- */
static void unlock_timeout_cb(TimerHandle_t xTimer);
static esp_err_t send_cmd_13_to_mcu(uint8_t user_type, uint16_t user_id);

/* ----------------------------------------------------- */

esp_err_t unlock_init(void)
{
    ESP_LOGI(TAG, "unlock_init: create timer & reset state");

    s_unlock_in_progress = false;
    s_current_user_type = 0;
    s_current_user_id = 0;

    // 创建一个一次性定时器(单次), 到期后调用 unlock_timeout_cb
    s_unlock_timer = xTimerCreate("unlockTimer",
                                  pdMS_TO_TICKS(UNLOCK_RESPONSE_TIMEOUT_MS),
                                  pdFALSE,   // 单次
                                  NULL,
                                  unlock_timeout_cb);
    if (!s_unlock_timer) {
        ESP_LOGE(TAG, "Failed to create unlockTimer");
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief 对外API：由云端确认开锁后，调用此函数向MCU发送 0x13 指令
 */
esp_err_t unlock_send_remote_unlock_to_mcu(uint8_t user_type, uint16_t user_id)
{
    // 如果当前已有一个远程开锁流程尚未结束，则拒绝本次请求(或取消之前的?)
    if (s_unlock_in_progress) {
        ESP_LOGW(TAG, "unlock_send_remote_unlock_to_mcu: previous unlock in progress, reject");
        return ESP_ERR_INVALID_STATE;
    }

    // 记录用户信息
    s_current_user_type = user_type;
    s_current_user_id   = user_id;

    // 尝试发送 0x13 指令
    esp_err_t ret = send_cmd_13_to_mcu(user_type, user_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "send_cmd_13_to_mcu fail, ret=%d", ret);
        return ret;
    }

    // 发送成功后，启动定时器，等待MCU回 0x12
    s_unlock_in_progress = true;
    xTimerStop(s_unlock_timer, 0); 
    xTimerStart(s_unlock_timer, 0);

    // 可在这里记录发送时间、或发MQTT消息“正在远程开锁”
    // s_send_time_ms = cc_hal_sys_get_ms();

    return ESP_OK;
}

/**
 * @brief 当 UART 收到 0x12(远程开锁应答) 时，调用此函数
 */
void unlock_handle_mcu_packet(const uart_packet_t *packet)
{
    if (packet->command != 0x12) {
        // 只关心0x12
        return;
    }

    ESP_LOGI(TAG, "Got 0x12 from MCU => remote unlock ack");

    if (s_unlock_in_progress) {
        // 停止超时
        xTimerStop(s_unlock_timer, 0);
        s_unlock_in_progress = false;
        ESP_LOGI(TAG, "Remote unlock done, user_type=0x%02X, user_id=%u",
                 s_current_user_type, s_current_user_id);

        // 可以在此处通知云端“远程开锁成功”
        // 例如:
        // char payload[64];
        // snprintf(payload, sizeof(payload), "{\"event\":\"remote_unlock_done\",\"user_id\":%u}", s_current_user_id);
        // gs_mqtt_publish("/event/property/post", (uint8_t*)payload, strlen(payload), GS_MQTT_QOS0, 0);

        // 如果你需要后续再做别的流程(比如“门已打开”事件上报), 
        // 通常由 MCU 发送 0x03(0x01) 再进入 msg_upload. 
    } else {
        ESP_LOGW(TAG, "Received 0x12 but s_unlock_in_progress == false (unexpected?)");
    }
}

/**
 * @brief 查询当前是否在等待 0x12
 */
bool unlock_is_in_progress(void)
{
    return s_unlock_in_progress;
}

/* -------------------- 静态函数实现 -------------------- */

/**
 * @brief 拼包并发送指令 0x13 (WiFi->MCU)
 */
static esp_err_t send_cmd_13_to_mcu(uint8_t user_type, uint16_t user_id)
{
    uart_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.header[0] = 0xAA;
    pkt.header[1] = 0x55;
    pkt.command   = 0x13;   // 远程开锁指令

    // data[0] = user_type (如0x05=手机用户)
    pkt.data[0] = user_type;

    // data[1..2] = user_id(2字节，小端)
    pkt.data[1] = (uint8_t)(user_id & 0xFF);
    pkt.data[2] = (uint8_t)((user_id >> 8) & 0xFF);

    // data[3..5] = 0x00
    pkt.data[3] = 0x00;
    pkt.data[4] = 0x00;
    pkt.data[5] = 0x00;

    // 计算校验和
    pkt.checksum = uart_comm_calc_checksum((uint8_t*)&pkt, sizeof(pkt)-1);

    ESP_LOGI(TAG, "Sending 0x13 => user_type=0x%02X, user_id=%u", user_type, user_id);

    // 调用 UART 发送函数
    return uart_comm_send_packet(&pkt);
}

/**
 * @brief 定时器超时回调：MCU未在限定时间内回 0x12
 */
static void unlock_timeout_cb(TimerHandle_t xTimer)
{
    ESP_LOGW(TAG, "unlock_timeout_cb: Did not receive 0x12 from MCU within %dms => fail",
             UNLOCK_RESPONSE_TIMEOUT_MS);

    if (s_unlock_in_progress) {
        s_unlock_in_progress = false;
        // 可以在这里通知云端：远程开锁失败 or 超时
        // char payload[64];
        // sprintf(payload, "{\"event\":\"remote_unlock_fail\",\"reason\":\"timeout\"}");
        // gs_mqtt_publish("/event/property/post", (uint8_t*)payload, strlen(payload), GS_MQTT_QOS0, 0);
    }
}
