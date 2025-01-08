/************************************************
 * File: cc_hal_ble.c (修改后示例)
 ************************************************/
#include "cc_hal_ble.h"
#include "cc_log.h"

#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

static char *TAG = "cc_hal_ble";

CC_EVENT_DEFINE_BASE(CC_HAL_BLE_EVENT);

/* 59462f12-9543-9999-12c8-58b459a2712d */
static const ble_uuid16_t gatt_svr_svc_sec_test_uuid = BLE_UUID16_INIT(0xFFE5);

/* 659e-897e-45e1-b016-007107c96df6 */
static const ble_uuid16_t gatt_svr_chr_sec_write_uuid = BLE_UUID16_INIT(0xFFF3);

/* 5c3a659e-897e-45e1-b016-007107c96df7 */
static const ble_uuid16_t gatt_svr_chr_sec_read_uuid = BLE_UUID16_INIT(0xFFF4);

static uint8_t g_ble_is_init = 0;
static cc_hal_ble_recv_cb_t g_ble_recv_cb = NULL;
static uint16_t g_ble_conn_handle = 0;

/** 
 * ============ 新增的全局变量 ============ 
 * 用于保证只有在 on_sync 就绪后，才真正开始广播
 */
static volatile int g_ble_is_sync = 0;      // 是否已完成 on_sync
static volatile int g_need_adv = 0;         // 标记：需要广播
static uint8_t g_adv_data[31];             // 缓存广播数据
static uint8_t g_scan_rsp_data[31];        // 缓存扫描响应数据
static uint8_t g_adv_len = 0;
static uint8_t g_scan_rsp_len = 0;

/* GATT Service / Characteristic 相关 */
uint16_t csc_notify_handle;
uint16_t csc_write_handle;

static int gap_event(struct ble_gap_event *event, void *arg);
static int gatt_svr_chr_access_sec_test(uint16_t conn_handle,
                                        uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt,
                                        void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: Security test */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_sec_test_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = &gatt_svr_chr_sec_write_uuid.u,
                .access_cb = gatt_svr_chr_access_sec_test,
                .val_handle = &csc_write_handle,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid = &gatt_svr_chr_sec_read_uuid.u,
                .access_cb = gatt_svr_chr_access_sec_test,
                .val_handle = &csc_notify_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            },
            {
                0, /* No more characteristics in this service. */
            },
        },
    },
    {
        0, /* No more services. */
    },
};

/* ========= 原有任务入口点 ========= */
static void __port_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* ========= on_reset 回调 (原逻辑不变) ========= */
static void on_reset(int reason) {
    CC_LOGE(TAG, "BLE on_reset (reason=%d)", reason);
}

/* 
 * ========= on_sync 回调：Host 就绪后会调用此函数 =========
 * 必须在这里才能确保 ble_gap_* 系列操作不会因为时机冲突而失败
 */
static void on_sync(void)
{
    int rc;
    uint8_t ble_addr_type;

    // NimBLE 内部完成了同步
    rc = ble_hs_id_infer_auto(0, &ble_addr_type);
    if (rc != 0) {
        CC_LOGE(TAG, "ble_hs_id_infer_auto rc=%d", rc);
    }

    /* 允许外部知道 BLE 已启动 */
    cc_event_post(CC_HAL_BLE_EVENT, CC_HAL_BLE_EVENT_ENABLED, NULL, 0);
    g_ble_is_init = 1;
    g_ble_is_sync = 1;

    CC_LOGI(TAG, "NimBLE on_sync() => BLE host is ready. ble_addr_type=%d", ble_addr_type);

    /* 
     * 如果在 on_sync 之前就调用过 cc_hal_ble_start_advzertising() 并设置了 g_need_adv，
     * 现在就可以真正设置广播数据并开始广播。 
     */
    if (g_need_adv) {
        CC_LOGI(TAG, "on_sync: Found pending adv => start now...");
        ble_gap_adv_set_data(g_adv_data, g_adv_len);
        ble_gap_adv_rsp_set_data(g_scan_rsp_data, g_scan_rsp_len);
        // 真正开始广播
        struct ble_gap_adv_params adv_params = {
            .conn_mode = BLE_GAP_CONN_MODE_UND,
            .disc_mode = BLE_GAP_DISC_MODE_GEN,
            .itvl_min  = 120,  // 可按需调整
            .itvl_max  = 240,
        };
        rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER,
                               &adv_params, gap_event, NULL);
        if (rc == 0) {
            CC_LOGI(TAG, "BLE advertising started (pending)...");
        } else {
            CC_LOGE(TAG, "ble_gap_adv_start error rc=%d", rc);
        }
        g_need_adv = 0;
    }
}

/* 
 * =========== gatt_svr_chr_access_sec_test 读写回调(与原逻辑几乎一致) ===========
 * 主要处理写入/通知等事件
 */
static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                              void *dst, uint16_t *len)
{
    uint16_t om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    int rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    return 0;
}

static int gatt_svr_chr_access_sec_test(uint16_t conn_handle,
                                        uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt,
                                        void *arg)
{
    int rc;
    const ble_uuid_t *uuid;
    static uint8_t gatt_svr_sec_recv_buf[20];
    uint16_t read_len = 0;

    uuid = ctxt->chr->uuid;

    if (ble_uuid_cmp(uuid, &gatt_svr_chr_sec_write_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            rc = gatt_svr_chr_write(ctxt->om, 1,
                                    sizeof(gatt_svr_sec_recv_buf),
                                    gatt_svr_sec_recv_buf,
                                    &read_len);
            if (rc == 0 && g_ble_recv_cb && read_len > 0) {
                g_ble_recv_cb(gatt_svr_sec_recv_buf, read_len);
            }
            return rc;
        } else {
            return BLE_ATT_ERR_UNLIKELY;
        }
    }
    return BLE_ATT_ERR_UNLIKELY;
}

