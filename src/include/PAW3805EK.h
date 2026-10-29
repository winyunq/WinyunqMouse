/**
 * @file PAW3805EK.h
 * @brief 裸芯片 PAW3805EK-CJV1 的最小三线 SPI/TMOS 驱动接口。
 * @details 初始化、复位等待和有限重试交给 TMOS；每次采样最多读取
 * 一组位移。不需要 MOTION/MFIO 引脚，不使用 Quick Burst 或未核实的 CPI 写入。
 * 本接口不管理外部供电，也不代表完成了硬件时序或功耗验证。
 */
#ifndef WINYUNQ_PAW3805EK_H
#define WINYUNQ_PAW3805EK_H

#include "PAW3805EKConfig.h"

/** @brief 驱动状态；未配置或未识别时不产生有效位移。 */
typedef enum
{
    PAW3805EK_UNCONFIGURED,  /**< 尚未确认 GPIO 连线，完全不访问 GPIO。 */
    PAW3805EK_STARTING,      /**< 等待 TMOS 探测或复位后复查。 */
    PAW3805EK_READY,         /**< 双产品 ID 匹配，可以采样。 */
    PAW3805EK_NOT_FOUND,     /**< 产品 ID 不匹配，有限重试已结束。 */
    PAW3805EK_POWERED_DOWN,  /**< 已发送有依据的传感器掉电命令。 */
    PAW3805EK_ERROR         /**< TMOS 注册或事件/定时器分配失败。 */
} PAW3805EK_State;

/**
 * @brief 初始化或重新探测传感器，不等待传感器就绪。
 * @details 必须在 BLE/TMOS 初始化后、协作式任务上下文调用。
 * 重复调用重用同一个 TMOS 任务。未配置时不注册任务、不操作 GPIO。
 */
void PAW3805EK_Init(void);

/** @brief 返回当前初始化/识别状态，不进行总线访问。 */
PAW3805EK_State PAW3805EK_GetState(void);

/**
 * @brief 最多获取一组 16 位原始位移，运行时间有界。
 * @param[out] dx X 轴二补码位移；无数据或失败时为 0。
 * @param[out] dy Y 轴二补码位移；无数据或失败时为 0。
 * @return 1 表示读取到一组有效数据，0 表示无数据、未就绪或参数无效。
 * @note 不应用轴旋转/翻转或 HID 截断；这些属于鼠标应用层。
 * 不可从中断调用，也不可与其他总线访问并发使用。
 */
unsigned char PAW3805EK_ReadMotion(signed short *dx, signed short *dy);

/**
 * @brief 已识别且就绪时请求传感器掉电，不控制电源 GPIO。
 * @return 1 表示命令已发出，0 表示当前状态不允许发送。
 * @note SPI 无应答位，返回值不是芯片已进入低功耗状态的测量证明。
 * 恢复采样时调用 PAW3805EK_Init() 重新识别/复位。
 */
unsigned char PAW3805EK_PowerDown(void);

#endif
