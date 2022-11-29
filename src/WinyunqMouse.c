/*BYTE1 –
|–bit7: 1 表示 Y 坐标的变化量超出－256 ~ 255的范围,0表示没有溢出
|–bit6: 1 表示 X 坐标的变化量超出－256 ~ 255的范围，0表示没有溢出
|–bit5: Y 坐标变化的符号位，1表示负数，即鼠标fdown移动?????WTF
|–bit4: X 坐标变化的符号位，1表示负数，即鼠标fleft移动?????WTF
|–bit3: 恒为1
|–bit2: 1表示mid按下
|–bit1: 1表示右键按下
|–bit0: 1表示left按下
BYTE2 – X坐标变化量，与byte的bit4组成9位符号数,负数表示fleft移，正数表右移。用补码表示变化量
BYTE3 – Y坐标变化量，与byte的bit5组成9位符号数，负数表示fdown移，正数表上移。用补码表示变化量
BYTE4 – 滚轮变化*/
#define __WINYUNQMOUSE_C
typedef unsigned char uint8;
typedef signed short int16;
typedef signed long int32;
typedef unsigned long UINT32;

#include "WinyunqMouse.h"
#include "WinyunqConfigure.h"
extern uint8 HidDev_Report(uint8 id, uint8 type, uint8 len, uint8 *pData); //上报函数

uint32_t LastMoveUPDownTime,LastMoveLeftRightTime;

uint8 Sleep;

uint8 LastClick;

int32 upt, downt, ledtt, rightt;
/**
 * @brief           函数简介                                      
 *  @details        【不能简单描述则必选】如果不能一句话描述函数，请使用Details进行详细描述
 * 
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-11-29
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-11-29
 * @version         1.0.0
 */
void MouseInit()
{
  mousedata.data = 0;
  EEPROM_READ(0, MouseConfigure.data, MouseConfigureSize); //读取保存的状态位
  if (MouseConfigure.data[MouseConfigureSize - 1] != 17)
  {
    MouseConfigure.details.right = 0;
    MouseConfigure.details.trackball = 0;
    MouseConfigure.details.help = 0;
    MouseConfigure.details.LEDOn = 0;
    MouseConfigure.details.speed = 7;
    MouseConfigure.details.report = 17;
    MouseConfigure.details.sleep = 3;
    MouseConfigure.data[MouseConfigureSize - 1] = 17;
    EEPROM_WRITE(0, MouseConfigure.data, MouseConfigureSize);
  }
  GPIOB_ModeCfg(LeftTouch, GPIO_ModeIN_PU); //left按下,默认高电平,按钮按下低电平
  GPIOA_ModeCfg(MoveUPDown, GPIO_ModeIN_Floating);
 {

    R16_PA_INT_MODE |= MoveUPDown;
    R32_PA_CLR |= MoveUPDown;
    R32_PA_OUT |= MoveUPDown;
    R16_PA_INT_IF = MoveUPDown;
    R16_PA_INT_EN |= MoveUPDown;
  }
    //GPIOA_ITModeCfg(MoveGroup, GPIO_ITMode_RiseEdge);
    PFIC_EnableIRQ(GPIO_A_IRQn);
    //PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Edge_LongDelay);
    GPIOB_ModeCfg(MoveLeftRight, GPIO_ModeIN_Floating);
     {

        R16_PB_INT_MODE |= MoveLeftRight;
        R32_PB_CLR |= MoveLeftRight;
        R32_PB_OUT |= MoveLeftRight;
        R16_PB_INT_IF = MoveLeftRight;
        R16_PB_INT_EN |= MoveLeftRight;
      }
        //GPIOA_ITModeCfg(MoveGroup, GPIO_ITMode_RiseEdge);
        PFIC_EnableIRQ(GPIO_B_IRQn);
  upt = downt = ledtt = rightt = 0;
  
  GPIOA_ModeCfg(trackballpower, GPIO_ModeOut_PP_20mA);
  if (MouseConfigure.details.help) //help仅使用右键，不需要向霍尔供电
    GPIOA_ResetBits(trackballpower);
  else //向霍尔供电
    GPIOA_SetBits(trackballpower);
  InitLED();
}
/**
 * @brief           函数简介                                      
 *  @details        【不能简单描述则必选】如果不能一句话描述函数，请使用Details进行详细描述
 * 
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-11-29
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-11-29
 * @version         1.0.0
 */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void GPIOA_IRQHandler(void)
{
if(GPIOA_ReadITFlagBit(MoveUP))upt++;
else downt++;
    GPIOA_ClearITFlagBit(  -1);
}
/**
 * @brief           函数简介                                      
 *  @details        【不能简单描述则必选】如果不能一句话描述函数，请使用Details进行详细描述
 * 
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-11-29
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-11-29
 * @version         1.0.0
 */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void GPIOB_IRQHandler(void)
{
    if(GPIOB_ReadITFlagBit(MoveLeft))ledtt++;
    else rightt++;
  GPIOB_ClearITFlagBit(-1); //后期有可能将轨迹球的GPIO拆为两部分
}
/**
 * @brief           函数简介                                      
 *  @details        【不能简单描述则必选】如果不能一句话描述函数，请使用Details进行详细描述
 * 
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-11-29
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-11-29
 * @version         1.0.0
 */
