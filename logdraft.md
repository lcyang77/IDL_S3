
I (12801) app_main: MQTT birth messages all sent, start UVC camera now...
I (12891) USB_STREAM: UVC Streaming Config Succeed, Version: 1.5.0
I (12973) USB_STREAM: USB streaming callback register succeed
I (13050) USB_STREAM: Pre-alloc ctrl urb succeed, size = 1024
I (13128) USB_STREAM: USB stream task start
I (13158) USB_STREAM: USB Streaming Start Succeed
I (13250) USB_STREAM: Waiting USB Device Connection
I (13408) USB_STREAM: line 151 HCD_PORT_EVENT_CONNECTION
I (13408) USB_STREAM: Action: ACTION_DEVICE_CONNECT
I (13505) USB_STREAM: Resetting Port
I (13566) USB_STREAM: Setting Port FIFO, 1
I (13566) USB_STREAM: USB Speed: full-speed
I (13622) USB_STREAM: ENUM Stage START, Succeed
I (13686) USB_STREAM: ENUM Stage GET_SHORT_DEV_DESC, Succeed
I (13762) USB_STREAM: Default pipe endpoint MPS update to 64
I (13837) USB_STREAM: ENUM Stage CHECK_SHORT_DEV_DESC, Succeed
I (13916) USB_STREAM: ENUM Stage SET_ADDR, Succeed
I (13991) USB_STREAM: ENUM Stage CHECK_ADDR, Succeed
I (14049) USB_STREAM: ENUM Stage GET_FULL_DEV_DESC, Succeed
I (14124) USB_STREAM: ENUM Stage CHECK_FULL_DEV_DESC, Succeed
I (14201) USB_STREAM: ENUM Stage GET_SHORT_CONFIG_DESC, Succeed
I (14280) USB_STREAM: ENUM Stage CHECK_SHORT_CONFIG_DESC, Succeed
I (14362) USB_STREAM: ENUM Stage GET_FULL_CONFIG_DESC, Succeed
# W (14440) USB_STREAM: 描述符不匹配：声明的帧描述符数量 = 7，实际提供的帧描述符数量 = 8
I (14571) USB_STREAM: Actual VS Interface(MPS <= 512) found, interface = 1, alt = 0
I (14671) USB_STREAM:   Endpoint(BULK) Addr = 0x81, MPS = 64
I (14746) USB_STREAM: Actual MJPEG format index, format index = 1, contains 7 frames
I (14847) USB_STREAM: Actual Frame: 7, width*height: 1280*720, frame index = 6
I (14942) USB_STREAM: ENUM Stage CHECK_FULL_CONFIG_DESC, Succeed
I (15023) USB_STREAM: ENUM Stage SET_CONFIG, Succeed
I (15091) uvc_camera_module: UVC Device connected
I (15155) USB_STREAM: Probe Format(1), Frame(6) 1280*720, interval(666666)
I (15245) USB_STREAM: Probe payload size = 2048
I (15466) USB_STREAM: dwMaxPayloadTransferSize set = 2048, probed = 768000
I (15467) USB_STREAM: Sample processing task start
I (15487) USB_STREAM: USB Device Connected
I (15546) uvc_camera_module: UVC camera initialization done.
I (15622) product: __event_handler: GS_MQTT_EVENT event: 0
I (15735) hal_network: Event dispatched from event loop base=MQTT_EVENTS, event_id=3
I (15797) hal_network: Other event id:3
W (57585) USB_STREAM: line 155 HCD_PORT_EVENT_DISCONNECTION
I (57586) USB_STREAM: Recover Stream Task
I (57586) USB_STREAM: usb stream task wait reset
I (57646) USB_STREAM: usb stream task reset, reason: device recover
I (57730) USB_STREAM: Resetting UVC pipe
I (57785) USB_STREAM: sample processing stop
I (57844) USB_STREAM: Sample processing task deleted
I (57962) uvc_camera_module: UVC Device disconnected
I (57980) USB_STREAM: Action: ACTION_PIPE_DFLT_DISABLE
I (58050) USB_STREAM: Action: ACTION_PORT_RECOVER
I (58114) USB_STREAM: Action: ACTION_DEVICE_DISCONN
I (58181) USB_STREAM: Waiting USB Connection
I (61262) USB_STREAM: line 151 HCD_PORT_EVENT_CONNECTION
I (61263) USB_STREAM: Action: ACTION_DEVICE_CONNECT
I (61317) USB_STREAM: Resetting Port
I (61378) USB_STREAM: Setting Port FIFO, 1
I (61378) USB_STREAM: USB Speed: full-speed
I (61434) USB_STREAM: ENUM Stage START, Succeed
I (61497) USB_STREAM: ENUM Stage GET_SHORT_DEV_DESC, Succeed
I (61573) USB_STREAM: Default pipe endpoint MPS update to 64
I (61648) USB_STREAM: ENUM Stage CHECK_SHORT_DEV_DESC, Succeed
I (61728) USB_STREAM: ENUM Stage SET_ADDR, Succeed
I (61803) USB_STREAM: ENUM Stage CHECK_ADDR, Succeed
I (61861) USB_STREAM: ENUM Stage GET_FULL_DEV_DESC, Succeed
I (61936) USB_STREAM: ENUM Stage CHECK_FULL_DEV_DESC, Succeed
I (62013) USB_STREAM: ENUM Stage GET_SHORT_CONFIG_DESC, Succeed
I (62092) USB_STREAM: ENUM Stage CHECK_SHORT_CONFIG_DESC, Succeed
I (62173) USB_STREAM: ENUM Stage GET_FULL_CONFIG_DESC, Succeed
# W (62252) USB_STREAM: 描述符不匹配：声明的帧描述符数量 = 7，实际提供的帧描述符数量 = 8
I (62383) USB_STREAM: Actual VS Interface(MPS <= 512) found, interface = 1, alt = 0
I (62483) USB_STREAM:   Endpoint(BULK) Addr = 0x81, MPS = 64
I (62558) USB_STREAM: Actual MJPEG format index, format index = 1, contains 7 frames
I (62659) USB_STREAM: Actual Frame: 7, width*height: 1280*720, frame index = 6
I (62754) USB_STREAM: ENUM Stage CHECK_FULL_CONFIG_DESC, Succeed
I (62835) USB_STREAM: ENUM Stage SET_CONFIG, Succeed
I (62903) uvc_camera_module: UVC Device connected
I (62966) USB_STREAM: Probe Format(1), Frame(6) 1280*720, interval(666666)
I (63057) USB_STREAM: Probe payload size = 2048
I (63123) USB_STREAM: dwMaxPayloadTransferSize set = 2048, probed = 768000
I (63210) USB_STREAM: Sample processing task start
I (66389) uvc_camera_module: UVC frame size: 100372 bytes
I (66437) img_upload: HTTP_EVENT_ON_CONNECTED
I (66440) img_upload: HTTP_EVENT_HEADER_SENT
I (67639) img_upload: HTTP_EVENT_ON_HEADER, key=Server, value=nginx
I (67639) img_upload: HTTP_EVENT_ON_HEADER, key=Date, value=Fri, 03 Jan 2025 08:03:28 GMT
I (67696) img_upload: HTTP_EVENT_ON_HEADER, key=Content-Type, value=text/html; charset=UTF-8
I (67805) img_upload: HTTP_EVENT_ON_HEADER, key=Transfer-Encoding, value=chunked
I (67902) img_upload: HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
I (67994) img_upload: HTTP_EVENT_ON_HEADER, key=Vary, value=Accept-Encoding
I (68086) img_upload: HTTP_EVENT_ON_HEADER, key=X-Powered-By, value=PHP/5.6.40
I (68181) img_upload: HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Origin, value=*
I (68282) img_upload: HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Headers, value=x-requested-with,content-type,Authorization
I (68427) img_upload: HTTP_EVENT_ON_DATA, len=104
I (68492) img_upload: Response data: {"error":0,"img_url":"https:\/\/gaoshi.wdaoyun.cn\/static\/pic\/2025-01-03\/677799d08331e832014561.jpg"}
I (68652) img_upload: HTTP response code: 200
I (68713) img_upload: HTTP_EVENT_DISCONNECTED
I (68775) img_upload: HTTP_EVENT_DISCONNECTED
I (68834) uvc_camera_module: UVC frame upload success
I (70643) uart_comm: Received 1 bytes of data
I (70644) uart_comm: Raw data: 18 


























