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
//#include "USBMouse.h"
extern uint8 HidDev_Report(uint8 id, uint8 type, uint8 len, uint8 *pData); // 上报函数
/// 上次上，下，左，右的霍尔触发时间
uint32_t LastMoveUPDownTime, NowMoveUPDownTime, LastMoveLeftRightTime, NowMoveLeftRightTime;

uint8 Sleep;
/// 上一次上报数据包时上报的按键状态
uint8 LastClick;
/// 上的霍尔触发次数
int32 UPTime,
    /// 下的霍尔触发次数
    DownTime,
    /// 左的霍尔触发次数
    LeftTime,
    /// 右的霍尔触发次数
    RightTime;
/**
 * @brief           鼠标系统初始化
 *  @details        包括配置文件的读取，鼠标传感器的初始化，LED指示灯初始化
 *
 *
 * */
/*
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
  MouseData.data = 0;
  EEPROM_READ(0, MouseConfigure.data, MouseConfigureSize); // 读取保存的状态位
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
  GPIOB_ModeCfg(LeftTouch, GPIO_ModeIN_PU); // left按下,默认高电平,按钮按下低电平
  GPIOA_ModeCfg(MoveUPDown, GPIO_ModeIN_Floating);
  {

    R16_PA_INT_MODE |= MoveUPDown;
    R32_PA_CLR |= MoveUPDown;
    R32_PA_OUT |= MoveUPDown;
    R16_PA_INT_IF = MoveUPDown;
    R16_PA_INT_EN |= MoveUPDown;
  }
  // GPIOA_ITModeCfg(MoveGroup, GPIO_ITMode_RiseEdge);
  PFIC_EnableIRQ(GPIO_A_IRQn);
  // PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Edge_LongDelay);
  GPIOB_ModeCfg(MoveLeftRight, GPIO_ModeIN_Floating);
  {

    R16_PB_INT_MODE |= MoveLeftRight;
    R32_PB_CLR |= MoveLeftRight;
    R32_PB_OUT |= MoveLeftRight;
    R16_PB_INT_IF = MoveLeftRight;
    R16_PB_INT_EN |= MoveLeftRight;
  }
  // GPIOA_ITModeCfg(MoveGroup, GPIO_ITMode_RiseEdge);
  PFIC_EnableIRQ(GPIO_B_IRQn);
  UPTime = DownTime = LeftTime = RightTime = 0;

  GPIOA_ModeCfg(trackballpower, GPIO_ModeOut_PP_20mA);
  if (MouseConfigure.details.help) // help仅使用右键，不需要向霍尔供电
    GPIOA_ResetBits(trackballpower);
  else // 向霍尔供电
    GPIOA_SetBits(trackballpower);
  InitLED();
}
/**
 * @brief           上下移动中断
 *  @details        上下移动的霍尔元件连接在GPIOA组上，当发生GPIOA组的中断时，若中断源来自MoveUP，则表示鼠标向上移动了，否则中断源来自MoveDown并且鼠标向下移动了
 *
 *
 * */
/*
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
__attribute__((section(".highcode"))) void
GPIOA_IRQHandler(void)
{
  if (GPIOA_ReadITFlagBit(MoveUP))
    UPTime++;
  else
    DownTime++;
  LastMoveUPDownTime = NowMoveUPDownTime;
  NowMoveUPDownTime = TMOS_GetSystemClock();
  GPIOA_ClearITFlagBit(-1);
}
/**
 * @brief           左右移动中断
 *  @details        左右移动的霍尔元件连接在GPIOB组上，当发生GPIOB组的中断时，若中断源来自MoveLeft，则表示鼠标向左移动了，否则中断源来自MoveRight并且鼠标向右移动了
 *
 *
 * */
/*
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
__attribute__((section(".highcode"))) void
GPIOB_IRQHandler(void)
{
  if (GPIOB_ReadITFlagBit(MoveLeft))
  {
    LeftTime++;
  }
  else
    RightTime++;
  LastMoveLeftRightTime = NowMoveLeftRightTime;
  NowMoveLeftRightTime = TMOS_GetSystemClock();
  GPIOB_ClearITFlagBit(-1); // 后期有可能将轨迹球的GPIO拆为两部分
}
/**
 * @brief           休眠
 *  @details        进入休眠状态，该过程将关闭LED，关闭霍尔电源，只保留轨迹球按键的终端唤醒。当按下轨迹球时，鼠标重启
 *
 *
 * */
