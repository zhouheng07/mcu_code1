#ifndef __LOWPOWER_H
#define __LOWPOWER_H

#include "stm32f10x.h"

/* 初始化 RTC（LSE 32.768kHz 晶振）+ EXTI17 闹钟唤醒 + NVIC。
 * 调用前请确保已执行 LED_GPIO_Config() / GPIO_LowPower_Config()。 */
void LowPower_Init(void);

/* 读取开机以来累计秒数（RTC 计数器，1Hz 跳变）。 */
uint32_t LowPower_GetSeconds(void);

/* 设置下一次闹钟为"当前秒 + 1"，并进入 STOP 模式（调压器低功耗）。
 * 由 RTC 闹钟经 EXTI17 唤醒；唤醒后系统时钟自动切到 HSI 8MHz。 */
void LowPower_EnterStopUntilNextSec(void);

/* 进入 STOP 模式直到 RTC 计数器 == target_sec（绝对秒数）。
 * target_sec 必须大于等于当前秒数，否则直接返回不休眠。
 * 用于停止加水时段一次跳过多秒（例如 s=1 醒来后直接睡到下一分 s=0）。 */
void LowPower_EnterStopUntilSec(uint32_t target_sec);

#endif /* __LOWPOWER_H */
