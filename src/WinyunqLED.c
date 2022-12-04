/**
 * @file            文件名:                WinyunqLED.c
 * @brief           函数简介                                      
 *  @details        需要#Define LEDIndicator指定LED所在GPIO_pin位置
 * 
 * @author          Winyunq             创建
 * @version         版本:                 1.0.0
 * @date            2022-12-03          创建
 * 
 * Copyright (c) 2022  Winyunq科技公司
 */
#include "WinyunqMouse.h"
#include "WinyunqConfigure.h"
#define LEDLock 0x80
#define LightChange 0x40
extern tmosTaskID PowerID;
tmosTaskID ConnectPower;
/// 没有连接设备，等待配对的标志位
uint8 noConnect=0;
uint32 SearchSleepTime=0;
/**
 * @brief           LED恢复默认状态                                      
 *  @details        LED恢复到由鼠标配置的亮灭状态
 * 
 * @param           参数名称:【task_id】      数据类型:             参数说明
 * @param           参数名称:【events】       数据类型:             参数说明
 * 
 * @return          uint16类型            返回值介绍                                                         
 *  @retval         【返回值分段则必选】值或范围      说明
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-12-03
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-12-03
 * @version         1.0.0
 */
uint16 DefaultLED( uint8 task_id, uint16 events ){
  noConnect&=~LEDLock;
  if(MouseConfigure.details.LEDOn)GPIOA_SetBits(LEDIndicator);//高电压点亮
  else GPIOA_ResetBits(LEDIndicator);//低电压灭灯
  return 0;
}
/**
 * @brief           LED 等待连接指示                                      
 *  @details        LED将进入指示灯形式，进行里亮-灭-亮的闪烁。该函数不能被主动调用，要调用FindConnectPower函数来启动
 * 
 * @param           参数名称:【task_id】      数据类型:             参数说明
 * @param           参数名称:【events】       数据类型:             参数说明
 * 
 * @return          uint16类型            返回值介绍                                                         
 *  @retval         【返回值分段则必选】值或范围      说明
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-12-03
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-12-03
 * @version         1.0.0
 */
uint16 WaitConnect( uint8 task_id, uint16 events ){
  if(noConnect){
    if(!(noConnect&LEDLock))GPIOA_InverseBits(LEDIndicator);
    SearchSleepTime++;if(SearchSleepTime*517>WinyunqMouseSleepTime)GoSleep();//搜索时间过长
  tmos_start_task( ConnectPower,3,517);
  }//LED闪烁
  else {DefaultLED(0,0);
  SearchSleepTime=0;
  }
  return 0;
}
/**
 * @brief           LED指示-正在查找设备                                      
 *  @details        将令LED进入闪烁状态，直到noConnect归0
 * 
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-12-03
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-12-03
 * @version         1.0.0
 */
void FindConnectPower(){//等待连接
 noConnect|=LightChange;
  SearchSleepTime=0;
  tmos_start_task( ConnectPower,3,517);
}
/**
 * @brief           锁定LED                                      
 *  @details        立刻锁定当前LED状态，当LED解锁时，将恢复为默认状态
 * 
 * @param           参数名称:【time】         数据类型:uint32       锁定LED的时间。在到达该时间后将还原LED为默认LED状态
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-12-03
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-12-03
 * @version         1.0.0
 */
void LockLED(uint32 time){
  noConnect|=LEDLock;
  tmos_start_task( TMOS_ProcessEventRegister( DefaultLED ),3,time);
}
extern uint16 PowerTask( uint8 task_id, uint16 events );
/**
 * @brief           LED初始化系统                                     
 *  @details        初始化LED相关进程，并且立刻点亮LED一段时间
 * 
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-12-03
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-12-03
 * @version         1.0.0
 */
void InitLED(){
  PowerID=TMOS_ProcessEventRegister( PowerTask );
  LockLED(7520);//信号引脚默认为高,之后允许修改
  GPIOA_ModeCfg(LEDIndicator,GPIO_ModeOut_PP_5mA);//初始化信号引脚，低电压点亮
    GPIOA_SetBits(LEDIndicator);
  ConnectPower=TMOS_ProcessEventRegister( WaitConnect );
}
