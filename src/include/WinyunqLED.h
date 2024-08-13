#ifndef WINYUNQLED_H
#define WINYUNQLED_H
#if(LEDOpenLevel == 1)
  #ifdef LEDPWMChannel
    #define OpenLED() PWMX_ACTOUT(LEDPWMChannel, 64, High_Level, ENABLE)
    #define CloseLED() PWMX_ACTOUT(LEDPWMChannel, 0, High_Level, ENABLE)
  #else
    #define OpenLED() GPIOA_SetBits(LEDIndicator)
    #define CloseLED() GPIOA_ResetBits(LEDIndicator)
  #endif
#else
  #ifdef LEDPWMChannel
    #define OpenLED() PWMX_ACTOUT(LEDPWMChannel, 64, Low_Level, ENABLE)
    #define CloseLED() PWMX_ACTOUT(LEDPWMChannel, 0, Low_Level, ENABLE)
  #else
    #define OpenLED() GPIOA_ResetBits(LEDIndicator)
    #define CloseLED() GPIOA_SetBits(LEDIndicator)
  #endif
#endif
#ifndef  __WINYUNQLED_C

#define Extern extern

#else

#define Extern 

#endif
Extern uint8 noConnect;
Extern uint32 SearchSleepTime;
Extern void FindConnectPower();
Extern void LockLED(uint32 time);
Extern void InitLED();
Extern uint16 DefaultLED(uint8 task_id, uint16 events );
#endif