/**
 * @file WinyunqMouse.h
 * @brief 鼠标应用入口与既有四字节上报缓冲区。
 * @details 保留旧布局以兼容现有调用者。BLE 中 X/Y/滚轮的有效范围
 * 由 hidmouseservice.c 的报告描述符定义为 -127..127；这里的 uint8
 * 保存二补码原始字节，旧符号/溢出字段不是 BLE 的额外坐标位。
 */
#ifndef WINYUNQMOUSE_H
#define WINYUNQMOUSE_H
#include "CONFIG.h"
#include "HAL.h"
/** @brief 在现有上报事件中采样按键/位移并按需发送 HID 报告。 */
extern void MouseEvent();
/** @brief 加载运行配置，初始化现有按键、传感器硬件和 LED。 */
extern void MouseInit();
/** @brief 按 UsingSleep 编译开关执行原有低功耗关机流程。 */
extern void GoSleep();
#ifndef  __WINYUNQMOUSE_C
/// 若此时不在WinyunqConfigure.c内，则以extern形式访问MouseConfigure
#define Extern extern

#else

#define Extern 

#endif

Extern union
{
  struct
  {
    volatile struct
    {
      boolean left : 1;  ///|–bit0:1表示left按下
      boolean right : 1;  ///|–bit1:1表示右键按下
      boolean mid : 1;  ///|–bit2:1表示mid按下
      boolean LEDOn : 1; ///< 旧布局保留位，不是 BLE 位移的一部分。
      boolean fleft : 1; ///< 旧符号标志；BLE X 的符号在坐标字节内。
      boolean fdown : 1; ///< 旧符号标志；BLE Y 的符号在坐标字节内。
      boolean x : 1; ///< 旧溢出标志；不会扩大 BLE X 的有效范围。
      boolean y : 1; ///< 旧溢出标志；不会扩大 BLE Y 的有效范围。
    } situation;              ///第1个字节:详情如上
     uint8 x;             ///第2个字节:X轴位移量
     uint8 y;             ///第3个字节:Y轴位移量
     uint8 z;             ///第4个字节:Z轴位移量,即滚轮
  } details;
  /// 以缓冲区形式访问
  uint8 buffer[4];
  /// 以一个字节数据访问
  int32 data; // 32位CPU一次性清空
} MouseData; //上报数据表,4字节

#undef Extern
#endif
