/**************************************
 * 文件名  ：led.c
 * 描述    ：led 及低功耗 GPIO 配置
 *
 * 实验平台：MINI STM32 核心板，主控 STM32F103C8T6
 * 硬件连接：-----------------
 *          |  PC13 - LED1     |
 *          |  PA15 - LED2     |  (低功耗优化后由 PC14 改接，原 PC14 让给 LSE 晶振)
 *           -----------------
 * 版本    ：ST3.0.0

*********************************************************/

#include "led.h"

 /***************  LED 用 I/O 口初始化 *******************/
void LED_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能 GPIOC / GPIOA / AFIO 时钟（AFIO 用于 JTAG 重映射释放 PA15） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_AFIO, ENABLE);

    /* 仅禁用 JTAG，保留 SWD 供调试；释放 PA15 / PB3 / PB4 */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    /* LED1 -> PC13：推挽输出，2MHz（PC13 限速，且降低开关损耗） */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* LED2 -> PA15：推挽输出，2MHz */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 初始关闭两路 LED（高电平关，依硬件原理图） */
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
    GPIO_SetBits(GPIOA, GPIO_Pin_15);
}

/***************  未用引脚设为模拟输入以消除漏电 *******************/
/* 模拟输入模式下 Schmitt 触发器关闭，输入数字部分不耗电；
 * 复位默认为浮空输入会持续漏电，故显式配置为 AIN。 */
void GPIO_LowPower_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  /* AIN 下无效，仅占位 */

    /* GPIOA：除 PA15(LED2)、PA13(SWDIO)、PA14(SWCLK) 外全部模拟 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_2  |
                                  GPIO_Pin_3  | GPIO_Pin_4  | GPIO_Pin_5  |
                                  GPIO_Pin_6  | GPIO_Pin_7  | GPIO_Pin_8  |
                                  GPIO_Pin_9  | GPIO_Pin_10 | GPIO_Pin_11 |
                                  GPIO_Pin_12;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* GPIOB：全部未用（PB3/PB4 已由禁 JTAG 释放） */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* GPIOC：仅 PC13 输出；PC14/PC15 留给 LSE 晶振，保持复位默认不触碰 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_2  |
                                  GPIO_Pin_3  | GPIO_Pin_4  | GPIO_Pin_5  |
                                  GPIO_Pin_6  | GPIO_Pin_7  | GPIO_Pin_8  |
                                  GPIO_Pin_9  | GPIO_Pin_10 | GPIO_Pin_11 |
                                  GPIO_Pin_12;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}
