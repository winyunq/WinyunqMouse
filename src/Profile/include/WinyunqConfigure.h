/**
 * @file            文件名:                WinyunqConfigure.h
 * @brief           函数简介                                      
 *  @details        【不能简单描述则必选】如果不能一句话描述函数，请使用Details进行详细描述
 * 
 * @author          Winyunq             创建
 * @version         版本:                 1.0.0
 * @date            2022-11-12          创建
 * 
 * Copyright (c) 2022  Winyunq科技公司
 */

#ifndef __WinyunqConfigure_H
#define __WinyunqConfigure_H
#ifdef __cplusplus
extern "C" {
#endif
typedef char boolean;
extern bStatus_t AddServiceWinyunqMouseConfigure();
#define MouseConfigureSize 8

#define InitConfigureData() EEPROM_READ(0, MouseConfigure.data, MouseConfigureSize)

#ifndef  __WinyunqConfigure_C
/// 若此时不在WinyunqConfigure.c内，则以extern形式访问MouseConfigure
#define Extern extern

#else

#define Extern 

#endif

Extern union
{
  struct
  {
    boolean right : 1;   //左右键交换
    boolean trackball : 1; //trackball
    boolean help : 1; //辅助按键模式
    boolean LEDOn : 1; // LED常开
    boolean MoveTypeBySpeed : 1 ; //以速度调整移速
    uint8 speed; // 移动速度
    uint8 report; // 数据回报率
    uint8 sleep; // 休眠时间
    uint8 SpeedMoveListTime; //以速度调整移速的速度阈值
  } details;
  uint8 data[MouseConfigureSize];
} MouseConfigure;

Extern void ConfigureMouse(char* buffer);

#ifdef __cplusplus
}

#endif
#undef Extern
#endif
