/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2019/11/05
 * Description        : 外设从机应用主函数及任务系统初始化
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "HAL.h"/*
#include "GATTprofile.h"
#include "OTAprofile.h"*/

#include "OTA.h"
/* 记录当前的Image */
uint8 CurrImageFlag = 0xff;
/* 用于APP判断文件有效性 */
const uint32_t Address = 0xFFFFFFFF;

__attribute__((aligned(4))) uint32_t Image_Flag __attribute__((section(".ImageFlag"))) = (uint32_t)&Address;

/* 注意：关于程序升级后flash的操作必须先执行，不开启任何中断，防止操作中断和失败 */
/*********************************************************************
 * @fn      ReadImageFlag
 *
 * @brief   读取当前的程序的Image标志，DataFlash如果为空，就默认是ImageA
 *
 * @return  none
 */
void ReadImageFlag(void)
{
    OTADataFlashInfo_t p_image_flash;

    EEPROM_READ(OTA_DATAFLASH_ADD, &p_image_flash, 4);
    CurrImageFlag = p_image_flash.ImageFlag;

    /* 程序第一次执行，或者没有更新过，以后更新后在擦除DataFlash */
    if((CurrImageFlag != IMAGE_A_FLAG) && (CurrImageFlag != IMAGE_B_FLAG))
    {
        CurrImageFlag = IMAGE_A_FLAG;
    }
}
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// How often to perform periodic event
#define SBP_PERIODIC_EVT_PERIOD              1000

// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL         160

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely

#define DEFAULT_DISCOVERABLE_MODE            GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL    6

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL    12

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY        0

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT         1000

// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST        TRUE

// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL        6

// Company Identifier: WCH
#define WCH_COMPANY_ID                       0x07D7

#define INVALID_CONNHANDLE                   0xFFFF

// Length of bd addr as a string
#define B_ADDR_STR_LEN                       15

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t Peripheral_TaskID = 0xff; // Task ID for internal task/event processing

// OTA IAP VARIABLES
/* OTA通讯的帧 */
OTA_IAP_CMD_t iap_rec_data;

/* OTA解析结果 */
uint32_t OpParaDataLen = 0;
uint32_t OpAdd = 0;

/* flash的数据临时存储 */
__attribute__((aligned(8))) uint8_t block_buf[16];

/* Image跳转函数地址定义 */
typedef int (*pImageTaskFn)(void);
pImageTaskFn user_image_tasks;

/* Flash 擦除过程 */
uint32_t EraseAdd = 0;      //擦除地址
uint32_t EraseBlockNum = 0; //需要擦除的块数
uint32_t EraseBlockCnt = 0; //擦除的块计数

/* FLASH 校验过程 */
uint8_t VerifyStatus = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void        OTA_IAPReadDataComplete(uint8 index);
void        OTA_IAPWriteData(uint8 index, uint8 *p_data, uint8 w_len);
void        Rec_OTA_IAP_DataDeal(void);
void        OTA_IAP_SendCMDDealSta(uint8_t deal_status);

/*********************************************************************
 * PROFILE CALLBACKS
 */


// Simple GATT Profile Callbacks
static OTAProfileCBs_t Peripheral_OTA_IAPProfileCBs = {
    OTA_IAPReadDataComplete, // Charactersitic value change callback
    OTA_IAPWriteData
};

// Callback when the connection parameteres are updated.
void PeripheralParamUpdate(uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout);

gapRolesParamUpdateCB_t PeripheralParamUpdate_t = NULL;

/**
 * 
*/
void OTAPeripheralInit()
{
    Peripheral_TaskID = TMOS_ProcessEventRegister(Peripheral_ProcessEvent);
    OTAProfile_AddService(GATT_ALL_SERVICES);
    OTAProfile_RegisterAppCBs(&Peripheral_OTA_IAPProfileCBs);

    // Setup a delayed profile startup
    tmos_set_event(Peripheral_TaskID, SBP_START_DEVICE_EVT);
}

