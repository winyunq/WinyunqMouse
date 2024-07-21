/**
 * @brief           上下移动中断
 *  @details        上下移动的霍尔元件连接在GPIOA组上，当发生GPIOA组的中断时，若中断源来自MoveUP，则表示鼠标向上移动了，否则中断源来自MoveDown并且鼠标向下移动了
 * 
 */
#define __WinyunqMouseHallDriver_C
#ifdef UsingUPDowmHallEdge
#include "WinyunqMouseHallDriver.h"
#include "WinyunqMouse.h"
#include "WinyunqConfigure.h"
/// @brief 触发上下移动中断的GPIO来源
uint32_t MoveUPDownInterruptFrom=MoveUP;
/// @brief 上下移动触发中断累计次数
uint32_t UPDownTime;
/// @brief 当前上下移动中断状态，0：当前是下降沿；1：当前是上升沿
uint8_t NowUPDownGroupInterruptForward;
/// @brief 触发左右移动中断的GPIO来源
uint32_t MoveLeftRightInterruptFrom=MoveLeft;
/// @brief 左右移动触发中断累计次数
uint32_t LeftRightTime;
/// @brief 当前左右移动中断状态，0：当前是下降沿；1：当前是上升沿
uint8_t NowLeftRightGroupInterruptForward;
///
uint32_t MoveUPDownHappenTime=0,NowLeaveMoveUPDownTime=0,MoveUPDownSpeedTime;
uint32_t MoveLeftRightHappenTime=0,NowLeaveMoveLeftRightTime=0,MoveLeftRightSpeedTime;
/**
 * @brief           修改上下移动中断方式                                      
 *  @details        修改上下移动中断方式。调用该函数时，若为上升沿中断，则改为下降沿，若为下降沿中断，则改为上升沿。该函数一般在中断发生时调用，即用于实现上升下降沿双边检测
 * 
 */
__attribute__((always_inline)) inline void ChangeUPDownInterruptEdge()
{
  if(NowUPDownGroupInterruptForward){
      /// 当前为上升沿状态，准备切换到下降沿状态
      NowUPDownGroupInterruptForward=0;
      /// 将中断方向修改为下降沿
      R32_PA_CLR |= MoveUPDown;
    }
    else{
      /// 当前为下降沿状态，准备切换到上升沿状态
      NowUPDownGroupInterruptForward=1;
      /// 将中断修改为上升沿
      R32_PA_OUT |= MoveUPDown;
    }
}
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode"))) 
void GPIOA_IRQHandler(void)
{
  /// 判断是同向方向移动产生的中断还是反向移动产生的中断
  if(GPIOA_ReadITFlagBit(MoveUPDownInterruptFrom)){
    /// 同向移动，计数正增，并且切换检测沿
    UPDownTime++;
    ChangeUPDownInterruptEdge();
  }
  else{
    /// 该中断来自于反方向，不进行中断预测方向修正，除非在MouseEvent()中处理xy数据时方向确实发生改变，即交给MouseEvent()切换方向
    UPDownTime--;
  }
  GPIOA_ClearITFlagBit(-1);
}
/**
 * @brief           修改左右移动中断方式                                      
 *  @details        修改左右移动中断方式。调用该函数时，若为上升沿中断，则改为下降沿，若为下降沿中断，则改为上升沿。该函数一般在中断发生时调用，即用于实现上升下降沿双边检测
 * 
 */
__attribute__((always_inline)) inline void ChangeLeftRightInterruptEdge()
{
  if(NowLeftRightGroupInterruptForward){
      /// 当前为上升沿状态，准备切换到下降沿状态
      NowLeftRightGroupInterruptForward=0;
      /// 将中断方向修改为下降沿
      R32_PB_CLR |= MoveLeftRight;
    }
    else{
      /// 当前为下降沿状态，准备切换到上升沿状态
      NowLeftRightGroupInterruptForward=1;
      /// 将中断修改为上升沿
      R32_PB_OUT |= MoveLeftRight;
    }
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
  /// 判断是同向方向移动产生的中断还是反向移动产生的中断
  if(GPIOB_ReadITFlagBit(MoveLeftRightInterruptFrom)){
    /// 同向移动，计数正增，并且切换检测沿
    LeftRightTime++;
    ChangeLeftRightInterruptEdge();
  }
  else{
    /// 该中断来自于反方向，不进行中断预测方向修正，除非在MouseEvent()中处理xy数据时方向确实发生改变，即交给MouseEvent()切换方向
    LeftRightTime--;
  }
 
  GPIOB_ClearITFlagBit(-1); // 后期有可能将轨迹球的GPIO拆为两部分
}

