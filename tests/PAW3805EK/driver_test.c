/* Host-only GPIO/TMOS model. This does not validate physical sensor timing. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define PAW3805EK_CONFIGURED 1
#define PAW3805EK_NCS_PORT 0
#define PAW3805EK_NCS_PIN 0
#define PAW3805EK_SCLK_PORT 0
#define PAW3805EK_SCLK_PIN 1
#define PAW3805EK_SDIO_PORT 0
#define PAW3805EK_SDIO_PIN 2
#include "../../src/PAW3805EK.c"

static unsigned char levels[2][24], outputs[2][24], registers[128];
static unsigned char selected, tx_bits, rx_bits;
static unsigned short sent;
static unsigned long gpio_calls, delay_calls, delay_max;
static unsigned char commands[128], values[128];
static unsigned int transaction_count, register_calls;
static unsigned short immediate, timed;
static unsigned long scheduled_ticks;
static unsigned char fail_register, fail_schedule, fail_set;
static uint16 (*task_callback)(uint8, uint16);

static unsigned char pin_number(unsigned long mask)
{
    unsigned char pin = 0;
    assert(mask != 0 && (mask & (mask - 1)) == 0);
    while ((mask >>= 1) != 0) ++pin;
    assert(pin < 24);
    return pin;
}

void test_pin_write(unsigned char port, unsigned long mask, unsigned char high)
{
    unsigned char pin = pin_number(mask);
    unsigned char previous = levels[port][pin];
    ++gpio_calls;
    levels[port][pin] = high != 0;
    if (port != 0) return;
    if (pin == 0 && !high)
    {
        assert(!selected && levels[0][1] && outputs[0][0] && outputs[0][1]);
        assert(outputs[0][2]);
        selected = 1;
        tx_bits = rx_bits = 0;
        sent = 0;
    }
    else if (pin == 0 && high && selected)
    {
        assert(levels[0][1]);
        assert(transaction_count < sizeof(commands));
        if (tx_bits == 16)
        {
            assert(rx_bits == 0 && (sent & 0x8000));
            commands[transaction_count] = (unsigned char)(sent >> 8);
            values[transaction_count] = (unsigned char)sent;
            registers[(sent >> 8) & 0x7F] = (unsigned char)sent;
        }
        else
        {
            assert(tx_bits == 8 && rx_bits == 8 && !(sent & 0x80));
            commands[transaction_count] = (unsigned char)sent;
            values[transaction_count] = registers[sent];
        }
        ++transaction_count;
        selected = 0;
    }
    else if (pin == 1 && selected && high && !previous)
    {
        if (outputs[0][2])
        {
            assert(rx_bits == 0 && tx_bits < 16);
            sent = (unsigned short)((sent << 1) | levels[0][2]);
            ++tx_bits;
        }
        else
        {
            assert(tx_bits == 8 && !(sent & 0x80) && rx_bits < 8);
            ++rx_bits;
        }
    }
}

void test_pin_mode(unsigned char port, unsigned long mask, GPIOModeTypeDef mode)
{
    unsigned char pin = pin_number(mask);
    ++gpio_calls;
    if (port == 0 && pin == 2)
    {
        if (mode == GPIO_ModeOut_PP_5mA) assert(!selected);
        if (selected) assert(tx_bits == 8 && rx_bits == 0);
    }
    outputs[port][pin] = mode == GPIO_ModeOut_PP_5mA;
}

unsigned long test_pin_read(unsigned char port, unsigned long mask)
{
    assert(port == 0 && mask == 4);
    assert(selected && !outputs[0][2] && levels[0][1]);
    assert(tx_bits == 8 && rx_bits >= 1 && rx_bits <= 8);
    return ((registers[sent] >> (8 - rx_bits)) & 1) ? mask : 0;
}

void test_delay(unsigned long us)
{
    assert(us >= 1 && us <= 10);
    ++delay_calls;
    if (us > delay_max) delay_max = us;
}

unsigned char TMOS_ProcessEventRegister(uint16 (*callback)(uint8, uint16))
{
    ++register_calls;
    task_callback = callback;
    return fail_register ? INVALID_TASK_ID : 7;
}

unsigned char tmos_set_event(unsigned char task, unsigned short event)
{
    assert(task == 7);
    if (fail_set) return 1;
    immediate |= event;
    return 0;
}

unsigned char tmos_start_task(unsigned char task, unsigned short event, unsigned long ticks)
{
    assert(task == 7);
    if (fail_schedule) return 0;
    timed |= event;
    scheduled_ticks = ticks;
    return 1;
}

unsigned char tmos_stop_task(unsigned char task, unsigned short event)
{
    assert(task == 7 && (event == 1 || event == 2));
    timed &= (unsigned short)~event;
    return 0;
}

unsigned char tmos_clear_event(unsigned char task, unsigned short event)
{
    assert(task == 7);
    immediate &= (unsigned short)~event;
    return 0;
}

static void run_immediate(void)
{
    unsigned short event = immediate;
    assert(event);
    immediate = 0;
    assert(task_callback(7, event) == 0);
}

static void run_timer(void)
{
    unsigned short event = timed;
    assert(event);
    timed = 0;
    assert(task_callback(7, event) == 0);
}

static void clear_log(void)
{
    assert(!selected);
    transaction_count = 0;
    memset(commands, 0, sizeof(commands));
    memset(values, 0, sizeof(values));
}

static void identify_and_reset(void)
{
    registers[0] = 0x31;
    registers[1] = 0x61;
    clear_log();
    PAW3805EK_Init();
    assert(PAW3805EK_GetState() == PAW3805EK_STARTING);
    assert(transaction_count == 0);
    run_immediate();
    assert(transaction_count == 3);
    assert(commands[0] == 0 && commands[1] == 1);
    assert(commands[2] == 0x86 && values[2] == 0x80);
    assert(scheduled_ticks == 8 && timed == 2);
    assert(PAW3805EK_GetState() == PAW3805EK_STARTING);
    run_timer();
    assert(PAW3805EK_GetState() == PAW3805EK_READY);
    assert(transaction_count == 5 && commands[3] == 0 && commands[4] == 1);
}

int main(void)
{
    signed short x, y;
    unsigned long before;
    unsigned int enrolled;
    assert(sizeof(signed short) == 2);
    assert(PAW3805EK_GetState() == PAW3805EK_UNCONFIGURED);

    fail_register = 1;
    PAW3805EK_Init();
    assert(PAW3805EK_GetState() == PAW3805EK_ERROR && gpio_calls == 0);
    fail_register = 0;
    identify_and_reset();
    enrolled = register_calls;

    clear_log();
    registers[2] = 0x80;
    registers[3] = 0xFF; registers[4] = 0;
    registers[0x11] = 0x7F; registers[0x12] = 0x80;
    assert(PAW3805EK_ReadMotion(&x, &y) == 1 && x == 32767 && y == -32768);
    assert(transaction_count == 5);
    assert(commands[0] == 2 && commands[1] == 3 && commands[2] == 4);
    assert(commands[3] == 0x11 && commands[4] == 0x12);
    /* Status remains asserted: one call still reads exactly one sample. */
    registers[3] = 0xFF; registers[4] = 1;
    registers[0x11] = 0xFF; registers[0x12] = 0;
    assert(PAW3805EK_ReadMotion(&x, &y) == 1 && x == -1 && y == 1);
    assert(transaction_count == 10);
    registers[2] = 0;
    assert(PAW3805EK_ReadMotion(&x, &y) == 0 && x == 0 && y == 0);
    assert(transaction_count == 11);
    before = gpio_calls;
    x = 8;
    assert(!PAW3805EK_ReadMotion(&x, 0) && x == 0);
    assert(!PAW3805EK_ReadMotion(&x, &x) && x == 0);
    assert(gpio_calls == before);

    assert(PAW3805EK_PowerDown() == 1);
    assert(commands[11] == 0x86 && values[11] == 0x08);
    assert(PAW3805EK_GetState() == PAW3805EK_POWERED_DOWN);
    before = gpio_calls;
    assert(!PAW3805EK_PowerDown() && !PAW3805EK_ReadMotion(&x, &y));
    assert(gpio_calls == before);

    /* Restart during reset cancels the old verify timer and reuses the task. */
    PAW3805EK_Init(); run_immediate();
    assert(timed == 2);
    PAW3805EK_Init();
    assert(timed == 0 && immediate == 1 && register_calls == enrolled);
    run_immediate(); run_timer();
    assert(PAW3805EK_GetState() == PAW3805EK_READY);

    /* A missing/mismatched device is read only and receives no reset writes. */
    clear_log(); registers[0] = 0xFF;
    PAW3805EK_Init(); run_immediate();
    assert(scheduled_ticks == 16);
    run_timer(); assert(scheduled_ticks == 16); run_timer();
    assert(PAW3805EK_GetState() == PAW3805EK_NOT_FOUND && timed == 0);
    assert(transaction_count == 6);
    for (before = 0; before < transaction_count; ++before) assert(commands[before] < 0x80);

    registers[0] = 0x31;
    clear_log(); fail_schedule = 1;
    PAW3805EK_Init(); run_immediate();
    assert(PAW3805EK_GetState() == PAW3805EK_ERROR);
    fail_schedule = 0;
    identify_and_reset();
    fail_set = 1;
    PAW3805EK_Init();
    assert(PAW3805EK_GetState() == PAW3805EK_ERROR);
    fail_set = 0;

    assert(delay_calls > 0 && delay_max == 1 && !selected && levels[0][1]);
    puts("PASS PAW3805EK host bus/state tests (not hardware validation)");
    return 0;
}
