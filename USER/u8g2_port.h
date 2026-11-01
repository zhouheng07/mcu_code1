#ifndef __U8G2_PORT_H
#define __U8G2_PORT_H
#include "stm32f10x.h"
#include "u8g2.h"
#include "spi.h"

extern u8g2_t u8g2;

uint8_t u8x8_stm32_spi_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_stm32_gpio_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
void OLED_U8G2_Init(void);

#endif