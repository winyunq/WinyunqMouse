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

#define __WINYUNQLED_C
#include "WinyunqLED.h"
#include "WinyunqMouse.h"
#include "WinyunqConfigure.h"
#define LEDLock 0x80
#define LightChange 0x40
#define LEDEvent 0b0000000000000010
uint8 LEDLevel=25;
tmosTaskID ConnectPower;
/**
 * @brief           LED恢复默认状态                                      
 *  @details        LED恢复到由鼠标配置的亮灭状态
 * 
 * @param           参数名称:【task_id】      数据类型:uint8             TMOS的task_id
 * @param           参数名称:【events】       数据类型:uint16            TMOS的events
 * 
 * @return          uint16类型            返回值介绍                                                         
 *  @retval         【返回值分段则必选】值或范围      说明
 * 
 */
uint16 DefaultLED( uint8 task_id, uint16 events ){
  noConnect&=~LEDLock;
  if(MouseConfigure.details.LEDOn)OpenLED();//高电压点亮
  else CloseLED();//低电压灭灯
  return 0;
}
/**
 * @brief           LED 等待连接指示                                      
 *  @details        LED将进入指示灯形式，进行里亮-灭-亮的闪烁。该函数不能被主动调用，要调用FindConnectPower函数来启动
 * 
 * @param           参数名称:【task_id】      数据类型:uint8             TMOS的task_id
 * @param           参数名称:【events】       数据类型:uint16            TMOS的events
 * 
 * @return          uint16类型            返回值介绍                                                         
 *  @retval         【返回值分段则必选】值或范围      说明
 * 
 */
uint16 WaitConnect( uint8 task_id, uint16 events ){
  if(noConnect){
    if(!(noConnect&LEDLock)){
      #ifdef LEDPWMChannel
      #else
        GPIOA_InverseBits(LEDIndicator);
      #endif
    }
    SearchSleepTime++;if(SearchSleepTime*517>WinyunqMouseSleepTime)GoSleep();//搜索时间过长
  tmos_start_task( ConnectPower,LEDEvent,WaitConnectIntervalLED);
  }//LED闪烁
  else {DefaultLED(0,0);
  SearchSleepTime=0;
  }
  return 0;
}
/**
 * @brief           LED指示-正在查找设备                                      
 *  @details        将令LED进入闪烁状态，直到noConnect归0
**/ 
void FindConnectPower(){//等待连接
 noConnect|=LightChange;
  SearchSleepTime=0;
  tmos_start_task( ConnectPower,LEDEvent,Second(0.25));
}
/**
 * @brief           锁定LED                                      
 *  @details        立刻锁定当前LED状态，当LED解锁时，将恢复为默认状态
 * 
 * @param           参数名称:【time】         数据类型:uint32       锁定LED的时间。在到达该时间后将还原LED为默认LED状态
 * 
**/ 
void LockLED(uint32 time){
  noConnect|=LEDLock;
  tmos_start_task( TMOS_ProcessEventRegister( DefaultLED ),LEDEvent,time);
}
extern uint16 PowerTask( uint8 task_id, uint16 events );
/**
 * @brief           LED初始化系统                                     
 *  @details        初始化LED相关进程，并且立刻点亮LED一段时间
 * 
**/
void InitLED(){
  GPIOA_ModeCfg(LEDIndicator,GPIO_ModeOut_PP_5mA);//初始化信号引脚，低电压点亮
 #ifdef LEDPWMChannel
  /// 采用PWM控制灯的亮度，在此不分频
  PWMX_CLKCfg(8);
  /// PWM31分频，因为在项目中LED灯多为固定频率
  PWMX_CycleCfg(PWMX_Cycle_256);
  
#endif
  OpenLED();
  LockLED(Second(3));//信号引脚默认为高,之后允许修改
  ConnectPower=TMOS_ProcessEventRegister( WaitConnect );
}
