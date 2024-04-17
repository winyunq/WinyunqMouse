#ifndef WinyunqMouseHallDriver_H
#define WinyunqMouseHallDriver_H
#ifndef  __WinyunqMouseHallDriver_C
extern void MoveByHallSpeed();
extern void MoveByHallSpeedHalf();
/// 若此时不在WinyunqConfigure.c内，则以extern形式访问MouseConfigure
#define Extern extern

#else

#define Extern 

#endif
#undef Extern
#endif
