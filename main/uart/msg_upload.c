#include "msg_upload.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <string.h>
#include <stdio.h>

#include "net_uart_comm.h"  // 包含 uart_comm_send_packet 等函数
#include "gs_mqtt.h"        // 包含 gs_mqtt_publish 等函数
#include "cc_hal_sys.h"     // 若需要 ms 计时 / 软复位
#include "cc_hal_os.h"      // 若需要队列/信号量
#include "esp_system.h"     // 可能需要 esp_restart() 等

static const char *TAG = "msg_upload";

/* =========== 命令定义（与 net_uart_comm.h 中 CMD_XXX 对应） =========== */
#define CMD_MSG_UPLOAD        0x03  // 消息上传
#define CMD_POWER_OFF_NOTIFY  0x04  // 断电通知
#define CMD_MSG_UPLOAD_RSP    0x02  // 消息上传的应答（通用）
#define CMD_REMOTE_UNLOCK_RSP 0x12  // 远程开锁应答（模块→MCU）
#define CMD_REMOTE_UNLOCK_CMD 0x13  // 远程开锁指令（云→模块→MCU）

// 事件定义（在 data[0]）：远程开锁请求=0x03，已打开=0x01
#define EVENT_UNLOCK_REQUEST  0x03  // 远程开锁请求
#define EVENT_UNLOCKED        0x01  // 已打开

// 超时时间定义
#define REMOTE_UNLOCK_TIMEOUT_MS   (60000)  // 60s
#define NORMAL_EVENT_TIMEOUT_MS    (12000)  // 12s

/* =========== 全局静态变量 =========== */

// 标记是否正在处理“远程开锁请求”
static bool s_remote_req_in_progress = false;
// 远程开锁请求超时定时器
static TimerHandle_t s_remote_req_timer = NULL;

// 标记是否正在处理“已打开（0x01）”事件上传
static bool s_unlocked_in_progress = false;
// 已打开事件超时定时器
static TimerHandle_t s_unlocked_timer = NULL;

/* =========== 静态函数声明 =========== */
static void msg_upload_send_common_ack(bool success);
static void remote_req_timer_cb(TimerHandle_t xTimer);
static void unlocked_timer_cb(TimerHandle_t xTimer);
static void handle_msg_upload(const uart_packet_t *packet);
static void handle_power_off_notify(const uart_packet_t *packet);

/* ------------------------------------------------------------- */
/* 初始化本模块：创建定时器等 */
esp_err_t msg_upload_init(void)
{
    s_remote_req_timer = xTimerCreate("remote_req_timer",
                                      pdMS_TO_TICKS(REMOTE_UNLOCK_TIMEOUT_MS),
                                      pdFALSE,
                                      NULL,
                                      remote_req_timer_cb);
    if (!s_remote_req_timer) {
        ESP_LOGE(TAG, "Failed to create remote_req_timer");
        return ESP_FAIL;
    }

    s_unlocked_timer = xTimerCreate("unlocked_timer",
                                    pdMS_TO_TICKS(NORMAL_EVENT_TIMEOUT_MS),
                                    pdFALSE,
                                    NULL,
                                    unlocked_timer_cb);
    if (!s_unlocked_timer) {
        ESP_LOGE(TAG, "Failed to create unlocked_timer");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "msg_upload_init success");
    return ESP_OK;
}