void GoSleep()
{//return 0;
  if (MouseConfigure.details.sleep)
  { //为0表示静止睡眠
  
  GPIOA_ResetBits(LEDIndicator); //关掉LED
    PFIC_DisableIRQ(GPIO_A_IRQn);
    
    GPIOA_ResetBits(trackballpower);
    
    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Edge_ShortDelay);
    //SetSysClock( CLK_SOURCE_HSE_6_4MHz );
    while(!GPIOB_ReadPortPin(GPIO_Pin_22));
    GPIOB_ITModeCfg(GPIO_Pin_22, GPIO_ITMode_FallEdge); // 下降沿唤醒 影响进入休眠，导致无法唤醒
    PFIC_EnableIRQ(GPIO_B_IRQn);
    LowPower_Shutdown(0); //这里是直接掉电了,转动轨迹球重启唤醒
  }
}
uint32 UsingSleepTime = 0;
/**
 * @brief           函数简介                                      
 *  @details        【不能简单描述则必选】如果不能一句话描述函数，请使用Details进行详细描述
 * 
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2022-11-29
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-11-29
 * @version         1.0.0
 */
void MoveByLocation(){
  
  int x = 0, y = 0;
  x = rightt - ledtt;
  ledtt = rightt = 0;
  y = downt - upt;
  upt = downt = 0;
  if (!MouseConfigure.details.help)
  {
    if (MouseConfigure.details.trackball)
    {
      mousedata.details.x = y;
      mousedata.details.y = -x;
    } //trackball下，以右手为例，方向逆时针旋转90°
    else
    {
      mousedata.details.x = x;
      mousedata.details.y = -y; //非trackball下Y轴反向
    }
    if (mousedata.details.x < 0)
      mousedata.details.situation.fleft = 1;
    if (mousedata.details.y < 0)
      mousedata.details.situation.fdown = 1; // Y方向
    mousedata.details.x *= MouseConfigure.details.speed;
    mousedata.details.y *= MouseConfigure.details.speed;
  }
  mousedata.details.z = 0;              //计算需要上报的数据
}
void MouseEvent()
{
  if (MouseConfigure.details.right)
  {
    if (!GPIOB_ReadPortPin(LeftTouch))
    {
      mousedata.details.situation.right = 1;
    }
  }
  else
  {
    if (!GPIOB_ReadPortPin(LeftTouch))
    {
      mousedata.details.situation.left = 1;
    }
  }
  if ((mousedata.buffer[0] == LastClick) //确认按键是否有更新
      && (mousedata.buffer[1] == 0)      //确认是否有x轴位移
      && (mousedata.buffer[2] == 0))     //确认是否有y轴位移
  {
    UsingSleepTime++;                                                    //无操作时间计时
    if (UsingSleepTime * MouseConfigure.details.report > WinyunqMouseSleepTime) //长时间处于无操作状态，进入睡眠模式
    {
      GoSleep();
    }
  }
  else
  {
    HidDev_Report(0, 1, 4, mousedata.buffer); //上报数据
    UsingSleepTime = 0;
  }
  LastClick = mousedata.buffer[0]; //记录上次上报的按键
  mousedata.data = 0;              //清空,X,Y,Z会在上次上报数据前必定被更新

  return;
}