/*
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
{ // return 0;
  if (MouseConfigure.details.sleep)
  { // 为0表示静止睡眠

    GPIOA_ResetBits(LEDIndicator); // 关掉LED
    PFIC_DisableIRQ(GPIO_A_IRQn);

    GPIOA_ResetBits(trackballpower);

    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Edge_ShortDelay);
    // SetSysClock( CLK_SOURCE_HSE_6_4MHz );
    while (!GPIOB_ReadPortPin(GPIO_Pin_22))
      ;
    GPIOB_ITModeCfg(GPIO_Pin_22, GPIO_ITMode_FallEdge); // 下降沿唤醒 影响进入休眠，导致无法唤醒
    PFIC_EnableIRQ(GPIO_B_IRQn);
    LowPower_Shutdown(0); // 这里是直接掉电了,转动轨迹球重启唤醒
  }
}
/**
 * @brief           获取按键状态
 *  @details        获取按键状态，并将其保存在MouseData中
 *
 *
 * */
/*
 * 创建者:             Winyunq
 * 创建日期:            2022-11-29
 *
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-11-29
 * @version         1.0.0
 */
uint32 UsingSleepTime = 0;
/**
 * @brief           获取按键状态
 *  @details        获取按键状态，并且保存到MouseData中
 *
 *
 * */
/*
 * 创建者:             Winyunq
 * 创建日期:            2022-11-29
 *
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-11-29
 * @version         1.0.0
 */
void GetTouchSituation()
{
  if (!GPIOB_ReadPortPin(LeftTouch))
  {
    if (MouseConfigure.details.right)
    {
      MouseData.details.situation.right = 1;
    }
    else
    {
      MouseData.details.situation.left = 1;
    }
  }
}
/**
 * @brief           检测速度，上报移动量
 *  @details        与MoveByLocation()类似，但是修改移动逻辑为，当移动速度足够快（两次霍尔时间间距足够小）时，放大速度。而当移动速度足够慢时，则以1像素最低精度移动
 *
 *
 * */
/*
 * 创建者:             Winyunq
 * 创建日期:            2022-11-29
 *
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-11-29
 * @version         1.0.0
 */
void MoveBySpeed()
{
  int32 SpeedTime, MoveSpeed;

  int x = 0, y = 0;
  x = RightTime - LeftTime;
  LeftTime = RightTime = 0;
  y = DownTime - UPTime;
  UPTime = DownTime = 0;
  SpeedTime = NowMoveLeftRightTime - LastMoveLeftRightTime;
  if (SpeedTime < WinyunqMouseMoveSpeedListTime)
    x *= MouseConfigure.details.speed;
  SpeedTime = NowMoveUPDownTime - LastMoveUPDownTime;
  if (SpeedTime < WinyunqMouseMoveSpeedListTime)
    y *= MouseConfigure.details.speed;
  if (!MouseConfigure.details.help)
  {
    if (MouseConfigure.details.trackball)
    {
      MouseData.details.x = y;
      MouseData.details.y = -x;
    } // trackball下，以右手为例，方向逆时针旋转90°
    else
    {
      MouseData.details.x = x;
      MouseData.details.y = -y; // 非trackball下Y轴反向
    }
    if (MouseData.details.x < 0)
      MouseData.details.situation.fleft = 1;
    if (MouseData.details.y < 0)
      MouseData.details.situation.fdown = 1; // Y方向
  }
  MouseData.details.z = 0; // 计算需要上报的数据
}
#define MoveDelayBufferSize 8
uint8 MoveDataBufferUPDOwn[MoveDelayBufferSize] = {0};
uint8 MoveDataBufferLeftRight[MoveDelayBufferSize] = {0};
uint8 MoveDataPoint = 0;
/**
 * @brief           轨迹球上下移动中断                                      
 *  @details        当发生上下移动中断时，调用该函数处理中断信息
 * 
 * @param           参数名称:【Forward】      数据类型:uint8        代表本次移动方向，值为1或-1。使用其它值不合理但不会导致BUG。
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2023-02-12
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2023-02-12
 * @version         1.0.0
 */
__attribute__((always_inline)) inline void MoveBySpeedUPAndDown(uint8 Forward)
{
  for (int i = 0; i < MoveDelayBufferSize; i++)
  {
    MoveDataBufferUPDOwn[i] += Forward;
  }
  //MoveDataBufferUPDOwn[MoveDataPoint] += Forward;
}
__attribute__((always_inline)) inline void MoveBySpeedLeftAndRight(uint8 Forward)
{
  for (int i = 0; i < MoveDelayBufferSize; i++)
  {
    MoveDataBufferLeftRight[i] += Forward;
  }
  //MoveDataBufferLeftRight[MoveDataPoint] += Forward;
}
/**
 * @brief           移动距离时域展开移动                                      
 *  @details        将一个移动数据平摊为多个移动数据，随后检测时域上有无过近的上报数据。若存在，则由此叠加数据，增加移动速度。
 * 
 * 
 * *//*
 * 创建者:             Winyunq
 * 创建日期:            2023-02-12
 * 
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2023-02-12
 * @version         1.0.0
 */
