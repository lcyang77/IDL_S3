#ifndef IMG_UPLOAD_H
#define IMG_UPLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

// 初始化图片上传模块，设置服务器URL等
esp_err_t img_upload_init(const char *server_url);

// 上传图片数据（JPEG 格式）
esp_err_t img_upload_send(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // IMG_UPLOAD_H
