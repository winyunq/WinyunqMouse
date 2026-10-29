/**
 * @file PAW3805EK.c
 * @brief PAW3805EK-CJV1 裸芯片三线 GPIO SPI 驱动。
 * @details 依据确切型号的公开协议事实独立编写，不包含 Cypress 驱动代码。
 * Mode 3、寄存器读写格式、双 ID、Motion 位、XY 顺序及复位/掉电值的证据：
 * https://github.com/Infineon/Code-Examples-BT-SDK-for-ModusToolbox/blob/fb65fbe6fbc0f0daa9b18837354407148d554acc/CYW920820EVB-02/apps/demo/hid/ble_mouse/motion/PAW3805_opticalsensor.c
 * @warning GPIO 半周期和换向等待是本实现的保守工程值，不冒充芯片最小时序。
 * 尚需用真实芯片和逻辑分析仪验证；不会配置未经证实的寄存器位。
 */
#include "PAW3805EK.h"

#if (PAW3805EK_CONFIGURED != 0) && (PAW3805EK_CONFIGURED != 1)
#error "PAW3805EK_CONFIGURED must be 0 or 1"
#endif

static PAW3805EK_State paw_state = PAW3805EK_UNCONFIGURED;

#if PAW3805EK_CONFIGURED
#include "CONFIG.h"

/* GUI 只生成连线宏；延时可由构建参数覆盖，不依赖生成器保留这些值。 */
#ifndef PAW3805EK_HALF_CLOCK_US
#define PAW3805EK_HALF_CLOCK_US 1
#endif
#ifndef PAW3805EK_READ_TURNAROUND_US
#define PAW3805EK_READ_TURNAROUND_US 1
#endif
#if PAW3805EK_HALF_CLOCK_US < 1 || PAW3805EK_HALF_CLOCK_US > 10
#error "PAW3805EK_HALF_CLOCK_US must be in the bounded engineering range 1..10"
#endif
#if PAW3805EK_READ_TURNAROUND_US < 1 || PAW3805EK_READ_TURNAROUND_US > 10
#error "PAW3805EK_READ_TURNAROUND_US must be in the bounded engineering range 1..10"
#endif

#define PAW_PIN_VALID(port, pin) (((port) == 0 && (pin) >= 0 && (pin) <= 15) || \
                                ((port) == 1 && (pin) >= 0 && (pin) <= 23))
#define PAW_PIN_RESERVED(port, pin) (((port) == 0 && ((pin) == 9 || (pin) == 12 || (pin) == 13)) || \
                                   ((port) == 1 && (pin) == 22))
#define PAW_PINS_EQUAL(ap, an, bp, bn) ((ap) == (bp) && (an) == (bn))
#if !PAW_PIN_VALID(PAW3805EK_NCS_PORT, PAW3805EK_NCS_PIN) || \
    !PAW_PIN_VALID(PAW3805EK_SCLK_PORT, PAW3805EK_SCLK_PIN) || \
    !PAW_PIN_VALID(PAW3805EK_SDIO_PORT, PAW3805EK_SDIO_PIN)
#error "Invalid PAW3805EK GPIO bank or pin number"
#endif
#if PAW_PIN_RESERVED(PAW3805EK_NCS_PORT, PAW3805EK_NCS_PIN) || \
    PAW_PIN_RESERVED(PAW3805EK_SCLK_PORT, PAW3805EK_SCLK_PIN) || \
    PAW_PIN_RESERVED(PAW3805EK_SDIO_PORT, PAW3805EK_SDIO_PIN)
#error "PAW3805EK GPIO conflicts with existing UART, battery, LED or button"
#endif
#if PAW_PINS_EQUAL(PAW3805EK_NCS_PORT, PAW3805EK_NCS_PIN, PAW3805EK_SCLK_PORT, PAW3805EK_SCLK_PIN) || \
    PAW_PINS_EQUAL(PAW3805EK_NCS_PORT, PAW3805EK_NCS_PIN, PAW3805EK_SDIO_PORT, PAW3805EK_SDIO_PIN) || \
    PAW_PINS_EQUAL(PAW3805EK_SCLK_PORT, PAW3805EK_SCLK_PIN, PAW3805EK_SDIO_PORT, PAW3805EK_SDIO_PIN)
#error "PAW3805EK NCS, SCLK and SDIO must use different GPIOs"
#endif

#define PAW_EVENT_PROBE  0x0001
#define PAW_EVENT_VERIFY 0x0002
/* TMOS tick = 0.625 ms；8 ticks = 5 ms，扣除一个 tick 的量化相位仍有
 * 4.375 ms，避免实际复位等待短于已知的 4 ms。 */