/**
 * @brief           检测位移，上报位移
 *  @details        检测在此期间各方向霍尔元件触发次数，从而确定在各方向的移动距离。不使用任何算法。
 * 
 */
void MoveByLocation()
{
  int x = 0, y = 0;
  /// 根据当前中断累加正方向确定移动计数方向
  if(MoveUPDownInterruptFrom==MoveUP){
    y=-UPDownTime;
  }
  else{
    y=UPDownTime;
  }
  if(MoveLeftRightInterruptFrom==MoveRight){
    x=LeftRightTime;
  }
  else{
    x=-LeftRightTime;
  }
  
  if(y>0)MoveUPDownInterruptFrom=MoveDown;
  if(y<0)MoveUPDownInterruptFrom=MoveUP;
  if(x>0)MoveLeftRightInterruptFrom=MoveRight;
  if(x<0)MoveLeftRightInterruptFrom=MoveLeft;
  /*这段代码存在问题，无法修改方向，具体原因不明，有可能为CPU乱序执行导致
  /// 判断上下方向是否发生反向
  if(UPDownTime<0){
    /// 修改当前预测主方向
    if(MoveUPDownInterruptFrom==MoveUP){
      MoveUPDownInterruptFrom=MoveDown;
    }
    else{
      MoveUPDownInterruptFrom=MoveUP;
    }
    /// 主动进行一次预测，待讨论
     ChangeUPDownInterruptEdge();
  }
  /// 判断左右方向是否发生反向
  if(LeftRightTime<0){
    /// 修改当前预测方向
    if(MoveLeftRightInterruptFrom==MoveRight){
      MoveLeftRightInterruptFrom=MoveLeft;
    }
    else{
      MoveLeftRightInterruptFrom=MoveRight;
    }
    /// 主动进行一次预测，待讨论
    ChangeLeftRightInterruptEdge();
  }*/
  UPDownTime=0;
  LeftRightTime=0;
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
void MoveByHallSpeedHalf(){
  MoveUPDownHappenTime++;
  /// 产生实际移动，重置预测时间
  if(UPDownTime){
        MoveUPDownSpeedTime=MoveUPDownHappenTime/2;
        NowLeaveMoveUPDownTime=MoveUPDownSpeedTime;
        /// 重置上下中断累计计时
    MoveUPDownHappenTime=0;  
    }
  if(NowLeaveMoveUPDownTime){
    /// 当处于插值状态，插值计数自检
  NowLeaveMoveUPDownTime--;
  /// 处于插值时间
  if(NowLeaveMoveUPDownTime==0)UPDownTime++;
  }
  /// 左右中断累计计时
  MoveLeftRightHappenTime++;
   /// 产生实际移动，重置预测时间
  if(LeftRightTime){
        MoveLeftRightSpeedTime=MoveLeftRightHappenTime/2;
        NowLeaveMoveLeftRightTime=MoveLeftRightSpeedTime;
            /// 重置上下中断累计计时
    MoveLeftRightHappenTime=0;
    }
  if(NowLeaveMoveLeftRightTime){
    /// 当处于插值状态，插值计数自检
  NowLeaveMoveLeftRightTime--;
  if(NowLeaveMoveLeftRightTime==0)LeftRightTime++;
  }
  MoveByLocation();
}
/**
 * @brief           霍尔插值移动                                      
 *  @details        移动过程中，将灵敏度的值以平均时间插在多帧移动，效果不好
  * 
 */
void MoveByHallSpeed(){
  /*
  /// 上下中断累计计时
  MoveUPDownHappenTime++;
  /// 产生实际移动，重置预测时间
  if(UPDownTime){
    if(1){
      /// 生成插值时间
      MoveUPDownSpeedTime=MoveUPDownHappenTime/MouseConfigure.details.speed;
      /// 开始倒计时
      NowLeaveMoveUPDownTime=MoveUPDownSpeedTime*MouseConfigure.details.speed-1;
      /// 时间丢失
      if(MoveUPDownHappenTime<0){
        MoveUPDownSpeedTime=1;
        NowLeaveMoveUPDownTime=MouseConfigure.details.speed;
      }
    }
    /// 重置上下中断累计计时
    MoveUPDownHappenTime=0;
    }
  if(NowLeaveMoveUPDownTime){
    /// 当处于插值状态，插值计数自检
  NowLeaveMoveUPDownTime--;
  /// 处于插值时间
  if(((NowLeaveMoveUPDownTime%MoveUPDownSpeedTime)==0))UPDownTime++;
  }
  /// 左右中断累计计时
  MoveLeftRightHappenTime++;
   /// 产生实际移动，重置预测时间
  if(LeftRightTime){
    if(1){
      /// 生成插值时间
      MoveLeftRightSpeedTime=MoveLeftRightHappenTime/MouseConfigure.details.speed;
      /// 开始倒计时
      NowLeaveMoveLeftRightTime=MoveLeftRightSpeedTime*MouseConfigure.details.speed-1;
      /// 时间丢失
      if(MoveLeftRightSpeedTime<2){
        MoveLeftRightSpeedTime=1;
        NowLeaveMoveLeftRightTime=MouseConfigure.details.speed;
      }
    }
    /// 重置上下中断累计计时
    MoveLeftRightHappenTime=0;
    }
  if(NowLeaveMoveLeftRightTime){
    /// 当处于插值状态，插值计数自检
  NowLeaveMoveLeftRightTime--;
    /// 处于插值时间
  if((NowLeaveMoveLeftRightTime%MoveLeftRightSpeedTime)==0)LeftRightTime++;
  }*/
  
    /// 上下中断累计计时
  MoveUPDownHappenTime++;
  /// 产生实际移动，重置预测时间
  if(UPDownTime){
        MoveUPDownSpeedTime=MoveUPDownHappenTime/2;
        NowLeaveMoveUPDownTime=MoveUPDownSpeedTime;
        /// 重置上下中断累计计时
    MoveUPDownHappenTime=0;  
    }
  if(NowLeaveMoveUPDownTime){
    /// 当处于插值状态，插值计数自检
  NowLeaveMoveUPDownTime--;
  /// 处于插值时间
  if(NowLeaveMoveUPDownTime==0)UPDownTime++;
  }
  /// 左右中断累计计时
  MoveLeftRightHappenTime++;
   /// 产生实际移动，重置预测时间
  if(LeftRightTime){
        MoveLeftRightSpeedTime=MoveLeftRightHappenTime/2;
        NowLeaveMoveLeftRightTime=MoveLeftRightSpeedTime;
            /// 重置上下中断累计计时
    MoveLeftRightHappenTime=0;
    }
  if(NowLeaveMoveLeftRightTime){
    /// 当处于插值状态，插值计数自检
  NowLeaveMoveLeftRightTime--;
  if(NowLeaveMoveLeftRightTime==0)LeftRightTime++;
  }
  {
      int x = 0, y = 0;
  /// 根据当前中断累加正方向确定移动计数方向
  if(MoveUPDownInterruptFrom==MoveUP){
    y=-UPDownTime;
  }
  else{
    y=UPDownTime;
  }
  if(MoveLeftRightInterruptFrom==MoveRight){
    x=LeftRightTime;
  }
  else{
    x=-LeftRightTime;
  }
  
  if(y>0)MoveUPDownInterruptFrom=MoveDown;
  if(y<0)MoveUPDownInterruptFrom=MoveUP;
  if(x>0)MoveLeftRightInterruptFrom=MoveRight;
  if(x<0)MoveLeftRightInterruptFrom=MoveLeft;
  /*这段代码存在问题，无法修改方向，具体原因不明，有可能为CPU乱序执行导致
  /// 判断上下方向是否发生反向
  if(UPDownTime<0){
    /// 修改当前预测主方向
    if(MoveUPDownInterruptFrom==MoveUP){
      MoveUPDownInterruptFrom=MoveDown;
    }
    else{
      MoveUPDownInterruptFrom=MoveUP;
    }
    /// 主动进行一次预测，待讨论
     ChangeUPDownInterruptEdge();
  }
  /// 判断左右方向是否发生反向
  if(LeftRightTime<0){
    /// 修改当前预测方向
    if(MoveLeftRightInterruptFrom==MoveRight){
      MoveLeftRightInterruptFrom=MoveLeft;
    }
    else{
      MoveLeftRightInterruptFrom=MoveRight;
    }
    /// 主动进行一次预测，待讨论
    ChangeLeftRightInterruptEdge();
  }*/
  UPDownTime=0;
  LeftRightTime=0;
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
  }
  MouseData.details.z = 0; // 计算需要上报的数据
  }
}
#endif