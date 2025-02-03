#include "msg_upload.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <string.h>
#include <stdio.h>

#include "net_uart_comm.h"  // 里头有 uart_comm_send_packet 等
#include "gs_mqtt.h"        // 里头有 gs_mqtt_publish 等
#include "cc_hal_sys.h"     // 若需要 ms计时 / 软复位
#include "cc_hal_os.h"      // 若需要队列/信号量
#include "esp_system.h"     // 可能需要 esp_restart() 等

static const char *TAG = "msg_upload";

/* =========== 命令定义(可与 net_uart_comm.h 中 CMD_XXX 对应) =========== */
#define CMD_MSG_UPLOAD        0x03  // 消息上传
#define CMD_POWER_OFF_NOTIFY  0x04  // 断电通知
#define CMD_MSG_UPLOAD_RSP    0x02  // 消息上传的应答(通用)
#define CMD_REMOTE_UNLOCK_RSP 0x12  // 远程开锁应答(模块->MCU)
#define CMD_REMOTE_UNLOCK_CMD 0x13  // 远程开锁指令(云->模块->MCU)

// 事件定义(在 data[0] )：远程开锁请求=0x03, 已打开=0x01 ...
#define EVENT_UNLOCK_REQUEST  0x03  // 远程开锁请求
#define EVENT_UNLOCKED        0x01  // 已打开

// 超时时间
#define REMOTE_UNLOCK_TIMEOUT_MS   (60000)  // 60s
#define NORMAL_EVENT_TIMEOUT_MS    (12000)  // 12s

/* =========== 全局静态变量 =========== */

// 标记当前是否正在处理“远程开锁请求”
static bool s_remote_req_in_progress = false;
// 远程开锁请求的超时定时器
static TimerHandle_t s_remote_req_timer = NULL;

// 标记当前是否正在处理“已打开(0x01)”的上传
static bool s_unlocked_in_progress = false;
// 已打开事件的超时定时器
static TimerHandle_t s_unlocked_timer = NULL;

/* =========== 静态函数声明 =========== */
static void msg_upload_send_common_ack(bool success);
static void remote_req_timer_cb(TimerHandle_t xTimer);
static void unlocked_timer_cb(TimerHandle_t xTimer);
static void handle_msg_upload(const uart_packet_t *packet);
static void handle_power_off_notify(void);

