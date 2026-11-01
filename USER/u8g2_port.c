#include "u8g2_port.h"
#include "delay.h"

u8g2_t u8g2;

// SPI??????
uint8_t u8x8_stm32_spi_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    uint8_t *pBuf;
    switch(msg)
    {
        case U8X8_MSG_BYTE_SEND:
            pBuf = arg_ptr;
            SPI1_SendBuffer(pBuf, arg_int);
            break;
        case U8X8_MSG_BYTE_SET_DC:
            if(arg_int) OLED_DC_H;
            else        OLED_DC_L;
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            OLED_CS_L;
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            OLED_CS_H;
            break;
        default:
            return 0;
    }
    return 1;
}

// ??+????
uint8_t u8x8_stm32_gpio_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch(msg)
    {
        case U8X8_MSG_GPIO_RESET:
            if(arg_int) OLED_RST_H;
            else        OLED_RST_L;
            break;
        case U8X8_MSG_DELAY_MILLI:
            delay_ms(arg_int);
            break;

        // ===== ??????????/????case,???? =====

        default:
            return 0;
    }
    return 1;
}

// OLED U8G2?????
void OLED_U8G2_Init(void)
{
    u8g2_Setup_ssd1306_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8x8_stm32_spi_cb,
        u8x8_stm32_gpio_delay_cb
    );
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0); // ????
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
}