#define PAW_RESET_TICKS 8
/* 应用层失败重试策略，不是传感器规定的上电等待时间。 */
#define PAW_RETRY_TICKS 16
#define PAW_PROBE_LIMIT 3

static unsigned char paw_task = INVALID_TASK_ID;
static unsigned char paw_probe_count;
static unsigned char paw_waiting_reset;

/** @brief 设置一个板级 GPIO 电平，端口 0 为 A、1 为 B。 */
static void paw_pin_write(unsigned char port, unsigned char pin, unsigned char high)
{
    unsigned long mask = 1UL << pin;
    if (port == 0)
    {
        if (high) GPIOA_SetBits(mask);
        else GPIOA_ResetBits(mask);
    }
    else
    {
        if (high) GPIOB_SetBits(mask);
        else GPIOB_ResetBits(mask);
    }
}

/** @brief 配置 GPIO 方向；SDIO 接收时不启用 MCU 内部上拉/下拉。 */
static void paw_pin_mode(unsigned char port, unsigned char pin, unsigned char output)
{
    GPIOModeTypeDef mode = output ? GPIO_ModeOut_PP_5mA : GPIO_ModeIN_Floating;
    if (port == 0) GPIOA_ModeCfg(1UL << pin, mode);
    else GPIOB_ModeCfg(1UL << pin, mode);
}

/** @brief 驱动 Mode 3 时钟：空闲高电平，下降沿换位，上升沿采样。 */
static void paw_clock(unsigned char high)
{
    paw_pin_write(PAW3805EK_SCLK_PORT, PAW3805EK_SCLK_PIN, high);
}

/** @brief 在片选有效期间发送一个 MSB-first 字节，结束时 SCLK 为高。 */
static void paw_send(unsigned char value)
{
    unsigned char bit;
    for (bit = 0; bit < 8; ++bit)
    {
        paw_clock(0);
        paw_pin_write(PAW3805EK_SDIO_PORT, PAW3805EK_SDIO_PIN, (value & 0x80) != 0);
        DelayUs(PAW3805EK_HALF_CLOCK_US);
        paw_clock(1);
        DelayUs(PAW3805EK_HALF_CLOCK_US);
        value <<= 1;
    }
}

/** @brief SDIO 已释放为输入时接收一个 MSB-first 字节。 */
static unsigned char paw_receive(void)
{
    unsigned char bit, value = 0;
    unsigned long mask = 1UL << PAW3805EK_SDIO_PIN;
    for (bit = 0; bit < 8; ++bit)
    {
        paw_clock(0);
        DelayUs(PAW3805EK_HALF_CLOCK_US);
        paw_clock(1);
        value <<= 1;
        if (PAW3805EK_SDIO_PORT == 0)
            value |= GPIOA_ReadPortPin(mask) != 0;
        else
            value |= GPIOB_ReadPortPin(mask) != 0;
        DelayUs(PAW3805EK_HALF_CLOCK_US);
    }
    return value;
}

/** @brief 打开独占总线事务；不屏蔽中断。 */
static void paw_begin(void)
{
    paw_pin_mode(PAW3805EK_SDIO_PORT, PAW3805EK_SDIO_PIN, 1);
    paw_pin_write(PAW3805EK_NCS_PORT, PAW3805EK_NCS_PIN, 0);
    DelayUs(PAW3805EK_HALF_CLOCK_US);
}

/** @brief 先释放片选，再释放 SDIO，保留高电平空闲时钟。 */
static void paw_end(void)
{
    paw_pin_write(PAW3805EK_NCS_PORT, PAW3805EK_NCS_PIN, 1);
    paw_pin_mode(PAW3805EK_SDIO_PORT, PAW3805EK_SDIO_PIN, 0);
    DelayUs(PAW3805EK_HALF_CLOCK_US);
}

/** @brief 读取寄存器；命令之后立即释放双向 SDIO，避免主从同时驱动。 */
static unsigned char paw_read(unsigned char address)
{
    unsigned char value;
    paw_begin();
    paw_send(address);
    paw_pin_mode(PAW3805EK_SDIO_PORT, PAW3805EK_SDIO_PIN, 0);
    DelayUs(PAW3805EK_READ_TURNAROUND_US);
    value = paw_receive();
    paw_end();
    return value;
}

/** @brief 写寄存器，地址最高位 1 表示写；仅用于已证实的复位/掉电命令。 */
static void paw_write(unsigned char address, unsigned char value)
{
    paw_begin();
    paw_send(address | 0x80);
    paw_send(value);
    paw_end();
}

/** @brief 仅当两个产品 ID 均匹配确切型号时接受传感器。 */
static unsigned char paw_identified(void)
{
    unsigned char first = paw_read(0x00);
    unsigned char second = paw_read(0x01);
    return first == 0x31 && second == 0x61;
}