/* ------------------------------------------------------------- */
/* 初始化本模块：创建定时器等 */
esp_err_t msg_upload_init(void)
{
    // 创建 60s定时器(单次), 用于“远程开锁请求”
    s_remote_req_timer = xTimerCreate("remote_req_timer",
                                      pdMS_TO_TICKS(REMOTE_UNLOCK_TIMEOUT_MS),
                                      pdFALSE,
                                      NULL,
                                      remote_req_timer_cb);
    if (!s_remote_req_timer) {
        ESP_LOGE(TAG, "Failed to create remote_req_timer");
        return ESP_FAIL;
    }

    // 创建 12s定时器(单次), 用于“已打开”事件
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
/* 在 UART 回调中，处理 MCU -> 模块 的指令 */
void msg_upload_uart_callback(const uart_packet_t *packet)
{
    // 分发指令
    switch (packet->command) {
    case CMD_MSG_UPLOAD:
        // 0x03: 消息上传
        handle_msg_upload(packet);
        break;

    case CMD_POWER_OFF_NOTIFY:
        // 0x04: 断电通知
        handle_power_off_notify();
        break;

    default:
        // 其他指令，若需要可以在这里扩展
        // ...
        break;
    }
}

/* ------------------------------------------------------------- */
/* 远程开锁命令(0x13) [云->模块->MCU]：  
 * 当 MQTT 回调解析到云端想要远程开锁，就调用此函数 => 发 0x12(或0x13) 给门锁MCU */
esp_err_t msg_upload_send_remote_unlock_cmd_to_lock(void)
{
    ESP_LOGI(TAG, "Sending remote unlock command(0x13 or 0x12) to door lock");

    // 这里根据你们实际协议定义：
    // 如果题图标明“下发远程开锁指令(0x13)”是 模块->门锁，
    // 就在此函数发 0x13；也有人会用 0x12 当“远程开锁应答”。
    // 可以自行选择，下例以发 0x12(远程开锁应答)为例:

    uart_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.header[0] = 0xAA;
    pkt.header[1] = 0x55;
    pkt.command   = CMD_REMOTE_UNLOCK_RSP; // 0x12

    // data区可自行定义，如 data[0] 放 0x01=同意开锁? 也可都 0x00
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
/* =========== 具体指令处理函数 =========== */

/* (1) 处理消息上传(0x03) */
static void handle_msg_upload(const uart_packet_t *packet)
{
    uint8_t event     = packet->data[0];     // 事件值
    uint8_t eventinfo = packet->data[1];     // 事件信息
    // 如果需要用户类型/用户编号，也可解析 data[2]~data[4]

    ESP_LOGI(TAG, "[CMD=0x03] event=0x%02X, event_info=0x%02X", event, eventinfo);

    if (event == EVENT_UNLOCK_REQUEST) {
        // 远程开锁请求(0x03)
        if (s_remote_req_in_progress) {
            // 已经在请求中，可能重复
            ESP_LOGW(TAG, "Remote unlock request in progress, ignore");
            // 如果协议要应答，则可以发失败
            msg_upload_send_common_ack(false);
            return;
        }

        s_remote_req_in_progress = true;
        // 启动 60s 定时器
        xTimerStop(s_remote_req_timer, 0);
        xTimerStart(s_remote_req_timer, 0);

        // 上传到云 -> 这里使用 gs_mqtt_publish
        // 例如 topic = "/event/property/post", payload={"act":"remote_req"}
        char payload[64];
        snprintf(payload, sizeof(payload), "{\"cmd\":3,\"desc\":\"remote_req\"}");
        gs_mqtt_publish("/event/property/post",(uint8_t*)payload,strlen(payload),GS_MQTT_QOS0,0);

        ESP_LOGI(TAG, "Remote request upload done, waiting 60s for cloud => 0x13(开锁指令)");
        // 此时先不马上回 0x02，应根据需求来定：有些协议要求立即回 0x02=“收到”。
        // 这里演示：立即回“收到”。
        msg_upload_send_common_ack(true);

    } else if (event == EVENT_UNLOCKED) {
        // 已打开(0x01)
        if (s_unlocked_in_progress) {
            // 已有一个“已打开”事件在处理
            ESP_LOGW(TAG, "Unlocked event in progress, ignore");
            msg_upload_send_common_ack(false);
            return;
        }

        s_unlocked_in_progress = true;
        xTimerStop(s_unlocked_timer, 0);
        xTimerStart(s_unlocked_timer, 0);

        // 上传到云
        char payload[64];
        snprintf(payload, sizeof(payload), "{\"cmd\":3,\"desc\":\"unlocked\"}");
        gs_mqtt_publish("/event/property/post",(uint8_t*)payload,strlen(payload),GS_MQTT_QOS0,0);

        ESP_LOGI(TAG, "Unlocked event uploaded, waiting 12s for cloud response if needed");
        // 同理，先立即回个 0x02=成功接收
        msg_upload_send_common_ack(true);

    } else {
        // 其他事件(报警/提醒等)
        // 直接当普通12s事件处理也行。此处仅示例
        ESP_LOGI(TAG, "Other event=0x%02X, treat as normal 12s event...", event);
        // 可以复用 s_unlocked_timer，或者建一个通用定时器
        // ...
        msg_upload_send_common_ack(true);
    }
}

/* (2) 处理断电通知(0x04) */
static void handle_power_off_notify(void)
{
    ESP_LOGI(TAG, "[CMD=0x04] Power off notify => respond 0x02, then do cleanup...");

    // 先回指令=0x02
    msg_upload_send_common_ack(true);

    // 清理网络/停止 MQTT
    ESP_LOGI(TAG, "Disconnecting WiFi/MQTT ...");
    gs_mqtt_reset_config();  // 依项目需求, 或 cc_hal_wifi_sta_disconnect()
    // ...或者更具体的清理

    // 等待门锁MCU真正断电（硬件切断电源）
    // 如果需要软件重启(esp_restart)也可以在这里做
}

/* ------------------------------------------------------------- */
/* 远程开锁请求(60s)定时器超时回调 */
static void remote_req_timer_cb(TimerHandle_t xTimer)
{
    ESP_LOGW(TAG, "Remote unlock request timed out (60s) => no cloud response => fail");
    s_remote_req_in_progress = false;

    // 如果协议要求：需要通知门锁远程开锁失败，则发 0x02=失败
    // 也可以再发个 "CMD_MSG_UPLOAD_RSP" with data[0]=0x02
    // 具体看你们协议
    // 这里演示简单不再发
}

/* 已打开(12s)定时器超时回调 */
static void unlocked_timer_cb(TimerHandle_t xTimer)
{
    ESP_LOGW(TAG, "Unlocked event timed out (12s)");
    s_unlocked_in_progress = false;

    // 同理，如需其他操作可加
    // 一般是认为云端没有进一步响应，也不影响。
}

/* ------------------------------------------------------------- */
/* 给门锁回“消息上传应答(0x02)”的通用函数 */
static void msg_upload_send_common_ack(bool success)
{
    uart_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.header[0] = 0xAA;
    pkt.header[1] = 0x55;
    pkt.command   = CMD_MSG_UPLOAD_RSP;  // 0x02

    // data[0] = 0x00=成功, 0x02=失败
    pkt.data[0] = success ? 0x00 : 0x02;
    // 剩余 data 可以填协议版本等
    pkt.data[1] = 0x03; 
    // data[2..5] = 0
    pkt.checksum = uart_comm_calc_checksum((uint8_t*)&pkt, sizeof(pkt)-1);

    ESP_LOGI(TAG, "Send 0x02 ack => success=%d", success);
    uart_comm_send_packet(&pkt);
}
