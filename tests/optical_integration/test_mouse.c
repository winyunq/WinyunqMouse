/** Host-only tests of the production MouseEvent optical integration. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "CONFIG.h"
#define __WinyunqConfigure_C
#include "WinyunqConfigure.h"
/* Including the production unit also lets the tests inspect its private queue. */
#include "../../src/WinyunqMouse.c"

static unsigned char ble_result, button_down, sample_valid;
static signed short sample_x, sample_y;
static unsigned int report_count, accepted_count, init_count, led_count;
static unsigned char last_report[4];
static signed long accepted_x, accepted_y;

void EEPROM_READ(unsigned long address, void *data, unsigned long length)
{ (void)address; memset(data, 0, length); }
void EEPROM_WRITE(unsigned long address, const void *data, unsigned long length)
{ (void)address; (void)data; assert(length == 8); }
void GPIOB_ModeCfg(unsigned long pins, unsigned char mode)
{ assert(pins == LeftTouch); assert(mode == GPIO_ModeIN_PU); }
unsigned long GPIOB_ReadPortPin(unsigned long pins)
{ assert(pins == LeftTouch); return button_down ? 0 : pins; }
void InitLED(void) { ++led_count; }
void PAW3805EK_Init(void) { ++init_count; }
unsigned char PAW3805EK_PowerDown(void) { assert(0); return 0; }
unsigned char PAW3805EK_ReadMotion(signed short *dx, signed short *dy)
{
  *dx = sample_valid ? sample_x : 0;
  *dy = sample_valid ? sample_y : 0;
  unsigned char result = sample_valid;
  sample_valid = 0;
  return result;
}
uint8 BLEHIDReport(uint8 id, uint8 type, uint8 len, uint8 *data)
{
  assert(id == 0 && type == 1 && len == 4);
  ++report_count;
  memcpy(last_report, data, 4);
  assert((signed char)data[1] >= -127);
  assert((signed char)data[2] >= -127);
  if (!ble_result) {
    ++accepted_count;
    accepted_x += (signed char)data[1];
    accepted_y += (signed char)data[2];
  }
  return ble_result;
}
static void reset_test(void)
{
  ble_result = button_down = sample_valid = 0;
  report_count = accepted_count = init_count = led_count = 0;
  accepted_x = accepted_y = 0;
  memset(last_report, 0, sizeof last_report);
  LastClick = 0;
  UsingSleepTime = 0;
  MouseInit();
  assert(init_count == 1 && led_count == 1);
  assert(MouseConfigure.details.report == 17);
}
static void sample(signed short x, signed short y)
{ sample_x = x; sample_y = y; sample_valid = 1; }
static void drain(unsigned int count)
{ while (count--) MouseEvent(); }
static void transformed(signed long *x, signed long *y)
{
  (void)x; (void)y;
#if PAW3805EK_SWAP_XY
  signed long t = *x; *x = *y; *y = t;
#endif
#if PAW3805EK_INVERT_X
  *x = -*x;
#endif
#if PAW3805EK_INVERT_Y
  *y = -*y;
#endif
}
int main(void)
{
  signed long x, y;
  assert(sizeof(signed short) == 2);
  reset_test();
  MouseEvent();
  assert(report_count == 0); /* No idle or startup motion. */

  sample(500, -500); drain(8);
  x = 500; y = -500; transformed(&x, &y);
  assert(accepted_x == x && accepted_y == y);
  assert(accepted_count == 4); /* 127 + 127 + 127 + 119, no wrap/drop. */

  reset_test(); sample(-32768, 32767); drain(300);
  x = -OPTICAL_MAX_PENDING; y = OPTICAL_MAX_PENDING; transformed(&x, &y);
  assert(accepted_x == x && accepted_y == y);
  assert(accepted_count == 8 && OpticalClippedAxes == 2);
  assert(OpticalPendingX == 0 && OpticalPendingY == 0);

  reset_test(); sample(300, -200); ble_result = 1; drain(2);
  assert(accepted_count == 0);
  assert(!MouseMotionCanDrain()); /* Busy transport backs off, no 1600 Hz spin. */
  ble_result = 0; MouseEvent(); assert(MouseMotionCanDrain()); drain(4);
  x = 300; y = -200; transformed(&x, &y);
  assert(accepted_x == x && accepted_y == y); /* Retry without double-count. */
  assert(!MouseMotionCanDrain());

  reset_test(); sample(400, 400); MouseEvent();
  ble_result = bleNotReady; MouseEvent();
  assert(OpticalPendingX == 0 && OpticalPendingY == 0);
  assert(!MouseMotionCanDrain());
  ble_result = 0;
  unsigned int before_reconnect = accepted_count;
  sample(90, 90); MouseEvent(); /* Device-side old data is discarded once. */
  assert(accepted_count == before_reconnect + 1); /* Button-state resynchronization. */
  assert(last_report[0] == 0 && last_report[1] == 0 && last_report[2] == 0);
  sample(5, 5); MouseEvent();
  assert(accepted_count == before_reconnect + 2);

  reset_test(); sample(400, 400); MouseEvent();
  MouseResetMotion(); /* Notification disable/disconnect callback. */
  assert(OpticalPendingX == 0 && OpticalPendingY == 0);
  assert(!MouseMotionCanDrain());

  reset_test(); button_down = 1; MouseEvent(); assert(LastClick & 1);
  MouseResetMotion(); button_down = 0; MouseEvent();
  assert(accepted_count == 2 && last_report[0] == 0); /* Release after notify toggle. */

  reset_test(); button_down = 1; ble_result = 1; MouseEvent();
  assert(LastClick == 0);
  ble_result = 0; MouseEvent(); assert(LastClick & 1);
  button_down = 0; MouseEvent(); assert(LastClick == 0);
  assert(accepted_count == 2); /* Press and release preserved after failure. */

  reset_test(); sample(500, 500); MouseEvent();
  MouseConfigure.details.help = 1; sample(300, 300); MouseEvent();
  assert(OpticalPendingX == 0 && OpticalPendingY == 0);
  MouseConfigure.details.help = 0;
  unsigned int reports = report_count; MouseEvent(); assert(report_count == reports);

  reset_test(); MouseConfigure.details.trackball = 1; sample(10, 20); MouseEvent();
  x = 10; y = 20; transformed(&x, &y);
  assert(accepted_x == y && accepted_y == -x);
  assert(OpticalAccumulate(999, 100) == OPTICAL_MAX_PENDING);
  assert(OpticalAccumulate(-999, -100) == -OPTICAL_MAX_PENDING);
  puts("PASS optical MouseEvent: bounded queue, reconnect discard, drain/backoff, buttons, help, orientation");
  return 0;
}
