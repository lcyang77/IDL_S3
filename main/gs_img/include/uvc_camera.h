// uvc_camera.h
/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_camera.h"
#include "esp_err.h"

/**
 * @brief  启动UVC摄像头（分配传输缓冲区，启动流、创建采集上传任务等）
 *         注意：只有在 Wi-Fi + MQTT 已准备就绪后才调用
 */
void uvc_camera_start(void);

/**
 * @brief  从UVC摄像头获取一帧
 * @note   同 esp_camera_fb_get() 功能类似
 *
 * @return camera_fb_t*   成功则返回帧指针，否则返回NULL
 */
camera_fb_t *esp_camera_fb_get(void);

/**
 * @brief  释放一帧
 * @param  fb  要释放的帧指针
 */
void esp_camera_fb_return(camera_fb_t *fb);

#ifdef __cplusplus
}
#endif
