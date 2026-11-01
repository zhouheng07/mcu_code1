#ifndef __SPI_H
#define __SPI_H
#include "stm32f10x.h"

void SPI1_Init(void);
void SPI1_SendData(u8 dat);
void SPI1_SendBuffer(u8 *buf, u16 len);

// ========== ????? ==========
#define OLED_RST_PIN    GPIO_Pin_2  // RES PA2
#define OLED_DC_PIN     GPIO_Pin_1  // DC PA1 ??
#define OLED_CS_PIN     GPIO_Pin_0  // CS PA0

#define OLED_RST_H      GPIO_SetBits(GPIOA, OLED_RST_PIN)
#define OLED_RST_L      GPIO_ResetBits(GPIOA, OLED_RST_PIN)
#define OLED_DC_H       GPIO_SetBits(GPIOA, OLED_DC_PIN)
#define OLED_DC_L       GPIO_ResetBits(GPIOA, OLED_DC_PIN)
#define OLED_CS_H       GPIO_SetBits(GPIOA, OLED_CS_PIN)
#define OLED_CS_L       GPIO_ResetBits(GPIOA, OLED_CS_PIN)

#endif