/* 
 * =========== 启动广播的核心逻辑：只缓存，真正启动放到 on_sync() 或同步后 ===========
 */
cc_err_t cc_hal_ble_start_advzertising(uint8_t *adv_data, uint8_t adv_len,
                                       uint8_t *scan_rsp_data, uint8_t scan_rsp_len)
{
    // 长度检查
    if (adv_len > 31 || scan_rsp_len > 31) {
        CC_LOGE(TAG, "advertising data too long");
        return CC_ERR_INVALID_ARG;
    }

    // 先存储
    memcpy(g_adv_data, adv_data, adv_len);
    g_adv_len = adv_len;

    if (scan_rsp_data && scan_rsp_len > 0) {
        memcpy(g_scan_rsp_data, scan_rsp_data, scan_rsp_len);
        g_scan_rsp_len = scan_rsp_len;
    } else {
        g_scan_rsp_len = 0;
    }

    // 如果已经 on_sync，就可直接开始；否则先设置 g_need_adv=1，等待 on_sync 时机
    if (g_ble_is_sync) {
        CC_LOGI(TAG, "BLE already sync => start adv immediately...");
        int rc = ble_gap_adv_set_data(g_adv_data, g_adv_len);
        if (rc != 0) {
            CC_LOGE(TAG, "ble_gap_adv_set_data rc=%d", rc);
            return CC_FAIL;
        }
        rc = ble_gap_adv_rsp_set_data(g_scan_rsp_data, g_scan_rsp_len);
        if (rc != 0) {
            CC_LOGE(TAG, "ble_gap_adv_rsp_set_data rc=%d", rc);
            return CC_FAIL;
        }

        // 真正开始广播
        struct ble_gap_adv_params adv_params = {
            .conn_mode = BLE_GAP_CONN_MODE_UND,
            .disc_mode = BLE_GAP_DISC_MODE_GEN,
            .itvl_min  = 120,
            .itvl_max  = 240,
        };
        rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                               &adv_params, gap_event, NULL);
        if (rc != 0) {
            CC_LOGE(TAG, "ble_gap_adv_start rc=%d => stop advertising", rc);
            return CC_FAIL;
        }
        g_need_adv = 0;
    } else {
        // 还未 on_sync，等待 on_sync 时再自动开始广播
        g_need_adv = 1;
        CC_LOGI(TAG, "BLE not sync yet, will start adv once on_sync is called");
    }

    return CC_OK;
}

/* ============== 停止广播(如需要) ============== */
cc_err_t cc_hal_ble_stop_advzertising(void){
    int rc = ble_gap_adv_stop();
    if (rc != 0){
        CC_LOGE(TAG, "ble_gap_adv_stop rc=%d", rc);
        return CC_FAIL;
    }
    g_need_adv = 0;
    return CC_OK;
}

/* ============== 发通知示例 ============== */
uint16_t cc_hal_ble_send(uint8_t *data, uint16_t len)
{
    if (0 == len || NULL == data) {
        return CC_ERR_INVALID_ARG;
    }
    for (size_t idx = 0; idx < len; idx += 20){
        struct os_mbuf *om = ble_hs_mbuf_from_flat(data + idx,
                                                   (len - idx > 20) ? 20 : (len - idx));
        ble_gattc_notify_custom(g_ble_conn_handle, csc_notify_handle, om);
    }
    return CC_OK;
}

/* ============== BLE 连接断开等 GAP 事件 ============== */
static int gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            if (rc == 0) {
                CC_LOGI(TAG, "BLE connected, conn_handle=%d", event->connect.conn_handle);
            }
        } else {
            // 连接失败 => resume advertising
            cc_hal_ble_start_advzertising(g_adv_data, g_adv_len, g_scan_rsp_data, g_scan_rsp_len);
        }
        g_ble_conn_handle = event->connect.conn_handle;
        cc_event_post(CC_HAL_BLE_EVENT, CC_HAL_BLE_EVENT_CONNECTED, NULL, 0);
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        CC_LOGI(TAG, "BLE disconnected, reason=%d => re-adv?", event->disconnect.reason);
        cc_event_post(CC_HAL_BLE_EVENT, CC_HAL_BLE_EVENT_DISCONNECTED, NULL, 0);
        // 断开后如需继续广播，可再次调用 start
        cc_hal_ble_start_advzertising(g_adv_data, g_adv_len, g_scan_rsp_data, g_scan_rsp_len);
        return 0;

    default:
        break;
    }
    return 0;
}

/* ============== 初始化 / 去初始化 ============== */
cc_err_t cc_hal_ble_init(cc_hal_ble_recv_cb_t recv_cb)
{
    int rc;

    nimble_port_init(); // 初始化 NimBLE

    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;

    // 注册 GATT 服务
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return CC_FAIL;
    }
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return CC_FAIL;
    }

    // 设置默认 GAP 设备名，可再改
    ble_svc_gap_device_name_set("CC");

    // 启动 NimBLE 主任务
    nimble_port_freertos_init(__port_task);

    g_ble_recv_cb = recv_cb;

    g_ble_is_init = 0;
    g_ble_is_sync = 0;
    g_need_adv    = 0;
    return CC_OK;
}

cc_err_t cc_hal_ble_deinit(void)
{
    // 先停播
    ble_gap_adv_stop();
    g_need_adv = 0;

    // 停 nimble 线程
    nimble_port_stop();
    nimble_port_deinit();

    g_ble_recv_cb   = NULL;
    g_ble_is_init   = 0;
    g_ble_is_sync   = 0;
    g_ble_conn_handle = 0;
    return CC_OK;
}
