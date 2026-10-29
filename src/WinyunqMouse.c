/**
 * @file WinyunqMouse.c
 * @brief 按键、传感器位移、运行配置与 HID 上报之间的连接层。
 * @details MouseInit() 加载 EEPROM 配置并安排所选传感器初始化；
 * MouseEvent() 由 HID 的 TMOS 上报事件调用，组装并发送四字节报告。
 * 更换 PAW3805EK 时应替换传感器初始化和位移采集入口，不改动
 * BLEHIDReport()、Profile 目录或主循环架构。
 * @note 实际 BLE 描述符位于 src/Profile/hidmouseservice.c：
 * 第 0 字节为按键，随后为 X、Y 和滚轮的 8 位相对位移（-127..127）。
 * MouseData 的旧符号/溢出位不能使 BLE 报告变成 9 位坐标。
 * @note 此分支默认选择 PAW3805EK；引脚未配置时不访问传感器 GPIO。
 * UsingPAW3805EK = 0 保留原霍尔路径。
 * @see @ref WinyunqArchitecture
 */
#define __WINYUNQMOUSE_C

#include "WinyunqMouse.h"
#include "WinyunqConfigure.h"
#if UsingPAW3805EK
#include "PAW3805EK.h"
#include "PAW3805EKConfig.h"
#else
#include "WinyunqMouseHallDriver.h"
#endif
#include "WinyunqLED.h"
#include "WinyunqBatteryService.h"
#ifdef UsingUSB
#include "USBMouse.h"
#endif
    
/// 该代码理应使用CH573枚举的Edge_ShortDelay，但由于CH579平台下没有该定义，只有short与long，故此重新定义
#define Edge_ShortDelay 1

extern uint8 BLEHIDReport(uint8 id, uint8 type, uint8 len, uint8 *pData); // 上报函数
uint8 Sleep;
/// 上一次成功上报的按键状态。
uint8 LastClick;
#if UsingPAW3805EK
/// 光学位移余量，避免 16 位传感器数据被 8 位 HID 报告直接截断。
static signed long OpticalPendingX, OpticalPendingY;
static signed short OpticalReportX, OpticalReportY;
/** @brief 最多保留八个 HID 最大帧/轴，避免断链或拥塞后长时间重放。 */
#define OPTICAL_MAX_PENDING (127L * 8)
/** @brief 队列饱和的轴次数，可在调试器观察；饱和表示主动丢弃超量位移。 */
static unsigned long OpticalClippedAxes;
static unsigned char OpticalDrainEligible, OpticalDiscardNextRead;
#else
/// 上次上，下，左，右的霍尔触发时间
uint32_t LastMoveUPDownTime, NowMoveUPDownTime, LastMoveLeftRightTime, NowMoveLeftRightTime;

/// 上的霍尔触发次数
#ifdef UsingUPDowmHallEdge
/// @brief 触发上下移动中断的GPIO来源
extern uint32_t MoveUPDownInterruptFrom;
/// @brief 上下移动触发中断累计次数
extern uint32_t UPDownTime;
/// @brief 当前上下移动中断状态，0：当前是下降沿；1：当前是上升沿
extern uint8_t NowUPDownGroupInterruptForward;
/// @brief 触发左右移动中断的GPIO来源
extern uint32_t MoveLeftRightInterruptFrom;
/// @brief 左右移动触发中断累计次数
extern uint32_t LeftRightTime;
/// @brief 当前左右移动中断状态，0：当前是下降沿；1：当前是上升沿
extern uint8_t NowLeftRightGroupInterruptForward;
#else
#endif
int32 UPTime,
    /// 下的霍尔触发次数
    DownTime,
    /// 左的霍尔触发次数
    LeftTime,
    /// 右的霍尔触发次数
    RightTime;

#endif

/**
 * @brief           鼠标系统初始化
 * @details 从 EEPROM 读取八字节运行配置；末字节标志无效时写入默认值。
 * 随后初始化原按键、所选传感器和 LED。光学传感器初始化由 TMOS 分段完成。
 * @note 这是传感器硬件初始化的替换点，EEPROM 格式和 LED 行为保持独立。
 * 
 */
