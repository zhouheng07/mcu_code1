#ifndef __DELAY_H
#define __DELAY_H 			   
#include "sys.h"  

void delay_init(void);
void delay_ms(__IO uint32_t  nTime);

/* 低功耗版 delay_ms：用 WFI 等待 SysTick 中断唤醒。
 * 不占 CPU，电流由 ~5mA 降到 ~3mA（仍比 STOP 高，因为 HSE/HSI 不关）。
 * 用于加水时段 100ms 颗粒度的 LED1 闪烁。期间 SysTick 必须保持开启。 */
void delay_ms_wfi(uint32_t nTime);

#endif





