I (14393) USB_STREAM: UVC Streaming Config Succeed, Version: 1.5.0
I (14476) USB_STREAM: USB streaming callback register succeed
I (14553) USB_STREAM: Pre-alloc ctrl urb succeed, size = 1024
I (14631) USB_STREAM: USB stream task start
I (14661) USB_STREAM: USB Streaming Start Succeed
I (14753) USB_STREAM: Waiting USB Device Connection
I (14911) USB_STREAM: line 151 HCD_PORT_EVENT_CONNECTION
I (14911) USB_STREAM: Action: ACTION_DEVICE_CONNECT
I (15008) USB_STREAM: Resetting Port
I (15069) USB_STREAM: Setting Port FIFO, 1
I (15069) USB_STREAM: USB Speed: full-speed
I (15125) USB_STREAM: ENUM Stage START, Succeed
I (15188) USB_STREAM: ENUM Stage GET_SHORT_DEV_DESC, Succeed
I (15264) USB_STREAM: Default pipe endpoint MPS update to 64
I (15339) USB_STREAM: ENUM Stage CHECK_SHORT_DEV_DESC, Succeed
I (15418) USB_STREAM: ENUM Stage SET_ADDR, Succeed
I (15494) USB_STREAM: ENUM Stage CHECK_ADDR, Succeed
I (15551) USB_STREAM: ENUM Stage GET_FULL_DEV_DESC, Succeed
I (15626) USB_STREAM: ENUM Stage CHECK_FULL_DEV_DESC, Succeed
I (15704) USB_STREAM: ENUM Stage GET_SHORT_CONFIG_DESC, Succeed
I (15783) USB_STREAM: ENUM Stage CHECK_SHORT_CONFIG_DESC, Succeed
I (15864) USB_STREAM: ENUM Stage GET_FULL_CONFIG_DESC, Succeed
# W (15942) USB_STREAM: 描述符不匹配：声明的帧描述符数量 = 7，实际提供的帧描述符数量 = 8
I (16074) USB_STREAM: Actual VS Interface(MPS <= 512) found, interface = 1, alt = 0
I (16173) USB_STREAM:   Endpoint(BULK) Addr = 0x81, MPS = 64
I (16248) USB_STREAM: Actual MJPEG format index, format index = 1, contains 7 frames
I (16349) USB_STREAM: Actual Frame: 7, width*height: 1280*720, frame index = 6
I (16444) USB_STREAM: ENUM Stage CHECK_FULL_CONFIG_DESC, Succeed
I (16525) USB_STREAM: ENUM Stage SET_CONFIG, Succeed
I (16593) uvc_camera_module: UVC Device connected
I (16657) USB_STREAM: Probe Format(1), Frame(6) 1280*720, interval(666666)
I (16747) USB_STREAM: Probe payload size = 2048
I (16813) USB_STREAM: dwMaxPayloadTransferSize set = 2048, probed = 768000
I (16901) USB_STREAM: Sample processing task start
I (16921) USB_STREAM: USB Device Connected
I (17023) uvc_camera_module: UVC camera initialization done.
I (17099) product: __event_handler: GS_MQTT_EVENT event: 0
I (17229) hal_network: Event dispatched from event loop base=MQTT_EVENTS, event_id=3
I (17274) hal_network: Other event id:3