void MouseInit()
{
  /// 初始化数据
  MouseData.data = 0;
#if UsingPAW3805EK
  OpticalPendingX = OpticalPendingY = 0;
  OpticalReportX = OpticalReportY = 0;
  OpticalClippedAxes = 0;
  OpticalDrainEligible = OpticalDiscardNextRead = 0;
#else
  UPTime = DownTime = LeftTime = RightTime = 0;
#endif
  /// 加载EEPROM中存储的配置数据
  EEPROM_READ(0, MouseConfigure.data, MouseConfigureSize); // 读取保存的状态位
  /// 检测鼠标是否初始化过，若MouseConfigureSize - 1处数据为17，则已经初始化过，否则需要执行下列代码初始化
  if (MouseConfigure.data[MouseConfigureSize - 1] != 17)
  {
    MouseConfigure.details.right = 0;
    MouseConfigure.details.trackball = 0;
    MouseConfigure.details.help = 0;
    MouseConfigure.details.LEDOn = 0;
    MouseConfigure.details.speed = 3;
    MouseConfigure.details.report = 17;
    MouseConfigure.details.sleep = 3;
    MouseConfigure.data[MouseConfigureSize - 1] = 17;
    /// 将初始化数据保存至EEPROM
    EEPROM_WRITE(0, MouseConfigure.data, MouseConfigureSize);
  }
  /// 原按键引脚不变，按下为低电平。
  GPIOB_ModeCfg(LeftTouch, GPIO_ModeIN_PU);
#if UsingPAW3805EK
  /// 只安排传感器探测/复位事件，等待时间由 TMOS 处理。
  PAW3805EK_Init();
#else
  /// 原霍尔 GPIO、中断和供电路径。
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
  /// 初始化霍尔供电
  GPIOA_ModeCfg(trackballpower, GPIO_ModeOut_PP_20mA);
  /// 判断鼠标配置是否使用了轨迹球，如果是则打开霍尔供电，如果否则关闭霍尔供电
  if (MouseConfigure.details.help) // help仅使用右键，不需要向霍尔供电
    GPIOA_ResetBits(trackballpower);
  else // 向霍尔供电
    GPIOA_SetBits(trackballpower);
#endif
  /// 鼠标外设初始化完成，初始化LED灯，LED灯将长亮三秒
  InitLED();
}
#if !UsingPAW3805EK && !defined(UsingUPDowmHallEdge)
/**
 * @brief           上下移动中断
 *  @details        上下移动的霍尔元件连接在GPIOA组上，当发生GPIOA组的中断时，若中断源来自MoveUP，则表示鼠标向上移动了，否则中断源来自MoveDown并且鼠标向下移动了
 * 
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

#endif
/**
 * @brief           休眠
 * @details 仅在 UsingSleep == 1 时启用原有关机路径：上报电量，关闭
 * LED/霍尔供电，等待按键释放，配置 GPIO 唤醒后调用 LowPower_Shutdown()。
 * @warning 现有按键释放等待含阻塞循环；不得把它当作传感器初始化时序模板。
 * PAW3805EK 的供电和低功耗序列必须另按确切型号资料确认。
 * 
 */
void GoSleep()
{ 
  #if defined(UsingSleep) && (UsingSleep == 1)
  /// 在休眠之前先上报一次电量
  battNotifyLevel();
  /// 判断鼠标是否允许休眠，为0表示禁止睡眠
  if (MouseConfigure.details.sleep)
  {
    /// 关掉LED灯
    CloseLED();
#if UsingPAW3805EK
    /// 使用已证实的芯片掉电命令，不用旧霍尔 GPIO 给裸芯片断电。
    PAW3805EK_PowerDown();
#else
    PFIC_DisableIRQ(GPIO_A_IRQn);
    GPIOA_ResetBits(trackballpower);
#endif
    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Edge_ShortDelay);
    // SetSysClock( CLK_SOURCE_HSE_6_4MHz );
    /// 在进入休眠之前先确保按键处于抬起状态，确保可以正常唤醒
    while (1){
      /// 按键已经抬起
      if(GPIOB_ReadPortPin(GPIO_Pin_22)){
        /// 防止抖动
        DelayMs(50);
        if(GPIOB_ReadPortPin(GPIO_Pin_22)){
          break;
        }
      }
    }
    // 下降沿唤醒 影响进入休眠，导致无法唤醒
    PFIC_EnableIRQ(GPIO_B_IRQn);
    GPIOB_ITModeCfg(GPIO_Pin_22, GPIO_ITMode_FallEdge); 
    /// 系统掉电了,转动轨迹球重启唤醒
    LowPower_Shutdown(0); 
  }
  #endif
}
/**
 * @brief           获取按键状态
 *  @details        获取按键状态，并将其保存在MouseData中
 * 
 */
