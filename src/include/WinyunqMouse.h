#ifndef WINYUNQMOUSE_H
#define WINYUNQMOUSE_H
#include "CONFIG.h"
#include "CH57x_common.h"
#include "HAL.h"
extern void MouseEvent();
extern void MouseInit();
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
      boolean LEDOn : 1; ///|–bit3:恒为1
      boolean fleft : 1;  ///|–bit4:X 坐标变化的符号位，1表示负数，即鼠标fleft移动
      boolean fdown : 1;  ///|–bit5:Y 坐标变化的符号位，1表示负数，即鼠标fdown移动
      boolean x : 1; ///|–bit6:1 表示 X 坐标的变化量超出－256 ~ 255的范围，0表示没有溢出
      boolean y : 1; ///|–bit7:1 表示 Y 坐标的变化量超出－256 ~ 255的范围,0表示没有溢出
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

#endif
