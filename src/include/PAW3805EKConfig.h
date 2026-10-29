/**
 * @file PAW3805EKConfig.h
 * @brief PAW3805EK-CJV1 的板级连线配置，可由离线配置页面生成。
 * @warning 默认未配置，不访问传感器 GPIO。端口/引脚是 WCH GPIO 编号，
 * 不是芯片封装脚号；必须核对实际开发板引出、供电和共地。
 */
#ifndef WINYUNQ_PAW3805EK_CONFIG_H
#define WINYUNQ_PAW3805EK_CONFIG_H

#ifndef PAW3805EK_CONFIGURED
#define PAW3805EK_CONFIGURED 0
#endif
#ifndef PAW3805EK_NCS_PORT
#define PAW3805EK_NCS_PORT 0
#endif
#ifndef PAW3805EK_NCS_PIN
#define PAW3805EK_NCS_PIN 0
#endif
#ifndef PAW3805EK_SCLK_PORT
#define PAW3805EK_SCLK_PORT 0
#endif
#ifndef PAW3805EK_SCLK_PIN
#define PAW3805EK_SCLK_PIN 0
#endif
#ifndef PAW3805EK_SDIO_PORT
#define PAW3805EK_SDIO_PORT 0
#endif
#ifndef PAW3805EK_SDIO_PIN
#define PAW3805EK_SDIO_PIN 0
#endif
#ifndef PAW3805EK_SWAP_XY
#define PAW3805EK_SWAP_XY 0
#endif
#ifndef PAW3805EK_INVERT_X
#define PAW3805EK_INVERT_X 0
#endif
#ifndef PAW3805EK_INVERT_Y
#define PAW3805EK_INVERT_Y 0
#endif

#endif
