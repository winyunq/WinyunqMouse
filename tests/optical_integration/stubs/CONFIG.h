#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H
typedef char boolean;
typedef unsigned char uint8;
typedef unsigned char uint8_t;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long uint32_t;
typedef signed long int32;
typedef unsigned char bStatus_t;
#define UsingPAW3805EK 1
#define UsingBLE 1
#define bleNotReady 0x10
#define LeftTouch (1UL << 22)
#define GPIO_ModeIN_PU 0
#define MouseConfigureSize 8
#define WinyunqMouseSleepTime (MouseConfigure.details.sleep * 5 * 2000)
void EEPROM_READ(unsigned long address, void *data, unsigned long length);
void EEPROM_WRITE(unsigned long address, const void *data, unsigned long length);
void GPIOB_ModeCfg(unsigned long pins, unsigned char mode);
unsigned long GPIOB_ReadPortPin(unsigned long pins);
#endif
