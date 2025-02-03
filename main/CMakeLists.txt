# 设置最小的 CMake 版本
cmake_minimum_required(VERSION 3.5)

# 指定组件名称（可选）
set(COMPONENT_NAME "main")

# 显式列出源文件
set(SRCS
    main.c
    frame_parser.c
    product.c
    get_time.c
    gs/gs_bind.c
    gs/gs_device.c
    gs/gs_main.c
    gs/gs_mqtt.c
    gs/gs_ota.c
    gs/gs_wifi.c
    gs_img/img_upload.c           # 添加新的图片上传源文件
    gs_img/uvc_camera.c
    uart/net_uart_comm.c
    uart/net_sta.c
    uart/cc_event_reset.c
    uart/uart_config.c
    uart/msg_upload.c
    uart/unlock.c
    uart/img_transfer.c
    # 如果有其他源文件，继续添加
)

# 设置包含目录
set(INCLUDE_DIRS
    .
    gs/include
    gs_img/include
    uart/include
)

# 注册组件
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES json esp_http_client esp_http_server xfer_http
    PRIV_REQUIRES cc captive_portal flexible_button rbuffer
)