# W (315870) USB_STREAM: line 155 HCD_PORT_EVENT_DISCONNECTION
I (315871) USB_STREAM: Recover Stream Task
I (315871) USB_STREAM: usb stream task wait reset
I (315935) USB_STREAM: usb stream task reset, reason: device recover
I (316019) USB_STREAM: Resetting UVC pipe
I (316075) USB_STREAM: sample processing stop
I (316136) USB_STREAM: Sample processing task deleted
I (316254) uvc_camera_module: UVC Device disconnected
I (316274) USB_STREAM: Action: ACTION_PIPE_DFLT_DISABLE
I (316344) USB_STREAM: Action: ACTION_PORT_RECOVER
I (316410) USB_STREAM: Action: ACTION_DEVICE_DISCONN
I (316477) USB_STREAM: Waiting USB Connection
I (319230) USB_STREAM: line 151 HCD_PORT_EVENT_CONNECTION
I (319231) USB_STREAM: Action: ACTION_DEVICE_CONNECT
I (319287) USB_STREAM: Resetting Port
I (319349) USB_STREAM: Setting Port FIFO, 1
I (319349) USB_STREAM: USB Speed: full-speed
I (319407) USB_STREAM: ENUM Stage START, Succeed
I (319472) USB_STREAM: ENUM Stage GET_SHORT_DEV_DESC, Succeed
I (319549) USB_STREAM: Default pipe endpoint MPS update to 64
I (319625) USB_STREAM: ENUM Stage CHECK_SHORT_DEV_DESC, Succeed
I (319705) USB_STREAM: ENUM Stage SET_ADDR, Succeed
I (319782) USB_STREAM: ENUM Stage CHECK_ADDR, Succeed
I (319840) USB_STREAM: ENUM Stage GET_FULL_DEV_DESC, Succeed
I (319916) USB_STREAM: ENUM Stage CHECK_FULL_DEV_DESC, Succeed
I (319994) USB_STREAM: ENUM Stage GET_SHORT_CONFIG_DESC, Succeed
I (320075) USB_STREAM: ENUM Stage CHECK_SHORT_CONFIG_DESC, Succeed
I (320157) USB_STREAM: ENUM Stage GET_FULL_CONFIG_DESC, Succeed
# W (320236) USB_STREAM: 描述符不匹配：声明的帧描述符数量 = 7，实际提供的帧描述符数量 = 8
I (320369) USB_STREAM: Actual VS Interface(MPS <= 512) found, interface = 1, alt = 0
I (320470) USB_STREAM:  Endpoint(BULK) Addr = 0x81, MPS = 64
I (320546) USB_STREAM: Actual MJPEG format index, format index = 1, contains 7 frames
I (320648) USB_STREAM: Actual Frame: 7, width*height: 1280*720, frame index = 6
I (320744) USB_STREAM: ENUM Stage CHECK_FULL_CONFIG_DESC, Succeed
I (320826) USB_STREAM: ENUM Stage SET_CONFIG, Succeed
I (320895) uvc_camera_module: UVC Device connected
I (320959) USB_STREAM: Probe Format(1), Frame(6) 1280*720, interval(666666)
I (321051) USB_STREAM: Probe payload size = 2048
I (321115) USB_STREAM: dwMaxPayloadTransferSize set = 2048, probed = 768000
I (321206) USB_STREAM: Sample processing task start
I (324387) uvc_camera_module: UVC frame size: 101360 bytes
I (324424) img_upload: HTTP_EVENT_ON_CONNECTED
I (324427) img_upload: HTTP_EVENT_HEADER_SENT
I (325517) img_upload: HTTP_EVENT_ON_HEADER, key=Server, value=nginx
I (325518) img_upload: HTTP_EVENT_ON_HEADER, key=Date, value=Fri, 03 Jan 2025 10:05:59 GMT
I (325576) img_upload: HTTP_EVENT_ON_HEADER, key=Content-Type, value=text/html; charset=UTF-8
I (325687) img_upload: HTTP_EVENT_ON_HEADER, key=Transfer-Encoding, value=chunked
I (325784) img_upload: HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
I (325878) img_upload: HTTP_EVENT_ON_HEADER, key=Vary, value=Accept-Encoding
I (325971) img_upload: HTTP_EVENT_ON_HEADER, key=X-Powered-By, value=PHP/5.6.40
I (326067) img_upload: HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Origin, value=*
I (326169) img_upload: HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Headers, value=x-requested-with,content-type,Authorization
I (326315) img_upload: HTTP_EVENT_ON_DATA, len=104
I (326381) img_upload: Response data: {"error":0,"img_url":"https:\/\/gaoshi.wdaoyun.cn\/static\/pic\/2025-01-03\/6777b686eb1d8068294962.jpg"}
I (326543) img_upload: HTTP response code: 200
I (326604) img_upload: HTTP_EVENT_DISCONNECTED
I (326667) img_upload: HTTP_EVENT_DISCONNECTED
I (326727) uvc_camera_module: UVC frame upload success


























