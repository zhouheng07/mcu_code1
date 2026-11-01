#ifndef __LED_H
#define	__LED_H

#include "stm32f10x.h"

/* the macro definition to trigger the led on or off
 * 1 - off
 - 0 - on
 */
#define ON  0
#define OFF 1

/* 低功耗优化后引脚分配：
 * LED1 -> PC13 (保持不变，注意 PC13 驱动能力 ≤3mA、速率 ≤2MHz)
 * LED2 -> PA15 (原 PC14 让给 LSE 32.768kHz 晶振；PA15 由禁用 JTAG 释放) */
#define LED1(a)	if (a)	\
					GPIO_SetBits(GPIOC,GPIO_Pin_13);\
					else		\
					GPIO_ResetBits(GPIOC,GPIO_Pin_13)

#define LED2(a)	if (a)	\
					GPIO_SetBits(GPIOA,GPIO_Pin_15);\
					else		\
					GPIO_ResetBits(GPIOA,GPIO_Pin_15)

/* 便捷翻转宏（主循环使用） */

/* LED1 翻转：读 ODR 取反写回，while 回读 ODR 直到值真正变了再退出。
 * 防止 STOP 唤醒 / APB 总线握手导致写操作漏过。 */
#define LED1_TOGGLE  do {                                                 \
    BitAction _before = (BitAction)GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13);  \
    BitAction _after  = (_before == Bit_SET) ? Bit_RESET : Bit_SET;             \
    do {                                                                       \
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, _after);                              \
    } while (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) != _after);             \
} while (0)

#define LED2_TOGGLE   GPIO_WriteBit(GPIOA, GPIO_Pin_15, \
                        (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_15)))

/* PA15 (LED2) 显式置位/清零 + 回读校验：
 *   SET_GPIOA15(1) → 拉到高电平（ODR.15 == 1），并 while 读 ODR 直到确认
 *   SET_GPIOA15(0) → 拉到低电平（ODR.15 == 0），并 while 读 ODR 直到确认
 * 非 0/1 调用默认按非零值当作 1 处理。 */
#define SET_GPIOA15(n)  do {                                                  \
    BitAction _v = ((n) == 0) ? Bit_RESET : Bit_SET;                          \
    do {                                                                       \
        if (_v == Bit_SET) GPIO_SetBits(GPIOA, GPIO_Pin_15);                   \
        else                GPIO_ResetBits(GPIOA, GPIO_Pin_15);                \
    } while (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_15) != _v);                \
} while (0)

void LED_GPIO_Config(void);
void GPIO_LowPower_Config(void);   /* 将所有未用引脚配为模拟输入，关 Schmitt 漏电 */

#endif /* __LED_H */