void MoveBySpeedDelay()
{
  int x=0, y=0;
  x = RightTime - LeftTime;
  LeftTime = RightTime = 0;
  y = DownTime - UPTime;
  UPTime = DownTime = 0;
  MoveBySpeedUPAndDown(x);
  MoveBySpeedLeftAndRight(y);
  x=y=0;
  /// @brief 关Y方向中断
 /// PFIC_DisableIRQ(GPIO_A_IRQn);
  /// @brief 对时域移动求和
  for (int i = 0; i < MoveDelayBufferSize; i++)
  {
    y += MoveDataBufferUPDOwn[i];
  }
  /// @brief 当前时刻值过期
  MoveDataBufferUPDOwn[MoveDataPoint]=0;
  /// @brief Y轴数据访问完毕，开中断（注意这种用法有可能导致数据丢失，建议想办法屏蔽中断）
 /// PFIC_EnableIRQ(GPIO_A_IRQn);
  /// @brief 当轨迹球移动的非常缓慢时，由于其插入的值为MoveDelayBufferSize个1，因此，当且仅当刚刚触发轨迹球，全部为1时，移动1，其他时候舍弃
  y/=(MoveDelayBufferSize);
  /// @brief 关X方向中断
 /// PFIC_DisableIRQ(GPIO_B_IRQn);
  for (int i = 0; i < MoveDelayBufferSize; i++)
  {
    x += MoveDataBufferLeftRight[i];
  }
  /// @brief 此过程将为当前时刻插入的值进行加权。即对于队列第一个值，其权为2，其它值权为1。
  MoveDataBufferLeftRight[MoveDataPoint]=0;
 /// PFIC_EnableIRQ(GPIO_B_IRQn);
  y/=(MoveDelayBufferSize);
  MouseData.details.x = x;
  MouseData.details.y = y;
  MoveDataPoint++;
  if (MoveDataPoint == MoveDelayBufferSize){
    MoveDataPoint = 0;
  }
}
/**
 * @brief           检测位移，上报位移
 *  @details        检测在此期间各方向霍尔元件触发次数，从而确定在各方向的移动距离
 *
 *
 * */
/*
 * 创建者:             Winyunq
 * 创建日期:            2022-11-29
 *
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-11-29
 * @version         1.0.0
 */
void MoveByLocation()
{

  int x = 0, y = 0;
  x = RightTime - LeftTime;
  LeftTime = RightTime = 0;
  y = DownTime - UPTime;
  UPTime = DownTime = 0;
  if (!MouseConfigure.details.help)
  {
    if (MouseConfigure.details.trackball)
    {
      MouseData.details.x = y;
      MouseData.details.y = -x;
    } // trackball下，以右手为例，方向逆时针旋转90°
    else
    {
      MouseData.details.x = x;
      MouseData.details.y = -y; // 非trackball下Y轴反向
    }
    if (MouseData.details.x < 0)
      MouseData.details.situation.fleft = 1;
    if (MouseData.details.y < 0)
      MouseData.details.situation.fdown = 1; // Y方向
    MouseData.details.x *= MouseConfigure.details.speed;
    MouseData.details.y *= MouseConfigure.details.speed;
  }
  MouseData.details.z = 0; // 计算需要上报的数据
}
/**
 * @brief           鼠标上报事件
 *  @details        鼠标上报事件，立刻获取按键状态，移动数据，并判断是否形成了有效数据包。若数据包无效则无视且休眠计数。若数据包有效则提交数据报且清空休眠时间
 *
 *
 * */
/*
 * 创建者:             Winyunq
 * 创建日期:            2022-11-29
 *
 *      《初始化》
 * 修订内容:            创建函数
 * @author          Winyunq进行完善
 * @date            2022-11-29
 * @version         1.0.0
 */
void MouseEvent()
{
  GetTouchSituation();/*
  if (MouseConfigure.details.MoveTypeBySpeed)
  {
    MoveBySpeed();
  }
  else
  {
    MoveByLocation();
  }*/
  #ifdef UsingUSB
  MoveByLocation();
  #else
  MoveBySpeedDelay();
  #endif
  if ((MouseData.buffer[0] == LastClick) // 确认按键是否有更新
      && (MouseData.buffer[1] == 0)      // 确认是否有x轴位移
      && (MouseData.buffer[2] == 0))     // 确认是否有y轴位移
  {
    UsingSleepTime++;                                                           // 无操作时间计时
    if (UsingSleepTime * MouseConfigure.details.report > WinyunqMouseSleepTime) // 长时间处于无操作状态，进入睡眠模式
    {
      GoSleep();
    }
  }
  else
  {
    #ifdef UsingBLE
    HidDev_Report(0, 1, 4, MouseData.buffer); // 上报数据
    #endif
    #ifdef UsingUSB
    DevHIDReport(MouseData.buffer,4);
    #endif
    UsingSleepTime = 0;
  }
  LastClick = MouseData.buffer[0]; // 记录上次上报的按键
  MouseData.data = 0;              // 清空,X,Y,Z会在上次上报数据前必定被更新

  return;
}