[2025-01-06 20:19:17.104]
RX：X?蜕叺? VCC          : 0x01 (3V)[0m
[0;32mI (1986) octal_psram: SRF          : 0x01 (Fast Refresh)[0m
[0;32mI (2058) octal_psram: BurstType    : 0x00 ( Wrap)[0m
[0;32mI (2122) octal_psram: BurstLen     : 0x03 (1024 Byte)[0m
[0;32mI (2191) octal_psram: Readlatency  : 0x02 (10 cycles@Fixed)[0m
[0;32mI (2266) octal_psram: DriveStrength: 0x00 (1/1)[0m
[0;32mI (2329) MSPI Timing: PSRAM timing tuning index: 5[0m
[0;32mI (2394) esp_psram: Found 8MB PSRAM device[0m
[0;32mI (2451) esp_psram: Speed: 80MHz[0m
[0;32mI (2498) cpu_start: Multicore app[0m
[0;32mI (3016) esp_psram: SPI SRAM memory test OK[0m
[0;32mI (3075) cpu_start: Pro cpu start user code[0m
[0;32mI (3075) cpu_start: cpu freq: 240000000 Hz[0m
[0;32mI (3076) app_init: Application information:[0m
[0;32mI (3115) app_init: Project name:     IDL_S3[0m
[0;32mI (3173) app_init: App version:      2d7c032-dirty[0m
[0;32mI (3238) app_init: Compile time:     Jan  6 2025 20:14:45[0m
[0;32mI (3311) app_init: ELF file SHA256:  6ecd04fce...[0m
[0;32mI (3376) app_init: ESP-IDF:          v5.3.2[0m
[0;32mI (3434) efuse_init: Min chip rev:     v0.0[0m
[0;32mI (3493) efuse_init: Max chip rev:     v0.99 [0m
[0;32mI (3553) efuse_init: Chip rev:         v0.2[0m
[0;32mI (3611) heap_init: Initializing. RAM available for dynamic allocation:[0m
[0;32mI (3699) heap_init: At 3FCADBA8 len 0003BB68 (238 KiB): RAM[0m
[0;32mI (3774) heap_init: At 3FCE9710 len 00005724 (21 KiB): RAM[0m
[0;32mI (3848) heap_init: At 3FCF0000 len 00008000 (32 KiB): DRAM[0m
[0;32mI (3923) heap_init: At 600FE100 len 00001EE8 (7 KiB): RTCRAM[0m
[0;32mI (3999) esp_psram: Adding pool of 7680K of PSRAM memory to heap allocator[0m
[0;32mI (4090) spi_flash: detected chip: gd[0m
[0;32mI (4142) spi_flash: flash io: dio[0m
[0;32mI (4190) sleep: Configure to isolate all GPIO pins in sleep state[0m
[0;32mI (4271) sleep: Enable automatic switching of GPIO sleep configuration[0m
[0;32mI (4357) coexist: coex firmware version: cbb41d7[0m
[0;32mI (4422) coexist: coexist rom version e7ae62f[0m
[0;32mI (4483) main_task: Started on CPU0[0m
[0;32mI (4533) esp_psram: Reserving pool of 32K of internal memory for DMA/internal allocations[0m
[0;32mI (4638) main_task: Calling app_main()[0m
[0;32mI (4713) pp: pp rom version: e7ae62f[0m
[0;32mI (4743) net80211: net80211 rom version: e7ae62f[0m
I (4808) wifi:wifi driver task: 3fcbfce8, prio:23, stack:6656, core=0
I (4890) wifi:wifi firmware version: b0fd6006b
I (4930) wifi:wifi certification version: v7.0
I (4980) wifi:config NVS flash: enabled
I (5023) wifi:config nano formating: disabled
I (5072) wifi:Init data frame dynamic rx buffer num: 32
I (5132) wifi:Init static rx mgmt buffer num: 5
I (5183) wifi:Init management short buffer num: 32
I (5237) wifi:Init static tx buffer num: 16
I (5284) wifi:Init tx cache buffer num: 32
I (5329) wifi:Init static tx FG buffer num: 2
I (5378) wifi:Init static rx buffer size: 1600
I (5428) wifi:Init static rx buffer num: 10
I (5475) wifi:Init dynamic rx buffer num: 32
[0;32mI (5524) wifi_init: rx ba win: 6[0m
[0;32mI (5570) wifi_init: accept mbox: 6[0m
[0;32mI (5619) wifi_init: tcpip mbox: 32[0m
[0;32mI (5668) wifi_init: udp mbox: 6[0m
[0;32mI (5714) wifi_init: tcp mbox: 6[0m
[0;32mI (5760) wifi_init: tcp tx win: 5760[0m
[0;32mI (5811) wifi_init: tcp rx win: 5760[0m
[0;32mI (5862) wifi_init: tcp mss: 1440[0m
[0;32mI (5910) wifi_init: WiFi IRAM OP enabled[0m
[0;32mI (5965) wifi_init: WiFi RX IRAM OP enabled[0m
[0;32mI (6023) hal_kvs: Get gs_product_key blob[0m
[0;32mI (6080) hal_kvs: Get gs_device_name blob[0m
[0;32mI (6136) hal_kvs: Get gs_device_secre blob[0m
[0;32mI (6193) gs_device: read dev triple product_key: lakdjfXsIwP, device_name: DUehPXElKuAy0Rii, device_secret: R6bWiPjwrLt6xFo_os73fBeQWEdNkmV1[0m
gs_product_key: lakdjfXsIwP
gs_device_name: DUehPXElKuAy0Rii
gs_device_secret: R6bWiPjwrLt6xFo_os73fBeQWEdNkmV1
[0;32mI (6472) gs_device: gs_device_set_version sw: 1.21.0.0 hw: 1.0.0[0m
[0;32mI (6552) hal_kvs: Get gs_wifi blob[0m
[0;32mI (6602) gs_wifi: read wifi config ssid: ASUS Wi-Fi7, password: y2756096[0m
[0;32mI (6690) hal_kvs: Get gs_bind blob[0m
[0;32mI (6739) hal_kvs: Get gs_bind_boot blob[0m
[0;32mI (6793) hal_kvs: Get gs_mqtt blob[0m
[0;32mI (6842) gs_mqtt: read mqtt config 120.25.207.32:1883[0m
[0;32mI (6911) product: We have Wi-Fi config but won't connect now (waiting for 0x01)...[0m
[0;32mI (7011) hal_kvs: Get gs_product_key blob[0m
[0;32mI (7066) hal_kvs: Get gs_device_name blob[0m
[0;32mI (7123) hal_kvs: Get gs_device_secre blob[0m
[0;32mI (7180) gs_device: read dev triple product_key: lakdjfXsIwP, device_name: DUehPXElKuAy0Rii, device_secret: R6bWiPjwrLt6xFo_os73fBeQWEdNkmV1[0m
gs_product_key: lakdjfXsIwP
gs_device_name: DUehPXElKuAy0Rii
gs_device_secret: R6bWiPjwrLt6xFo_os73fBeQWEdNkmV1
[0;32mI (7459) get_time_module: get_time_init done. device_name=DefaultName[0m
[0;32mI (7544) app_main: ===> DETECTED BIND STATUS CHANGED TO TRUE, fetch network time now...[0m
[0;32mI (7648) app_main: Wi-Fi config is currently saved => will connect after 0x01 from MCU...[0m
[0;32mI (7754) img_upload: img_upload_init: server_url=http://120.25.207.32:3466/upload/ajaxuploadfile.php[0m
[0;32mI (7872) img_upload: img_upload_init: auth_header=secret {5c627423c152a8717eb659107ba7549c}[0m
[0;32mI (7980) app_main: img_upload module initialized,[0;32mI (8171) uart: queue free spaces: 20[0m
[0;32mI (8172) uart_comm: UART event task started[0m
[0;32mI (8172) uart_comm: UART communication initialized successfully[0m
[0;32mI (8227) app_main: UART communication initialized[0m
[0;32mI (8292) uart_comm: Registering new packet callback[0m

[2025-01-06 20:19:20.451]
RX：[0;32mI (9548) get_time_module: FetchTime -> http://gaoshi.wdaoyun.cn/mqtt/getTime.php?device_name=DefaultName[0m
[0;32mI (9558) cc_http: http_begin url: http://gaoshi.wdaoyun.cn/mqtt/getTime.php?device_name=DefaultName[0m
[0;32mI (9673) cc_http: http_start cnt: 1 url: http://gaoshi.wdaoyun.cn/mqtt/getTime.php?device_name=DefaultName[0m
[0;32mI (9777) HAL_OS_TASK: Attempting to create task: hal_http0[0m
[0;32mI (9850) HAL_OS_TASK: Task parameters: stack_size=3072, priority=4[0m
[0;32mI (9933) HAL_OS_TASK: Current free heap size: 136243 bytes[0m
[0;32mI (10007) HAL_OS_TASK: Successfully created task: hal_http0[0m
[http_tcp_conn_wrapper 98] hostinfo is null, return EDNS  [0;32mI (10082) HAL_OS_TASK: Task handle: 0x3fcdfaf4[0m
[0;32mI (10145) cc_http: CC_HTTP_EVENT_ERROR: 252[0m
[0;32mI (10206) HAL_OS_TASK: Remaining free heap size: 132687 bytes[0m
[0;32mI (10264) cc_http: http ResponseCode: 0   resp_len: 0[0m
[0;32mI (10411) cc_http: http_restart url: http://gaoshi.wdaoyun.cn/mqtt/getTime.php?device_name=DefaultName[0m
[0;32mI (10560) cc_http: http_start cnt: 2 url: http://gaoshi.wdaoyun.cn/mqtt/getTime.php?device_name=DefaultName[0m
[0;32mI (10655) HAL_OS_TASK: Attempting to create task: hal_http0[0m
[0;32mI (10730) HAL_OS_TASK: Task parameters: stack_size=3072, priority=4[0m
[0;32mI (10813) HAL_OS_TASK: Current free heap size: 136243 bytes[0m
[0;32mI (10888) HAL_OS_TASK: Successfully created task: hal_http0[0m
[http_tcp_conn_wrapper 98] hostinfo is null, return EDNS  [0;32mI (10963) HAL_OS_TASK: Task handle: 0x3fcdfaf4[0m
[0;32mI (11026) cc_http: CC_HTTP_EVENT_ERROR: 252[0m
[0;32mI (11087) HAL_OS_TASK: Remaining free heap size: 132687 bytes[0m
[0;32mI (11146) cc_http: http ResponseCode: 0   resp_len: 0[0m
[0;32mI (11292) cc_http: http_restart url: http://gaoshi.wdaoyun.cn/mqtt/getTime.php?device_name=DefaultName[0m
[0;32mI (11441) cc_http: http_start cnt: 3 url: http://gaoshi.wdaoyun.cn/mqtt/getTime.php?device_name=DefaultName[0m
[0;31mE (11536) get_time_module: HTTP resp_code=-1[0m
[0;32mI (11605) cc_http: http_stop url: http://gaoshi.wdaoyun.cn/mqtt/getTime.php?device_name=DefaultName[0m

[2025-01-06 20:19:36.379]
TX：AA550116090001000020
[2025-01-06 20:19:42.128]
RX：[0;32mI (28801) uart_comm: Received 10 bytes of data[0m
[0;32mI (28801) uart_comm: Raw data: AA 55 01 16 09 00 01 00 00 20 [0m
[0;32mI (28806) uart_comm: Received valid packet, CMD=0x01[0m
[0;32mI (28874) uart_comm: ====== Received Packet Details ======[0m
[0;32mI (28948) uart_comm: Header: 0xAA 0x55[0m
[0;32mI (29001) uart_comm: Command: 0x01[0m
[0;32mI (29050) uart_comm: Data: 16 09 00 01 00 00[0m
[0;32mI (29109) uart_comm: Checksum: 0x20[0m
[0;32mI (29159) uart_comm: ================================[0m
[0;32mI (29228) app_main: uart_packet_received: CMD=0x01[0m
[0;32mI (29293) app_main: Got CMD_WIFI_CONFIG (0x01) => do_wifi_connect_or_config...[0m
[0;32mI (29388) app_main: [do_wifi_connect_or_config] Detected existing Wi-Fi config => connect now[0m
[0;32mI (29499) cc_hal_wifi: Connecting to Wi-Fi:ASUS Wi-Fi7, PSW:y2756096[0m
[0;32mI (29584) phy_init: phy_version 680,a6008b2,Jun  4 2024,16:41:10[0m
I (29695) wifi:mode : sta (dc:da:0c:1e:2e:78)
I (29712) wifi:enable tsf
[0;32mI (29742) gs_wifi: gs_wifi_sta_start_connect ssid: ASUS Wi-Fi7, password: y2756096[0m
I (29751) wifi:new:<1,0>, old:<1,0>, ap:<255,255>, sta:<1,0>, prof:1, snd_ch_cfg:0x0
I (29929) wifi:state: init -> auth (0xb0)
[0;32mI (29975) gs_mqtt: __event_handler: GS_WIFI_EVENT event: 0[0m
I (29987) wifi:state: auth -> assoc (0x0)
I (30097) wifi:state: assoc -> run (0x10)
I (30168) wifi:connected with ASUS Wi-Fi7, aid = 1, channel 1, BW20, bssid = 42:e4:24:d9:ee:64
I (30237) wifi:security: WPA2-PSK, phy: bgn, rssi: -16
I (30297) wifi:pm start, type: 1

I (30333) wifi:dp: 1, bi: 102400, li: 3, scale listen interval from 307200 us to 307200 us
I (30429) wifi:set rx beacon pti, rx_bcn_pti: 14, bcn_timeout: 25000, mt_pti: 14, mt_time: 10000
I (30532) wifi:AP's beacon interval = 102400 us, DTIM period = 1
[0;32mI (30602) gs_wifi: wifi connect[0m
[0;32mI (30646) gs_mqtt: __event_handler: GS_WIFI_EVENT event: 1[0m
[0;32mI (30839) app_main: WiFi connect/config done? -> send CMD_WIFI_RESPONSE (0x02) ack => success=WIFI_CONFIG_SUCCESS (0x00)[0m
[0;32mI (30858) uart_comm: Sending WiFi configuration response, status: 0x00[0m
[0;32mI (30944) uart_comm: ====== Sending Packet Packet Details ======[0m
[0;32mI (31025) uart_comm: Header: 0xAA 0x55[0m
[0;32mI (31078) uart_comm: Command: 0x02[0m
[0;32mI (31127) uart_comm: Data: 00 01 00 00 00 00[0m
[0;32mI (31186) uart_comm: Checksum: 0x02[0m
[0;32mI (31236) uart_comm: ================================[0m
[0;32mI (31305) uart_comm: Raw response: AA 55 02 00 01 00 00 00 00 02 [0m
[0;32mI (31386) uart_comm: Successfully sent 10 bytes[0m
猆[0;32mI (31603) esp_netif_handlers: sta ip: 192.168.50.47, mask: 255.255.255.0, gw: 192.168.50.1[0m
[0;32mI (31604) cc_hal_wifi: Got IP: 192.168.50.47[0m
[0;32mI (31636) gs_wifi: wifi got ip: 192.168.50.47[0m
[0;32mI (31697) gs_mqtt: __event_handler: GS_WIFI_EVENT event: 3[0m
[0;32mI (31771) hal_network: Event dispatched from event loop base=MQTT_EVENTS, event_id=7[0m
[0;32mI (31872) hal_network: Other event id:7[0m
I (31937) wifi:<ba-add>idx:0 (ifx:0, 42:e4:24:d9:ee:64), tid:0, ssn:0, winSize:64
[0;32mI (32346) hal_network: Event dispatched from event loop base=MQTT_EVENTS, event_id=1[0m
[0;32mI (32346) gs_mqtt: __event_handler: GS_MQTT_EVENT event: 0[0m
[0;32mI (32388) gs_mqtt: gs_mqtt_publish topic: /event/property/post data: {"ver":"1.21.0.0","act":"0002","seq_no":"33290826691135830692772803924917"}[0m
[0;32mI (32553) app_main: Version message sent successfully[0m
[0;32mI (32620) gs_mqtt: gs_mqtt_publish topic: /event/property/post data: {"ver":"1.21.0.0","act":"0003","type":"02","data":"-16","seq_no":"04504024310661975104459076344090"}[0m
[0;32mI (32812) app_main: RSSI message sent successfully[0m
[0;32mI (32875) app_main: MQTT birth messages all sent, start UVC camera now...[0m
[0;32mI (32965) USB_STREAM: UVC Streaming Config Succeed, Version: 1.5.0[0m
[0;32mI (33047) USB_STREAM: USB streaming callback register succeed[0m
[0;32mI (33124) USB_STREAM: Pre-alloc ctrl urb succeed, size = 1024[0m
[0;32mI (33202) USB_STREAM: USB stream task start[0m
[0;32mI (33232) USB_STREAM: USB Streaming Start Succeed[0m
[0;32mI (33324) USB_STREAM: Waiting USB Device Connection[0m

[2025-01-06 20:19:45.216]
RX：I (36515) wifi:<ba-add>idx:1 (ifx:0, 42:e4:24:d9:ee:64), tid:6, ssn:2, winSize:64

[2025-01-06 20:19:53.808]
TX：AA55100000000000000F
[2025-01-06 20:19:56.616]
RX：[0;32mI (46231) uart_comm: Received 10 bytes of data[0m
[0;32mI (46232) uart_comm: Raw data: AA 55 10 00 00 00 00 00 00 0F [0m
[0;32mI (46236) uart_comm: Received valid packet, CMD=0x10[0m
[0;32mI (46304) uart_comm: ====== Received Packet Details ======[0m
[0;32mI (46378) uart_comm: Header: 0xAA 0x55[0m
[0;32mI (46431) uart_comm: Command: 0x10[0m
[0;32mI (46480) uart_comm: Data: 00 00 00 00 00 00[0m
[0;32mI (46539) uart_comm: Checksum: 0x0F[0m
[0;32mI (46589) uart_comm: ================================[0m
[0;32mI (46658) app_main: uart_packet_received: CMD=0x10[0m
[0;32mI (46723) app_main: Received CMD_GET_TIME=0x10 => send current time from get_time module...[0m
[0;32mI (46832) app_main:   => read from get_time module: UTC=0, TZ=0[0m
[0;32mI (46911) uart_comm: Sending Time Response (CMD=0x11), utc=0, timezone=0[0m
[0;32mI (47000) uart_comm: ====== SendTimeResp Packet Details ======[0m
[0;32mI (47078) uart_comm: Header: 0xAA 0x55[0m
[0;32mI (47131) uart_comm: Command: 0x11[0m
[0;32mI (47180) uart_comm: Data: 00 00 00 00 00 00[0m
[0;32mI (47239) uart_comm: Checksum: 0x10[0m
[0;32mI (47289) uart_comm: ================================[0m
[0;32mI (47358) uart_comm: ====== Sending Packet Packet Details ======[0m
[0;32mI (47438) uart_comm: Header: 0xAA 0x55[0m
[0;32mI (47491) uart_comm: Command: 0x11[0m
[0;32mI (47540) uart_comm: Data: 00 00 00 00 00 00[0m
[0;32mI (47600) uart_comm: Checksum: 0x10[0m
[0;32mI (47649) uart_comm: ================================[0m
[0;32mI (47719) uart_comm: Raw response: AA 55 11 00 00 00 00 00 00 10 [0m
[0;32mI (47800) uart_comm: Successfully sent 10 bytes[0m
猆










































#include "net_uart_comm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include "gs_device.h"  // 添加设备信息相关头文件
#include "cc_hal_wifi.h"

// 为了获取缓存时间,需要包含 get_time.h
#include "get_time.h"

// 引入必要的头文件以支持 NVS 和系统重启
#include "nvs_flash.h"
#include "esp_system.h"

static const char *TAG = "uart_comm";

// ====== UART 相关配置 ======
#define UART_NUM            UART_NUM_0
#define UART_TX_GPIO        43  // 请根据硬件情况修改
#define UART_RX_GPIO        44  // 请根据硬件情况修改
#define UART_BUFFER_SIZE    1024
#define UART_QUEUE_SIZE     20

// UART事件队列
static QueueHandle_t uart_event_queue = NULL;
// UART事件任务
static TaskHandle_t  uart_task_handle = NULL;

// 数据包回调
static uart_packet_callback_t s_packet_callback = NULL;

// 清除数据互斥锁
static SemaphoreHandle_t clear_data_mutex = NULL;

/**
 * @brief 调试打印原始数据
 */
static void print_raw_data(const char* prefix, const uint8_t* data, size_t len)
{
    char buffer[256];
    int offset = 0;
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s: ", prefix);
    for (size_t i = 0; i < len && offset < (int)sizeof(buffer) - 3; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%02X ", data[i]);
    }
    ESP_LOGI(TAG, "%s", buffer);
}

/**
 * @brief 打印数据包详细信息
 */
static void print_packet_details(const uart_packet_t *packet, const char* prefix)
{
    ESP_LOGI(TAG, "====== %s Packet Details ======", prefix);
    ESP_LOGI(TAG, "Header: 0x%02X 0x%02X", packet->header[0], packet->header[1]);
    ESP_LOGI(TAG, "Command: 0x%02X", packet->command);
    ESP_LOGI(TAG, "Data: %02X %02X %02X %02X %02X %02X",
             packet->data[0], packet->data[1], packet->data[2],
             packet->data[3], packet->data[4], packet->data[5]);
    ESP_LOGI(TAG, "Checksum: 0x%02X", packet->checksum);
    ESP_LOGI(TAG, "==============================");
}

/**
 * @brief 计算校验和：简单求和取低8位
 */
uint8_t uart_comm_calc_checksum(const uint8_t *data, size_t length)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

/**
 * @brief 发送数据包
 */
esp_err_t uart_comm_send_packet(const uart_packet_t *packet)
{
    if (!packet) {
        ESP_LOGE(TAG, "Invalid null packet pointer");
        return ESP_ERR_INVALID_ARG;
    }

    // 调试打印
    print_packet_details(packet, "Sending");

    // 写UART
    int tx_len = sizeof(uart_packet_t);
    int ret = uart_write_bytes(UART_NUM, (const char *)packet, tx_len);
    if (ret < 0) {
        ESP_LOGE(TAG, "uart_write_bytes failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Successfully sent %d bytes", ret);
    return ESP_OK;
}

/**
 * @brief 发送 WiFi 配置响应
 */
esp_err_t uart_comm_send_wifi_response(wifi_config_status_t status)
{
    uart_packet_t packet = {
        .header  = {0xAA, 0x55},
        .command = CMD_WIFI_RESPONSE,
        .data    = {0},
    };

    // data[0] 放 status
    packet.data[0] = (uint8_t)status;
    // 填充其他数据为0x00
    for (int i = 1; i < 6; i++) {
        packet.data[i] = 0x00;
    }

    // 计算校验和
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Sending WiFi configuration response, status=0x%02X", status);
    return uart_comm_send_packet(&packet);
}

/**
 * @brief 发送配网退出应答
 */
esp_err_t uart_comm_send_exit_config_ack(void)
{
    uart_packet_t packet = {
        .header  = {0xAA, 0x55},
        .command = CMD_EXIT_CONFIG_ACK,
        .data    = {0},
    };

    // data区全0
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Sending Exit Config ACK (0x1B)");
    return uart_comm_send_packet(&packet);
}

/**
 * @brief 发送网络状态通知（保留原有实现）
 */
esp_err_t uart_comm_send_network_status(bool connected)
{
    uart_packet_t packet = {
        .header  = {0xAA, 0x55},
        .command = CMD_NETWORK_STATUS,
        .data    = {0},
    };

    // 根据连接状态设置联网状态值
    packet.data[0] = connected ? 0x04 : 0x03; // 示例：已连接云服务器或仅连接路由器

    // 获取UTC时间和时区
    uint32_t utc_time = get_time_get_utc();
    int8_t timezone = get_time_get_timezone();

    // 填充UTC时间（小端）
    packet.data[1] = (uint8_t)(utc_time & 0xFF);
    packet.data[2] = (uint8_t)((utc_time >> 8) & 0xFF);
    packet.data[3] = (uint8_t)((utc_time >> 16) & 0xFF);
    packet.data[4] = (uint8_t)((utc_time >> 24) & 0xFF);

    // 填充时区值
    packet.data[5] = (uint8_t)timezone;

    // 计算校验和
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Sending network status, connected=%d", connected);
    return uart_comm_send_packet(&packet);
}

/**
 * @brief 发送网络时间应答
 */
esp_err_t uart_comm_send_network_time(uint32_t utc_time_sec, int8_t timezone_15min)
{
    uart_packet_t packet = {
        .header  = {0xAA, 0x55},
        .command = CMD_GET_NETWORK_TIME_RSP,
        .data    = {0},
    };

    packet.data[0] = (uint8_t)(utc_time_sec & 0xFF);
    packet.data[1] = (uint8_t)((utc_time_sec >> 8) & 0xFF);
    packet.data[2] = (uint8_t)((utc_time_sec >> 16) & 0xFF);
    packet.data[3] = (uint8_t)((utc_time_sec >> 24) & 0xFF);

    packet.data[4] = (uint8_t)timezone_15min;
    // data[5] = 0

    packet.checksum = uart_comm_calc_checksum((uint8_t *)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Send CMD=0x11, UTC=%u, TZ=%d", (unsigned)utc_time_sec, (int)timezone_15min);
    return uart_comm_send_packet(&packet);
}

/**
 * @brief 发送设备信息应答
 */
esp_err_t uart_comm_send_device_info(const char *device_id, const uint8_t *mac)
{
    if (!device_id || !mac) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    uart_device_info_packet_t packet = {
        .header = {0xAA, 0x55},
        .command = CMD_GET_DEVICE_INFO_RSP,
        .device_id = {0}, // 初始化为0
        .mac = {0},
        .reserved = {0},
        .checksum = 0,
    };

    // 确保设备ID长度不超过12字节，并复制到数据包
    size_t device_id_len = strlen(device_id);
    if (device_id_len > 12) {
        device_id_len = 12;
        ESP_LOGW(TAG, "Device ID length exceeds 12 bytes, truncating");
    }
    memcpy(packet.device_id, device_id, device_id_len);
    if (device_id_len < 12) {
        memset(packet.device_id + device_id_len, 0x00, 12 - device_id_len); // 填充0x00
    }

    // 复制MAC地址(6字节) 
    memcpy(packet.mac, mac, 6);
    // 保留字节清零
    memset(packet.reserved, 0, sizeof(packet.reserved));

    // 计算校验和
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, 
                     sizeof(uart_device_info_packet_t) - 1);

    ESP_LOGI(TAG, "Sending device info response, ID=%s, MAC=%02X:%02X:%02X:%02X:%02X:%02X",
             device_id, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // 打印完整的UART数据（十六进制）
    print_raw_data("UART Data Sent", (uint8_t *)&packet, sizeof(uart_device_info_packet_t));

    // 写入UART
    int tx_len = sizeof(uart_device_info_packet_t);
    int ret = uart_write_bytes(UART_NUM, (const char *)&packet, tx_len);
    if (ret < 0) {
        ESP_LOGE(TAG, "uart_write_bytes failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Successfully sent %d bytes", ret);
    return ESP_OK;
}

/**
 * @brief 发送清除数据响应
 * @param success 执行结果，true 表示成功，false 表示失败
 * @return ESP_OK 成功，其他错误码失败
 */
esp_err_t uart_comm_send_clear_data_response(bool success)
{
    uart_packet_t packet = {
        .header  = {0xAA, 0x55},
        .command = CMD_WIFI_RESPONSE, // 使用已有的WiFi响应命令
        .data    = {0},
    };

    if (success) {
        packet.data[0] = 0x00; // 执行成功
    } else {
        packet.data[0] = 0x02; // 执行失败
    }

    // 填充其他数据为0x00
    for (int i = 1; i < 6; i++) {
        packet.data[i] = 0x00;
    }

    // 计算校验和
    packet.checksum = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);

    ESP_LOGI(TAG, "Sending CMD_WIFI_RESPONSE (0x02) for CMD_CLEAR_DATA, success=%d", success);
    return uart_comm_send_packet(&packet);
}

/**
 * @brief 处理清除数据命令
 * @return ESP_OK 成功，其他错误码失败
 */
static esp_err_t uart_comm_handle_clear_data(void)
{
    ESP_LOGI(TAG, "Handling CMD_CLEAR_DATA: Clearing NVS and resetting state");

    // 获取互斥锁以防止并发清除操作
    if (xSemaphoreTake(clear_data_mutex, (TickType_t)10) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take clear_data_mutex");
        return ESP_FAIL;
    }

    // 1. 完全断开 WiFi 连接
    ESP_LOGI(TAG, "Disconnecting WiFi...");
    esp_err_t ret = cc_hal_wifi_sta_disconnect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disconnect WiFi: %d", ret);
        xSemaphoreGive(clear_data_mutex);
        return ret;
    }

    // 2. 清除 NVS
    ESP_LOGI(TAG, "Erasing NVS...");
    ret = nvs_flash_erase();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase NVS: %d", ret);
        xSemaphoreGive(clear_data_mutex);
        return ret;
    }

    // 3. 重新初始化 NVS
    ESP_LOGI(TAG, "Re-initializing NVS...");
    ret = nvs_flash_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to re-initialize NVS: %d", ret);
        xSemaphoreGive(clear_data_mutex);
        return ret;
    }

    // 4. 重置状态机（假设有相应的函数）
    ESP_LOGI(TAG, "Resetting state machine...");
    cc_event_reset(); // 假设有这个函数

    // 5. 发送成功响应
    ret = uart_comm_send_clear_data_response(true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send clear data success response");
        // 尽管发送失败，但继续重启
    }

    ESP_LOGI(TAG, "Clear data completed, restarting device...");

    // 6. 重启设备以应用更改
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    // 释放互斥锁（虽然设备即将重启）
    xSemaphoreGive(clear_data_mutex);

    return ESP_OK;
}

/**
 * @brief 解析完数据包后调用的回调
 */
static void uart_packet_received_internal(const uart_packet_t *packet)
{
    ESP_LOGI(TAG, "uart_packet_received_internal: CMD=0x%02X", packet->command);

    switch (packet->command) {
    case CMD_WIFI_CONFIG: {
        ESP_LOGI(TAG, "Got CMD_WIFI_CONFIG (0x01)");
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    }

    case CMD_EXIT_CONFIG: {
        ESP_LOGI(TAG, "Got CMD_EXIT_CONFIG (0x1A)");
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    }

    case CMD_NETWORK_STATUS: {
        ESP_LOGI(TAG, "Got CMD_NETWORK_STATUS (0x23)");
        // 根据协议，此指令为WiFi->MCU主动发送的联网状态通知
        // 可根据需要处理或忽略
        // 示例：解析数据内容
        if (packet->data[0] == 0x01) {
            ESP_LOGI(TAG, "Network Status: Not Configured");
        } else if (packet->data[0] == 0x02) {
            ESP_LOGI(TAG, "Network Status: Connecting to Router/Base Station");
        } else if (packet->data[0] == 0x03) {
            ESP_LOGI(TAG, "Network Status: Connected to Router/Base Station");
        } else if (packet->data[0] == 0x04) {
            ESP_LOGI(TAG, "Network Status: Connected to Cloud Server");
        } else {
            ESP_LOGW(TAG, "Unknown Network Status: 0x%02X", packet->data[0]);
        }

        // 解析UTC时间和时区
        uint32_t utc_time_sec = packet->data[1] |
                                 (packet->data[2] << 8) |
                                 (packet->data[3] << 16) |
                                 (packet->data[4] << 24);
        int8_t timezone_15min = (int8_t)packet->data[5];

        ESP_LOGI(TAG, "Received UTC Time: %u, Timezone: %d", utc_time_sec, timezone_15min);
        break;
    }

    case CMD_GET_NETWORK_TIME: {
        ESP_LOGI(TAG, "Got CMD_GET_NETWORK_TIME (0x10)");

        // 这里直接从 get_time 模块获取缓存值并发送应答
        uint32_t utc_sec  = get_time_get_utc();
        int8_t   tz_15min = get_time_get_timezone();
        ESP_LOGI(TAG, "Now cached time: UTC=%u, TimeZone=%d", (unsigned)utc_sec, (int)tz_15min);

        esp_err_t ret = uart_comm_send_network_time(utc_sec, tz_15min);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send network time");
        }
        break;
    }

    case CMD_GET_DEVICE_INFO: {
        ESP_LOGI(TAG, "Got CMD_GET_DEVICE_INFO (0x06)");

        char device_id[13] = {0}; // 12字节ID + 1字节结束符
        uint8_t mac[6] = {0};

        // 获取设备ID
        if (gs_device_get_product_key(device_id) != CC_OK) {
            ESP_LOGE(TAG, "Failed to get device ID");
            return;
        }

        // 获取MAC地址
        if (cl_hal_wifi_sta_get_mac(mac) != CC_OK) { // 修改函数名为 cl_hal_wifi_sta_get_mac
            ESP_LOGE(TAG, "Failed to get WiFi MAC");
            return;
        }

        ESP_LOGI(TAG, "Retrieved Device ID: %s", device_id);
        ESP_LOGI(TAG, "Retrieved MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        // 发送应答
        esp_err_t ret = uart_comm_send_device_info(device_id, mac);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send device info");
        }
        break;
    }

    case CMD_CLEAR_DATA: {
        ESP_LOGI(TAG, "Got CMD_CLEAR_DATA (0x05)");
        // 调用清除数据的处理函数
        esp_err_t ret = uart_comm_handle_clear_data();
        if (ret == ESP_OK) {
            // 已在 handle_clear_data 中发送响应
        } else {
            // 发送失败响应
            uart_comm_send_clear_data_response(false);
        }
        break;
    }

    default:
        ESP_LOGW(TAG, "Unknown cmd=0x%02X", packet->command);
        if (s_packet_callback) {
            s_packet_callback(packet);
        }
        break;
    }
}

/**
 * @brief UART事件任务：读取硬件队列事件并解析为数据包
 */
static void uart_event_task(void *arg)
{
   uart_event_t event;
   uint8_t data_buffer[UART_BUFFER_SIZE];
   int state = 0;
   uart_packet_t packet;
   int len = 0;

   ESP_LOGI(TAG, "UART event task started");

   while (1) {
       if (xQueueReceive(uart_event_queue, &event, portMAX_DELAY)) {
           switch (event.type) {
           case UART_DATA:
               // 读取串口数据
               len = uart_read_bytes(UART_NUM, data_buffer, event.size, portMAX_DELAY);
               if (len > 0) {
                   print_raw_data("Raw data", data_buffer, len);

                   // 使用简易状态机逐字节解析
                   for (int i = 0; i < len; i++) {
                       uint8_t b = data_buffer[i];
                       switch (state) {
                       case 0: // 等待帧头0xAA
                           if (b == 0xAA) {
                               packet.header[0] = b;
                               state = 1;
                           }
                           break;
                       case 1: // 等待帧头0x55
                           if (b == 0x55) {
                               packet.header[1] = b;
                               state = 2;
                           } else {
                               state = 0;
                           }
                           break;
                       case 2: // 接收command
                           packet.command = b;
                           state = 3;
                           break;
                       case 3: // data[0]
                       case 4: // data[1]
                       case 5: // data[2]  
                       case 6: // data[3]
                       case 7: // data[4]
                       case 8: // data[5]
                           packet.data[state - 3] = b;
                           state++;
                           break;
                       case 9: // checksum
                           packet.checksum = b;
                           {
                               uint8_t calc = uart_comm_calc_checksum((uint8_t*)&packet, sizeof(uart_packet_t) - 1);
                               if (calc == packet.checksum) {
                                   ESP_LOGI(TAG, "Received valid packet");
                                   print_packet_details(&packet, "Received");
                                   // 调用内部回调分发
                                   uart_packet_received_internal(&packet);
                               } else {
                                   ESP_LOGE(TAG, "Checksum mismatch: calc=0x%02X, recv=0x%02X", calc, packet.checksum);
                               }
                           }
                           state = 0;
                           break;
                       default:
                           state = 0;
                           break;
                       }
                   }
               }
               break;

           case UART_FIFO_OVF:
               ESP_LOGE(TAG, "HW FIFO overflow");
               uart_flush_input(UART_NUM);
               xQueueReset(uart_event_queue);
               break;

           case UART_BUFFER_FULL:
               ESP_LOGE(TAG, "Ring buffer full");
               uart_flush_input(UART_NUM);
               xQueueReset(uart_event_queue);
               break;

           case UART_BREAK:
               ESP_LOGW(TAG, "UART Break");
               break;

           case UART_PARITY_ERR:
               ESP_LOGW(TAG, "UART Parity Error");
               break;

           case UART_FRAME_ERR:
               ESP_LOGW(TAG, "UART Frame Error");
               break;

           default:
               ESP_LOGW(TAG, "Unhandled UART event type: %d", event.type);
               break;
           }
       }
   }
}

/**
 * @brief 初始化清除数据互斥锁
 */
static void init_clear_data_mutex(void)
{
    clear_data_mutex = xSemaphoreCreateMutex();
    if (clear_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create clear_data_mutex");
    } else {
        ESP_LOGI(TAG, "clear_data_mutex created successfully");
    }
}

/**
 * @brief 初始化 UART
 */
esp_err_t uart_comm_init(void)
{
   ESP_LOGI(TAG, "Initializing UART communication");

   uart_config_t uart_config = {
       .baud_rate  = 9600,
       .data_bits  = UART_DATA_8_BITS,
       .parity     = UART_PARITY_DISABLE,
       .stop_bits  = UART_STOP_BITS_1,
       .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
   };

   esp_err_t ret = uart_param_config(UART_NUM, &uart_config);
   if (ret != ESP_OK) {
       ESP_LOGE(TAG, "uart_param_config failed: %d", ret);
       return ret;
   }

   ret = uart_set_pin(UART_NUM, UART_TX_GPIO, UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
   if (ret != ESP_OK) {
       ESP_LOGE(TAG, "uart_set_pin failed: %d", ret);
       return ret;
   }

   // 安装驱动
   ret = uart_driver_install(UART_NUM, UART_BUFFER_SIZE, UART_BUFFER_SIZE,
                             UART_QUEUE_SIZE, &uart_event_queue, 0);
   if (ret != ESP_OK) {
       ESP_LOGE(TAG, "uart_driver_install failed: %d", ret);
       return ret;
   }

   // 创建事件任务
   BaseType_t xRet = xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, &uart_task_handle);
   if (xRet != pdPASS) {
       ESP_LOGE(TAG, "Failed to create uart_event_task");
       return ESP_FAIL;
   }

   // 初始化清除数据互斥锁
   init_clear_data_mutex();

   ESP_LOGI(TAG, "UART communication initialized successfully");
   return ESP_OK;
}

/**
 * @brief 注册数据包回调
 */
esp_err_t uart_comm_register_callback(uart_packet_callback_t callback)
{
   s_packet_callback = callback;
   return ESP_OK;
}




































