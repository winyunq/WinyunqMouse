#ifndef WINYUNQLED_H
#define WINYUNQLED_H
extern tmosTaskID ConnectPower;
extern uint8 noConnect;
extern uint32 SearchSleepTime;
extern void FindConnectPower();
extern void LockLED(uint32 time);
extern void InitLED();

#endif