void PeripheralParamUpdate(uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout)
{
    PRINT("update %d %d %d \n", connInterval, connSlaveLatency, connTimeout);

    //		GAPRole_SendUpdateParam( DEFAULT_DESIRED_MIN_CONN_INTERVAL, DEFAULT_DESIRED_MAX_CONN_INTERVAL,
    //                                 DEFAULT_DESIRED_SLAVE_LATENCY, DEFAULT_DESIRED_CONN_TIMEOUT, GAPROLE_NO_ACTION );
}

/*********************************************************************
 * @fn      Peripheral_ProcessEvent
 *
 * @brief   Peripheral Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16_t Peripheral_ProcessEvent(uint8_t task_id, uint16_t events)
{
    //OTA_FLASH_ERASE_EVT
    if(events & OTA_FLASH_ERASE_EVT)
    {
        uint8_t status;

        PRINT("ERASE:%08x num:%d\r\n", (int)(EraseAdd + EraseBlockCnt * FLASH_BLOCK_SIZE), (int)EraseBlockCnt);
        status = FLASH_ROM_ERASE(EraseAdd + EraseBlockCnt * FLASH_BLOCK_SIZE, FLASH_BLOCK_SIZE);

        /* 擦除失败 */
        if(status != SUCCESS)
        {
            OTA_IAP_SendCMDDealSta(status);
            return (events ^ OTA_FLASH_ERASE_EVT);
        }

        EraseBlockCnt++;

        /* 擦除结束 */
        if(EraseBlockCnt >= EraseBlockNum)
        {
            PRINT("ERASE Complete\r\n");
            OTA_IAP_SendCMDDealSta(status);
            return (events ^ OTA_FLASH_ERASE_EVT);
        }

        return (events);
    }

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      Peripheral_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void Peripheral_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        default:
            break;
    }
}

/*********************************************************************
 * @fn      peripheralStateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void peripheralStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    switch(newState)
    {
        case GAPROLE_STARTED:
            PRINT("Initialized..\n");
            break;

        case GAPROLE_ADVERTISING:
            PRINT("Advertising..\n");
            break;

        case GAPROLE_CONNECTED:
        {
            gapEstLinkReqEvent_t *event = (gapEstLinkReqEvent_t *)pEvent;
            uint16_t              conn_interval = 0;

            conn_interval = event->connInterval;
            PRINT("Connected.. \n");

            if(conn_interval > DEFAULT_DESIRED_MAX_CONN_INTERVAL)
            {
                PRINT("Send Update\r\n");
                GAPRole_PeripheralConnParamUpdateReq(event->connectionHandle,
                                                     DEFAULT_DESIRED_MIN_CONN_INTERVAL,
                                                     DEFAULT_DESIRED_MAX_CONN_INTERVAL,
                                                     DEFAULT_DESIRED_SLAVE_LATENCY,
                                                     DEFAULT_DESIRED_CONN_TIMEOUT,
                                                     Peripheral_TaskID);
            }
            break;
        }
        case GAPROLE_CONNECTED_ADV:
            PRINT("Connected Advertising..\n");
            break;
        case GAPROLE_WAITING:
        {
            uint8_t initial_advertising_enable = TRUE;

            // Set the GAP Role Parameters
            GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
            PRINT("Disconnected..\n");
        }
        break;

        case GAPROLE_ERROR:
            PRINT("Error..\n");
            break;

        default:
            break;
    }
}
/*********************************************************************
 * @fn      OTA_IAP_SendData
 *
 * @brief   OTA IAP发送数据，使用时限制20字节以内
 *
 * @param   p_send_data - 发送数据的指针
 * @param   send_len    - 发送数据的长度
 *
 * @return  none
 */
void OTA_IAP_SendData(uint8_t *p_send_data, uint8_t send_len)
{
    OTAProfile_SendData(OTAPROFILE_CHAR, p_send_data, send_len);
}

/*********************************************************************
 * @fn      OTA_IAP_SendCMDDealSta
 *
 * @brief   OTA IAP执行的状态返回
 *
 * @param   deal_status - 返回的状态
 *
 * @return  none
 */
