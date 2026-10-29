/**
 * @file main.c
 * @brief 芯片、BLE 与鼠标模块的初始化入口。
 * @details BLE 模式下依次初始化 WCH 协议栈、HAL、HID 服务和鼠标，
 * 随后由 Main_Circulation() 持续调度 TMOS。传感器读取应在任务事件中
 * 完成，不另建阻塞式主循环，也不改变现有 BLE HID 服务。
 * @see @ref WinyunqArchitecture
 */
/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : �������Ӧ��������������ϵͳ��ʼ��
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
/* ͷ�ļ����� */
#include "CONFIG.h"
#include "HAL.h"
#include "hiddev.h"
#include "hidmouse.h"
#include "WinyunqMouse.h"
//#include "USBMouse.h"
/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/**
 * @brief 持续运行现有系统调度器，不返回。
 * @details 定义 UsingBLE 时只调度 TMOS；MouseEvent() 由 HID 任务触发。
 * 未定义 UsingBLE 时沿用原有直接调用 MouseEvent() 的路径。
 * @warning TMOS 采用协作式任务处理，事件回调中的长时间等待会阻碍 BLE。
 **/
__attribute__((section(".highcode")))
void Main_Circulation()
{
    while(1)
    {
        /// 蓝牙，USB可以组合使用，一旦使用蓝牙，则采用TMOS系统调动，否则仅USB模式下将直接循环访问MouseEvent()
        #ifdef UsingBLE
        TMOS_SystemProcess();
        #else
        MouseEvent();
        #endif
    }
}

/**
 * @brief 初始化系统并进入永久运行的调度循环。
 * @details 保留原有时钟、BLE、可选 USB、鼠标及可选 OTA 初始化顺序。
 * 
 * 
 * @return 正常运行时不返回。
 * 
 */
int main(void)
{

    SetSysClock(CLK_SOURCE_PLL_60MHz);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PD);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PD);
#endif
#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
/// 支持蓝牙状态下，需要进行的蓝牙初始化
#ifdef UsingBLE

#ifdef UsingOTA
    ReadImageFlag();
#endif
#if(CHIP_ID ==ID_CH573)
    CH57X_BLEInit();
#endif
#if(CHIP_ID ==ID_CH592)
    CH59x_BLEInit();
#endif
    HAL_Init();
    GAPRole_PeripheralInit();
    HidDev_Init();
    HidEmu_Init();
    //PWR_PeriphClkCfg(DISABLE,CloseClock);
    //HSECFG_Current( HSE_RCur_75 );
    #if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
    #endif
#endif
/// 支持USB状态下，需要进行的USB初始化
#ifdef UsingUSB
    USBModeInit();
#endif
    MouseInit();
#ifdef UsingOTA
    OTAPeripheralInit();
#endif
    Main_Circulation();
}

/******************************** endfile @ main ******************************/
