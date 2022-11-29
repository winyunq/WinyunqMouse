#include "WinyunqMouse.h"
#include "WinyunqConfigure.h"
#define LEDLock 0x80
#define LightChange 0x40
extern tmosTaskID PowerID;
tmosTaskID ConnectPower;
uint8 noConnect=0;
uint32 SearchSleepTime=0;
uint16 DefaultLED( uint8 task_id, uint16 events ){
  noConnect&=~LEDLock;
  if(MouseConfigure.details.LEDOn)GPIOA_SetBits(LEDIndicator);//高电压点亮
  else GPIOA_ResetBits(LEDIndicator);//低电压灭灯
  return 0;
}

uint16 WaitConnect( uint8 task_id, uint16 events ){//GoSleep();
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

void FindConnectPower(){//等待连接
 noConnect|=LightChange;
  SearchSleepTime=0;
  tmos_start_task( ConnectPower,3,517);
}
void LockLED(uint32 time){
  noConnect|=LEDLock;
  tmos_start_task( TMOS_ProcessEventRegister( DefaultLED ),3,time);
}
extern uint16 PowerTask( uint8 task_id, uint16 events );
void InitLED(){
  PowerID=TMOS_ProcessEventRegister( PowerTask );
  LockLED(7520);//信号引脚默认为高,之后允许修改
  GPIOA_ModeCfg(LEDIndicator,GPIO_ModeOut_PP_5mA);//初始化信号引脚，低电压点亮
    GPIOA_SetBits(LEDIndicator);
  ConnectPower=TMOS_ProcessEventRegister( WaitConnect );
}
