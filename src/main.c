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
 * @brief           系统循环                                      
 *  @details        系统循环函数，在该处调用 
 * 
 * 
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
 * @brief           系统函数入口                                      
 *  @details        初始化鼠标系统并且启动TMOS管理程序
 * 
 * 
 * @return          int类型               程序是否正常退出                                                    
 *  @retval         0                    正常退出
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
    PWR_PeriphClkCfg(DISABLE,CloseClock);
    HSECFG_Current( HSE_RCur_75 );
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