void OTA_IAP_SendCMDDealSta(uint8_t deal_status)
{
    uint8_t send_buf[2];

    send_buf[0] = deal_status;
    send_buf[1] = 0;
    OTA_IAP_SendData(send_buf, 2);
}

/*********************************************************************
 * @fn      OTA_IAP_CMDErrDeal
 *
 * @brief   OTA IAP异常命令码处理
 *
 * @return  none
 */
void OTA_IAP_CMDErrDeal(void)
{
    OTA_IAP_SendCMDDealSta(0xfe);
}

/*********************************************************************
 * @fn      SwitchImageFlag
 *
 * @brief   切换dataflash里的ImageFlag
 *
 * @param   new_flag    - 切换的ImageFlag
 *
 * @return  none
 */
void SwitchImageFlag(uint8_t new_flag)
{
    uint16_t i;
    uint32_t ver_flag;

    /* 读取第一块 */
    EEPROM_READ(OTA_DATAFLASH_ADD, (uint32_t *)&block_buf[0], 4);

    /* 擦除第一块 */
    EEPROM_ERASE(OTA_DATAFLASH_ADD, EEPROM_PAGE_SIZE);

    /* 更新Image信息 */
    block_buf[0] = new_flag;

    /* 编程DataFlash */
    EEPROM_WRITE(OTA_DATAFLASH_ADD, (uint32_t *)&block_buf[0], 4);
}

/*********************************************************************
 * @fn      DisableAllIRQ
 *
 * @brief   关闭所有的中断
 *
 * @return  none
 */
void DisableAllIRQ(void)
{
    SYS_DisableAllIrq(NULL);
}

/*********************************************************************
 * @fn      Rec_OTA_IAP_DataDeal
 *
 * @brief   接收到OTA数据包处理
 *
 * @return  none
 */
