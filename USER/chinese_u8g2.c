//开始加水/终止加水，一天一次

#include "u8g2.h"
#include "chinese_u8g2.h"
extern u8g2_t u8g2;
/****************?16*16****************/
// ?


// x,y:?????; font:??????
void U8G2_DrawChinese16(u8g2_t *u8g2, uint16_t x, uint16_t y, const uint8_t *font)
{
    uint8_t i,j;
    for(i = 0; i < 16; i++)
    {
        for(j = 0; j < 2; j++)
        {
            u8g2_DrawXBMP(u8g2, x + j*8, y + i, 8, 1, &font[i*2 + j]);
        }
    }
}

void OLED_RefreshPage(char buf[32])
{

    u8g2_ClearBuffer(&u8g2);

    // ============??? Y=0============
    if(water_state == WATER_START)
    {
        U8G2_DrawChinese16(&u8g2, 0, 0, font_kai);
        U8G2_DrawChinese16(&u8g2, 16,0, font_shi);
        U8G2_DrawChinese16(&u8g2, 32,0, font_jia);
        U8G2_DrawChinese16(&u8g2, 48,0, font_shui);
    }
    else
    {
        U8G2_DrawChinese16(&u8g2, 0, 0, font_zhong);
        U8G2_DrawChinese16(&u8g2, 16,0, font_zhi);
        U8G2_DrawChinese16(&u8g2, 32,0, font_jia);
        U8G2_DrawChinese16(&u8g2, 48,0, font_shui);
    }

    // ============??? Y=18============
    U8G2_DrawChinese16(&u8g2, 0, 18, font_yi);
    U8G2_DrawChinese16(&u8g2, 16,18, font_tian);
    U8G2_DrawChinese16(&u8g2, 32,18, font_yi);
    U8G2_DrawChinese16(&u8g2, 48,18, font_ci);

    // ============??? Y=40 ????============
    u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
		u8g2_DrawStr(&u8g2, 0, 24, buf);
    u8g2_SendBuffer(&u8g2);
}