/* ------------------------------------------------------------- */
/* 在 UART 回调中处理 MCU → 模块指令 */
void msg_upload_uart_callback(const uart_packet_t *packet)
{
    switch (packet->command) {
    case CMD_MSG_UPLOAD:
        handle_msg_upload(packet);
        break;

    case CMD_POWER_OFF_NOTIFY:
        ESP_LOGI(TAG, "Got CMD_POWER_OFF_NOTIFY (0x04) => handling power off notification");
        handle_power_off_notify(packet);
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------- */
/* 远程开锁命令（0x13）【云→模块→MCU】 */
esp_err_t msg_upload_send_remote_unlock_cmd_to_lock(void)
{
    ESP_LOGI(TAG, "Sending remote unlock command (0x13 or 0x12) to door lock");

    uart_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.header[0] = 0xAA;
    pkt.header[1] = 0x55;
    pkt.command   = CMD_REMOTE_UNLOCK_RSP; // 0x12

    pkt.data[0] = 0x00;
    pkt.data[1] = 0x00;
    pkt.data[2] = 0x00;
    pkt.data[3] = 0x00;
    pkt.data[4] = 0x00;
    pkt.data[5] = 0x00;

    pkt.checksum = uart_comm_calc_checksum((uint8_t*)&pkt, sizeof(pkt)-1);

    return uart_comm_send_packet(&pkt);
}

/* ------------------------------------------------------------- */
/* (1) 处理消息上传（0x03） */
static void handle_msg_upload(const uart_packet_t *packet)
{
    uint8_t event     = packet->data[0];
    uint8_t eventinfo = packet->data[1];

    ESP_LOGI(TAG, "[CMD=0x03] event=0x%02X, event_info=0x%02X", event, eventinfo);

    if (event == EVENT_UNLOCK_REQUEST) {
        if (s_remote_req_in_progress) {
            ESP_LOGW(TAG, "Remote unlock request in progress, ignore");
            msg_upload_send_common_ack(false);
            return;
        }

        s_remote_req_in_progress = true;
        xTimerStop(s_remote_req_timer, 0);
        xTimerStart(s_remote_req_timer, 0);

        char payload[64];
        snprintf(payload, sizeof(payload), "{\"cmd\":3,\"desc\":\"remote_req\"}");
        gs_mqtt_publish("/event/property/post", (uint8_t*)payload, strlen(payload), GS_MQTT_QOS0, 0);

        ESP_LOGI(TAG, "Remote request upload done, waiting 60s for cloud => 0x13 (unlock command)");
        msg_upload_send_common_ack(true);

    } else if (event == EVENT_UNLOCKED) {
        if (s_unlocked_in_progress) {
            ESP_LOGW(TAG, "Unlocked event in progress, ignore");
            msg_upload_send_common_ack(false);
            return;
        }

        s_unlocked_in_progress = true;
        xTimerStop(s_unlocked_timer, 0);
        xTimerStart(s_unlocked_timer, 0);

        char payload[64];
        snprintf(payload, sizeof(payload), "{\"cmd\":3,\"desc\":\"unlocked\"}");
        gs_mqtt_publish("/event/property/post", (uint8_t*)payload, strlen(payload), GS_MQTT_QOS0, 0);

        ESP_LOGI(TAG, "Unlocked event uploaded, waiting 12s for cloud response if needed");
        msg_upload_send_common_ack(true);

    } else {
        ESP_LOGI(TAG, "Other event=0x%02X, treat as normal 12s event...", event);
        msg_upload_send_common_ack(true);
    }
}

/* ------------------------------------------------------------- */
/* (2) 处理断电通知（0x04） */
/* 此处仅发送符合协议要求的应答包，不再启动异步清理任务 */
static void handle_power_off_notify(const uart_packet_t *packet)
{
    uint8_t mode = packet->data[0]; // 获取模式字段（0x00 默认，0x01 测试模式）
    ESP_LOGI(TAG, "[CMD=0x04] Power off notify received, mode=0x%02X", mode);

    // 立即发送符合断电通知协议要求的应答包
    if (uart_comm_send_power_off_ack(true) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send Power Off ACK");
    }
}

/* ------------------------------------------------------------- */
/* 远程开锁请求（60s）定时器超时回调 */
static void remote_req_timer_cb(TimerHandle_t xTimer)
{
    ESP_LOGW(TAG, "Remote unlock request timed out (60s) => no cloud response => fail");
    s_remote_req_in_progress = false;
}

/* 已打开（12s）定时器超时回调 */
static void unlocked_timer_cb(TimerHandle_t xTimer)
{
    ESP_LOGW(TAG, "Unlocked event timed out (12s)");
    s_unlocked_in_progress = false;
}

/* ------------------------------------------------------------- */
/* 给门锁返回“消息上传应答（0x02）”的通用函数 */
static void msg_upload_send_common_ack(bool success)
{
    uart_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.header[0] = 0xAA;
    pkt.header[1] = 0x55;
    pkt.command   = CMD_MSG_UPLOAD_RSP;  // 0x02

    pkt.data[0] = success ? 0x00 : 0x02;
    pkt.data[1] = 0x03;
    pkt.data[2] = 0x00;
    pkt.data[3] = 0x00;
    pkt.data[4] = 0x00;
    pkt.data[5] = 0x00;
    pkt.checksum = uart_comm_calc_checksum((uint8_t*)&pkt, sizeof(pkt)-1);

    ESP_LOGI(TAG, "Send 0x02 ack => success=%d", success);
    uart_comm_send_packet(&pkt);
}