void Rec_OTA_IAP_DataDeal(void)
{
    switch(iap_rec_data.other.buf[0])
    {
        /* 编程 */
        case CMD_IAP_PROM:
        {
            uint32_t i;
            uint8_t  status;

            OpParaDataLen = iap_rec_data.program.len;
            OpAdd = (uint32_t)(iap_rec_data.program.addr[0]);
            OpAdd |= ((uint32_t)(iap_rec_data.program.addr[1]) << 8);
            OpAdd = OpAdd * 16;

            OpAdd += IMAGE_A_SIZE;

            PRINT("IAP_PROM: %08x len:%d \r\n", (int)OpAdd, (int)OpParaDataLen);

            /* 当前是ImageA，直接编程 */
            status = FLASH_ROM_WRITE(OpAdd, iap_rec_data.program.buf, (uint16_t)OpParaDataLen);
            if(status)             PRINT("IAP_PROM err \r\n");
            OTA_IAP_SendCMDDealSta(status);
            break;
        }
        /* 擦除 -- 蓝牙擦除由主机控制 */
        case CMD_IAP_ERASE:
        {
            OpAdd = (uint32_t)(iap_rec_data.erase.addr[0]);
            OpAdd |= ((uint32_t)(iap_rec_data.erase.addr[1]) << 8);
            OpAdd = OpAdd * 16;

            OpAdd += IMAGE_A_SIZE;

            EraseBlockNum = (uint32_t)(iap_rec_data.erase.block_num[0]);
            EraseBlockNum |= ((uint32_t)(iap_rec_data.erase.block_num[1]) << 8);
            EraseAdd = OpAdd;
            EraseBlockCnt = 0;

            /* 检验就放在擦除里清0 */
            VerifyStatus = 0;

            PRINT("IAP_ERASE start:%08x num:%d\r\n", (int)OpAdd, (int)EraseBlockNum);

            if(EraseAdd < IMAGE_B_START_ADD || (EraseAdd + (EraseBlockNum - 1) * FLASH_BLOCK_SIZE) > IMAGE_IAP_START_ADD)
            {
                OTA_IAP_SendCMDDealSta(0xFF);
            }
            else
            {
                /* 启动擦除 */
                tmos_set_event(Peripheral_TaskID, OTA_FLASH_ERASE_EVT);
            }
            break;
        }
        /* 校验 */
        case CMD_IAP_VERIFY:
        {
            uint32_t i;
            uint8_t  status = 0;

            OpParaDataLen = iap_rec_data.verify.len;

            OpAdd = (uint32_t)(iap_rec_data.verify.addr[0]);
            OpAdd |= ((uint32_t)(iap_rec_data.verify.addr[1]) << 8);
            OpAdd = OpAdd * 16;

            OpAdd += IMAGE_A_SIZE;
            PRINT("IAP_VERIFY: %08x len:%d \r\n", (int)OpAdd, (int)OpParaDataLen);

            /* 当前是ImageA，直接读取ImageB校验 */
            status = FLASH_ROM_VERIFY(OpAdd, iap_rec_data.verify.buf, OpParaDataLen);
            if(status)
            {
                PRINT("IAP_VERIFY err \r\n");
            }
            VerifyStatus |= status;
            OTA_IAP_SendCMDDealSta(VerifyStatus);
            break;
        }
        /* 编程结束 */
        case CMD_IAP_END:
        {
            PRINT("IAP_END \r\n");

            /* 当前的是ImageA */
            /* 关闭当前所有使用中断，或者方便一点直接全部关闭 */
            DisableAllIRQ();

            /* 修改DataFlash，切换至ImageIAP */
            SwitchImageFlag(IMAGE_IAP_FLAG);

            /* 等待打印完成 ，复位*/
            mDelaymS(10);
            SYS_ResetExecute();

            break;
        }
        case CMD_IAP_INFO:
        {
            uint8_t send_buf[20];

            PRINT("IAP_INFO \r\n");

            /* IMAGE FLAG */
            send_buf[0] = IMAGE_B_FLAG;

            /* IMAGE_SIZE */
            send_buf[1] = (uint8_t)(IMAGE_SIZE & 0xff);
            send_buf[2] = (uint8_t)((IMAGE_SIZE >> 8) & 0xff);
            send_buf[3] = (uint8_t)((IMAGE_SIZE >> 16) & 0xff);
            send_buf[4] = (uint8_t)((IMAGE_SIZE >> 24) & 0xff);

            /* BLOCK SIZE */
            send_buf[5] = (uint8_t)(FLASH_BLOCK_SIZE & 0xff);
            send_buf[6] = (uint8_t)((FLASH_BLOCK_SIZE >> 8) & 0xff);

            send_buf[7] = CHIP_ID&0xFF;
            send_buf[8] = (CHIP_ID>>8)&0xFF;
            /* 有需要再增加 */

            /* 发送信息 */
            OTA_IAP_SendData(send_buf, 20);

            break;
        }

        default:
        {
            OTA_IAP_CMDErrDeal();
            break;
        }
    }
}

/*********************************************************************
 * @fn      OTA_IAPReadDataComplete
 *
 * @brief   OTA 数据读取完成处理
 *
 * @param   index   - OTA 通道序号
 *
 * @return  none
 */
void OTA_IAPReadDataComplete(uint8 index)
{
    PRINT("OTA Send Comp \r\n");
}

/**
 * @brief           OTA 通道数据接收写入                                      
 *  @details        OTA 蓝牙GATT通信接收到了数据，将其写入连续地址
 * 
 * @param           参数名称:【index】        数据类型:uint8             OTA 通道序号
 * @param           参数名称:【p_data】       数据类型:uint8             蓝牙接收到的数据地址
 * @param           参数名称:【w_len】        数据类型:uint8             蓝牙接收到的数据长度
 * 
 **/
void OTA_IAPWriteData(uint8 index, uint8 *p_data, uint8 w_len)
{
    uint8  rec_len;
    uint8 *rec_data;

    rec_len = w_len;
    rec_data = p_data;
    /// 将OTA 蓝牙GATT接收到的数据复制到指定地址保存
    tmos_memcpy((uint8 *)&iap_rec_data, rec_data, rec_len);
    /// 根据当前状态决定如何处理这一组数据
    Rec_OTA_IAP_DataDeal();
}

/*********************************************************************
*********************************************************************/