/**
 * @brief 初始化状态机：探测、软复位、定时复查。
 * @param task_id TMOS 提供的任务编号。
 * @param events 待处理事件位图。
 * @return 未处理事件位，避免吞掉同时到达的另一个本任务事件。
 */
static uint16 paw_process(uint8 task_id, uint16 events)
{
    (void)task_id;
    if (events & PAW_EVENT_PROBE)
    {
        if (paw_state == PAW3805EK_STARTING && !paw_waiting_reset)
        {
            ++paw_probe_count;
            if (paw_identified())
            {
                paw_write(0x06, 0x80);
                paw_waiting_reset = 1;
                if (!tmos_start_task(paw_task, PAW_EVENT_VERIFY, PAW_RESET_TICKS))
                    paw_state = PAW3805EK_ERROR;
            }
            else if (paw_probe_count < PAW_PROBE_LIMIT)
            {
                if (!tmos_start_task(paw_task, PAW_EVENT_PROBE, PAW_RETRY_TICKS))
                    paw_state = PAW3805EK_ERROR;
            }
            else paw_state = PAW3805EK_NOT_FOUND;
        }
        return events ^ PAW_EVENT_PROBE;
    }
    if (events & PAW_EVENT_VERIFY)
    {
        if (paw_state == PAW3805EK_STARTING && paw_waiting_reset)
            paw_state = paw_identified() ? PAW3805EK_READY : PAW3805EK_NOT_FOUND;
        return events ^ PAW_EVENT_VERIFY;
    }
    return 0;
}
#endif

void PAW3805EK_Init(void)
{
#if PAW3805EK_CONFIGURED
    if (paw_task == INVALID_TASK_ID)
        paw_task = TMOS_ProcessEventRegister(paw_process);
    if (paw_task == INVALID_TASK_ID)
    {
        paw_state = PAW3805EK_ERROR;
        return;
    }
    /* Init 不在本任务回调内部调用，因此可安全取消旧定时器和已排队事件。 */
    tmos_stop_task(paw_task, PAW_EVENT_PROBE);
    tmos_stop_task(paw_task, PAW_EVENT_VERIFY);
    tmos_clear_event(paw_task, PAW_EVENT_PROBE | PAW_EVENT_VERIFY);
    paw_pin_write(PAW3805EK_NCS_PORT, PAW3805EK_NCS_PIN, 1);
    paw_pin_write(PAW3805EK_SCLK_PORT, PAW3805EK_SCLK_PIN, 1);
    paw_pin_mode(PAW3805EK_NCS_PORT, PAW3805EK_NCS_PIN, 1);
    paw_pin_mode(PAW3805EK_SCLK_PORT, PAW3805EK_SCLK_PIN, 1);
    paw_pin_mode(PAW3805EK_SDIO_PORT, PAW3805EK_SDIO_PIN, 0);
    paw_probe_count = 0;
    paw_waiting_reset = 0;
    paw_state = PAW3805EK_STARTING;
    if (tmos_set_event(paw_task, PAW_EVENT_PROBE) != 0)
        paw_state = PAW3805EK_ERROR;
#else
    paw_state = PAW3805EK_UNCONFIGURED;
#endif
}

PAW3805EK_State PAW3805EK_GetState(void)
{
    return paw_state;
}

unsigned char PAW3805EK_ReadMotion(signed short *dx, signed short *dy)
{
    if (dx) *dx = 0;
    if (dy) *dy = 0;
    if (!dx || !dy || dx == dy) return 0;
#if PAW3805EK_CONFIGURED
    if (paw_state == PAW3805EK_READY && (paw_read(0x02) & 0x80))
    {
        unsigned char xl = paw_read(0x03);
        unsigned char yl = paw_read(0x04);
        unsigned char xh = paw_read(0x11);
        unsigned char yh = paw_read(0x12);
        unsigned long x = ((unsigned long)xh << 8) | xl;
        unsigned long y = ((unsigned long)yh << 8) | yl;
        /* 显式扩展符号，避免无符号到有符号窄化的实现相关结果。 */
        *dx = (signed short)(x & 0x8000 ? (signed long)x - 65536L : (signed long)x);
        *dy = (signed short)(y & 0x8000 ? (signed long)y - 65536L : (signed long)y);
        return 1;
    }
#endif
    return 0;
}

unsigned char PAW3805EK_PowerDown(void)
{
#if PAW3805EK_CONFIGURED
    if (paw_state == PAW3805EK_READY)
    {
        paw_write(0x06, 0x08);
        paw_state = PAW3805EK_POWERED_DOWN;
        return 1;
    }
#endif
    return 0;
}
