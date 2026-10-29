#ifndef PAW_TEST_CONFIG_H
#define PAW_TEST_CONFIG_H

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef enum { GPIO_ModeIN_Floating, GPIO_ModeOut_PP_5mA } GPIOModeTypeDef;
#define INVALID_TASK_ID 0xFF

void test_pin_write(unsigned char port, unsigned long mask, unsigned char high);
void test_pin_mode(unsigned char port, unsigned long mask, GPIOModeTypeDef mode);
unsigned long test_pin_read(unsigned char port, unsigned long mask);
void test_delay(unsigned long microseconds);
unsigned char TMOS_ProcessEventRegister(uint16 (*callback)(uint8, uint16));
unsigned char tmos_set_event(unsigned char task, unsigned short event);
unsigned char tmos_start_task(unsigned char task, unsigned short event, unsigned long ticks);
unsigned char tmos_stop_task(unsigned char task, unsigned short event);
unsigned char tmos_clear_event(unsigned char task, unsigned short event);

#define GPIOA_SetBits(mask) test_pin_write(0, mask, 1)
#define GPIOB_SetBits(mask) test_pin_write(1, mask, 1)
#define GPIOA_ResetBits(mask) test_pin_write(0, mask, 0)
#define GPIOB_ResetBits(mask) test_pin_write(1, mask, 0)
#define GPIOA_ModeCfg(mask, mode) test_pin_mode(0, mask, mode)
#define GPIOB_ModeCfg(mask, mode) test_pin_mode(1, mask, mode)
#define GPIOA_ReadPortPin(mask) test_pin_read(0, mask)
#define GPIOB_ReadPortPin(mask) test_pin_read(1, mask)
#define DelayUs(us) test_delay(us)

#endif
