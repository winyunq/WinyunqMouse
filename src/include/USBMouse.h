#ifndef __USBMouse_H
#define __USBMouse_H
extern void USBModeInit();
extern void DevHIDReport(uint8_t* buffer,int size);
#endif