uint32 UsingSleepTime = 0;
/**
 * @brief           获取按键状态
 *  @details        获取按键状态，并且保存到MouseData中
 * 
 */
void GetTouchSituation()
{
  if (!GPIOB_ReadPortPin(LeftTouch))
  {
    
    //OpenLED();
    if (MouseConfigure.details.right)
    {
      MouseData.details.situation.right = 1;
    }
    else
    {
      MouseData.details.situation.left = 1;
    }
  }
    //CloseLED();
}
#if !UsingPAW3805EK
/**
 * @brief           检测速度，上报移动量
 *  @details        与MoveByLocation()类似，但是修改移动逻辑为，当移动速度足够快（两次霍尔时间间距足够小）时，放大速度。而当移动速度足够慢时，则以1像素最低精度移动
 * 
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
  x/=(MoveDelayBufferSize);
  MouseData.details.x = x;
  MouseData.details.y = y;
  MoveDataPoint++;
  if (MoveDataPoint == MoveDelayBufferSize){
    MoveDataPoint = 0;
  }
}
#ifndef UsingUPDowmHallEdge
/**
 * @brief           检测位移，上报位移
 *  @details        检测在此期间各方向霍尔元件触发次数，从而确定在各方向的移动距离。不使用任何算法。
 * 
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
      MouseData.details.x = -x;
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
#endif
#endif /* legacy Hall movement algorithms */

#if UsingPAW3805EK
/**
 * @brief 将待发位移限制为八帧/轴，避免无线拥塞后的长时间重放。
 * @param pending 尚未成功上报的位移。
 * @param delta 本次位移，已提升为 long，反转 -32768 时不会溢出。
 * @return 饱和后的待发送位移。
 */
static signed long OpticalAccumulate(signed long pending, signed long delta)
{
  const signed long limit = OPTICAL_MAX_PENDING;
  if (delta > 0 && pending > limit - delta)
  {
    if (OpticalClippedAxes < 0xffffffffUL) ++OpticalClippedAxes;
    return limit;
  }
  if (delta < 0 && pending < -limit - delta)
  {
    if (OpticalClippedAxes < 0xffffffffUL) ++OpticalClippedAxes;
    return -limit;
  }
  return pending + delta;
}

/** @brief 按现有 HID 描述符限制单帧位移，剩余量留给后续上报。 */
static signed short OpticalReportPart(signed long pending)
{
  if (pending > 127) return 127;
  if (pending < -127) return -127;
  return (signed short)pending;
}

/**
 * @brief 最多读取一组光学位移，并准备当前四字节 HID 报告。
 * @details SPI 事务有界，不循环等待 MOTION 清除。物理安装方向来自
 * PAW3805EKConfig.h；原 trackball 选项仍表示逆时针旋转 90 度。
 * help 模式持续读取并丢弃位移，避免退出该模式时产生积压跳动。
 * 旧 speed 字段不冒充 CPI；本实现不写未经核实的 CPI 设置。
 */
static void MoveByOptical(void)
{
  signed short sampleX = 0, sampleY = 0;
  signed long x, y, temporary;
  unsigned char valid = PAW3805EK_ReadMotion(&sampleX, &sampleY);

  if (OpticalDiscardNextRead)
  {
    /* 连接/通知切换后的第一次读取只清掉设备侧可能积存的旧位移。 */
    valid = 0;
    OpticalDiscardNextRead = 0;
  }

  if (MouseConfigure.details.help)
  {
    OpticalPendingX = OpticalPendingY = 0;
  }
  else if (valid)
  {
    x = sampleX;
    y = sampleY;
#if PAW3805EK_SWAP_XY
    temporary = x; x = y; y = temporary;
#endif
#if PAW3805EK_INVERT_X
    x = -x;
#endif
#if PAW3805EK_INVERT_Y
    y = -y;
#endif
    if (MouseConfigure.details.trackball)
    {
      temporary = x; x = y; y = -temporary;
    }
    OpticalPendingX = OpticalAccumulate(OpticalPendingX, x);
    OpticalPendingY = OpticalAccumulate(OpticalPendingY, y);
  }
  OpticalReportX = OpticalReportPart(OpticalPendingX);
  OpticalReportY = OpticalReportPart(OpticalPendingY);
  MouseData.details.x = (uint8)(signed char)OpticalReportX;
  MouseData.details.y = (uint8)(signed char)OpticalReportY;
  MouseData.details.z = 0;
}

