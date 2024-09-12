/********************************** (C) COPYRIGHT *******************************
 * File Name          : ota.h
 * Author             : WCH
 * Version            : V1.10
 * Date               : 2018/12/14
 * Description        : oad相关配置定义
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
#ifndef __OTA_H
#define __OTA_H

/* ------------------------------------------------------------------------------------------------
 *                                             OTA FLASH
 * ------------------------------------------------------------------------------------------------
 */

/* 整个用户code区分成四块，4K，216K，216K，12K，后三块下面分别叫做imageA（APP），imageB（OTA）和imageIAP */

/* FLASH定义 */
#define FLASH_BLOCK_SIZE       EEPROM_BLOCK_SIZE
#define IMAGE_SIZE             216 * 1024

/* imageA定义 */
#define IMAGE_A_FLAG           0x01
#define IMAGE_A_START_ADD      4 * 1024
#define IMAGE_A_SIZE           IMAGE_SIZE

/* imageB定义 */
#define IMAGE_B_FLAG           0x02
#define IMAGE_B_START_ADD      (IMAGE_A_START_ADD + IMAGE_SIZE)
#define IMAGE_B_SIZE           IMAGE_SIZE

/* imageIAP定义 */
#define IMAGE_IAP_FLAG         0x03
#define IMAGE_IAP_START_ADD    (IMAGE_B_START_ADD + IMAGE_SIZE)
#define IMAGE_IAP_SIZE         12 * 1024

/* IAP定义 */
/* 以下为IAP下载命令定义 */
#define CMD_IAP_PROM           0x80               // IAP编程命令
#define CMD_IAP_ERASE          0x81               // IAP擦除命令
#define CMD_IAP_VERIFY         0x82               // IAP校验命令
#define CMD_IAP_END            0x83               // IAP结束标志
#define CMD_IAP_INFO           0x84               // IAP获取设备信息

/* 数据帧长度定义 */
#define IAP_LEN                247

/* 存放在DataFlash地址，不能占用蓝牙的位置 */
#define OTA_DATAFLASH_ADD      0x00077000 - FLASH_ROM_MAX_SIZE

/* 存放在DataFlash里的OTA信息 */
typedef struct
{
    unsigned char ImageFlag; //记录的当前的image标志
    unsigned char Revd[3];
} OTADataFlashInfo_t;

/* OTA IAP通讯协议定义 */
/* 地址使用4倍偏移 */
typedef union
{
    struct
    {
        unsigned char cmd;          /* 命令码 0x81 */
        unsigned char len;          /* 后续数据长度 */
        unsigned char addr[2];      /* 擦除地址 */
        unsigned char block_num[2]; /* 擦除块数 */

    } erase; /* 擦除命令 */
    struct
    {
        unsigned char cmd;       /* 命令码 0x83 */
        unsigned char len;       /* 后续数据长度 */
        unsigned char status[2]; /* 两字节状态，保留 */
    } end;                       /* 结束命令 */
    struct
    {
        unsigned char cmd;              /* 命令码 0x82 */
        unsigned char len;              /* 后续数据长度 */
        unsigned char addr[2];          /* 校验地址 */
        unsigned char buf[IAP_LEN - 4]; /* 校验数据 */
    } verify;                           /* 校验命令 */
    struct
    {
        unsigned char cmd;              /* 命令码 0x80 */
        unsigned char len;              /* 后续数据长度 */
        unsigned char addr[2];          /* 地址 */
        unsigned char buf[IAP_LEN - 4]; /* 后续数据 */
    } program;                          /* 编程命令 */
    struct
    {
        unsigned char cmd;              /* 命令码 0x84 */
        unsigned char len;              /* 后续数据长度 */
        unsigned char buf[IAP_LEN - 2]; /* 后续数据 */
    } info;                             /* 编程命令 */
    struct
    {
        unsigned char buf[IAP_LEN]; /* 接收数据包*/
    } other;
} OTA_IAP_CMD_t;

/* 记录当前的Image */
extern unsigned char CurrImageFlag;

#endif
/********************************** (C) COPYRIGHT *******************************
 * File Name          : peripheral.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/11
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Simple BLE Peripheral Task Events
#define SBP_START_DEVICE_EVT    0x0001
#define SBP_PERIODIC_EVT        0x0002
#define OTA_FLASH_ERASE_EVT     0x0004  //OTA Flash擦除任务

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void Peripheral_Init(void);

/*
 * Task Event Processor for the BLE Application
 */
extern uint16_t Peripheral_ProcessEvent(uint8_t task_id, uint16_t events);

/*********************************************************************
*********************************************************************/
/********************************** (C) COPYRIGHT *******************************
 * File Name          : OTAprofile.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/11
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef OTAPROFILE_H
#define OTAPROFILE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// OTA Profile通道Index定义
#define OTAPROFILE_CHAR         0

// OTA 服务的UUID定义
#define OTAPROFILE_SERV_UUID    0xFEE0

// OTA 通讯通道UUID定义
#define OTAPROFILE_CHAR_UUID    0xFEE1

// Simple Keys Profile Services bit fields
#define OTAPROFILE_SERVICE      0x00000001

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */

// 读写操作函数回调
typedef void (*OTAProfileRead_t)(unsigned char paramID);
typedef void (*OTAProfileWrite_t)(unsigned char paramID, unsigned char *p_data, unsigned char w_len);

typedef struct
{
    OTAProfileRead_t  pfnOTAProfileRead;
    OTAProfileWrite_t pfnOTAProfileWrite;
} OTAProfileCBs_t;

/*********************************************************************
 * API FUNCTIONS
 */

bStatus_t OTAProfile_AddService(uint32_t services);

bStatus_t OTAProfile_RegisterAppCBs(OTAProfileCBs_t *appCallbacks);

bStatus_t OTAProfile_SendData(unsigned char paramID, unsigned char *p_data, unsigned char send_len);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif