#ifndef WINYUNQLED_H
#define WINYUNQLED_H
#if(LEDOpenLevel == 1)
#define OpenLED() GPIOA_SetBits(LEDIndicator)
#define CloseLED() GPIOA_ResetBits(LEDIndicator)
#else
#define OpenLED() GPIOA_ResetBits(LEDIndicator)
#define CloseLED() GPIOA_SetBits(LEDIndicator)
#endif
extern tmosTaskID ConnectPower;
extern uint8 noConnect;
extern uint32 SearchSleepTime;
extern void FindConnectPower();
extern void LockLED(uint32 time);
extern void InitLED();
extern uint16 DefaultLED(uint8 task_id, uint16 events );
#endif