/** @brief 仅在传输层接受本帧后扣除位移，发送失败时保留余量。 */
static void OpticalReportAccepted(void)
{
  OpticalPendingX -= OpticalReportX;
  OpticalPendingY -= OpticalReportY;
  OpticalDrainEligible = (OpticalPendingX != 0 || OpticalPendingY != 0);
}
#endif

/**
 * @brief 清除连接切换前的光学待发数据，不改变 BLE 服务或 EEPROM。
 * @details 下次传感器读取只用于丢弃设备侧的旧位移，按键仍正常处理。
 */
void MouseResetMotion(void)
{
#if UsingPAW3805EK
  OpticalPendingX = OpticalPendingY = 0;
  OpticalReportX = OpticalReportY = 0;
  OpticalDrainEligible = 0;
  OpticalDiscardNextRead = 1;
#endif
  LastClick = 0xff; /* 比较哨兵，不进入报告；重新启用后必须同步一次按键松/按状态。 */
}

/** @brief 仅当上一帧已被传输层接受且还有余量时请求更快的 TMOS 排空。 */
unsigned char MouseMotionCanDrain(void)
{
#if UsingPAW3805EK
  return OpticalDrainEligible;
#else
  return 0;
#endif
}

/**
 * @brief           鼠标上报事件
 * @details 先读取按键，再调用所选传感器路径。光学路径读取一组 16 位
 * 位移，按现有 8 位报告范围分帧；BLE 发送失败时保留位移与按键重试。
 * 旧霍尔路径的算法和上报逻辑保留，可通过 UsingPAW3805EK 选择。
 * @warning BLE 模式下本函数运行于 TMOS 回调，传感器初始化等待应拆成事件，
 * 不能在这里等待硬件就绪或另启永久轮询。
 * 
 */
void MouseEvent()
{
#if UsingPAW3805EK
  unsigned char reportAccepted = 1;
  unsigned char reportStatus;
  OpticalDrainEligible = 0;
#endif
  GetTouchSituation();/*
  if (MouseConfigure.details.MoveTypeBySpeed)
  {
    MoveBySpeed();
  }
  else
  {
    MoveByLocation();
  }*/
  #if UsingPAW3805EK
  MoveByOptical();
  #elif defined(UsingUSB)
   MoveByHallSpeedHalf();
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
      /// 只有使用蓝牙模式，即使用电池电源，才需要考虑休眠。事实上，该处逻辑应该还需要判断是否已连接USB。
       #ifdef UsingBLE
       GoSleep();
       #endif
    }
  }
  else
  {
#if UsingPAW3805EK
    reportAccepted = 0;
#endif
    #ifdef UsingBLE
    #if UsingPAW3805EK
    reportStatus = BLEHIDReport(0, 1, 4, MouseData.buffer);
    reportAccepted = (reportStatus == 0);
    if (reportStatus == bleNotReady)
      MouseResetMotion(); /* 断连/未就绪不是短暂队列拥塞，禁止重连后回放。 */
    #else
    BLEHIDReport(0, 1, 4, MouseData.buffer); // 上报数据
    #endif
    #endif
    #ifdef UsingUSB
    USBHIDReport(MouseData.buffer,4);
    #if UsingPAW3805EK
    reportAccepted = 1; /* 原 USB API 不返回状态，沿用其交付约定。 */
    #endif
    #endif
#if UsingPAW3805EK
    if (reportAccepted)
      OpticalReportAccepted();
#endif
    UsingSleepTime = 0;
  }
#if UsingPAW3805EK
  if (reportAccepted)
    LastClick = MouseData.buffer[0];
#else
  LastClick = MouseData.buffer[0]; // 记录上次上报的按键
#endif
  MouseData.data = 0;              // 清空,X,Y,Z会在上次上报数据前必定被更新

  return;
}
