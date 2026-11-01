/*****************************************************************************
  文件: main.c
  描述: 24 小时喂鱼指示循环（低功耗版 + OLED 显示）。
        两阶段策略：
          1) 加水（第 1 分钟 0:0:0~0:0:59）：LED1 100ms 闪烁；
             OLED 每秒刷新常亮。CPU 进入 WFI（SLEEP 模式）实现低功耗快闪。
          2) 停止加水（其余时刻）：CPU 进入 STOP 模式。
             - 每分钟开头 s=0 与 s=1 醒 2 秒，刷新 OLED 显示时间+状态；
             - s=1 刷新完后 OLED 关屏 (SSD1306 省电)，直接 STOP 睡到下一分钟 s=0。
             - LED1 每 4 秒翻转一次（s=0,4,8,12,16...）。
*******************************************************************************/
#include "delay.h"
#include "sys.h"
#include "led.h"
#include "stm32f10x.h"
#include "lowpower.h"
#include "u8g2_port.h"
#include <stdio.h>

/* SWD 下载窗口时长（ms）。进入 STOP 前保持 CPU 正常运行这么久，
 * 供 Keil 用 Normal 方式连接下载，避免 internal command error。
 * 下载方法：按复位键后立即点 Download。该窗口仅在每次上电执行一次，
 * 对电池续航影响可忽略。设为 0 则关闭窗口（量产可选，但下载需改用 under Reset）。 */
#define SWD_WINDOW_MS  5000


int main(void)
{
	uint32_t sec, s, m, h;
	char buf[32];

	/* 系统时钟已由启动文件调用 SystemInit() 配为 HSE 8MHz 直供（无 PLL）。
	 * 此处冗余调用无害，保留以兼容旧代码习惯。 */
	SystemInit();

	/* delay_init 初始化 SysTick 1ms 中断，仅供初始化阶段使用；
	 * 主循环不再依赖 delay_ms，时基由 RTC 提供。 */
	delay_init();

	/* LED 引脚配置 + 未用引脚设为模拟输入（关 Schmitt 漏电） */
	LED_GPIO_Config();
	GPIO_LowPower_Config();

	/* 初始化 RTC（LSE）+ EXTI17 闹钟唤醒 + NVIC。
	 * LSE 起振约 1–2s，函数内阻塞等待就绪。 */
	LowPower_Init();

	//OLEDÏÔÊ¾Ê±¼äºÍ×´Ì¬
	SPI1_Init();
	OLED_U8G2_Init();
	u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

	/* 信号灯：上电瞬间翻转 LED1，指示初始化完成 */
	LED1_TOGGLE;


	/* OLED 关屏状态标志，避免频繁发 SetPowerSave(1) 命令 */
	uint8_t oled_powered_down = 0;

	while (1)
	{
		uint8_t step;

		/* 秒数计算直接硬编码：
		 *   每分钟 10 秒（调试用，量产改回 60）
		 *   每周期 3 分钟（调试用，量产改回 1440 = 24h）
		 * 不使用宏替换，所有常量写死。 */
		sec = LowPower_GetSeconds();

		s = sec % 60;                /* 本分钟秒 0~9 */
		m = (sec / 60) % 60;     /* 本周期分钟 0,1,2 */
		h = (sec / 3600) % 24;

		if (h == 0 && m == 0)
		{
			/* =======================================================
			 * 阶段 A：加水（m=0, s=0~9）
			 * ======================================================= */

			/* s=0：开始加水（PA15=1） */
			if (s == 0)
			{
				SystemInit();
				SET_GPIOA15(1);
				
			}

			/* 确保 OLED 开屏 */
			if (oled_powered_down)
			{
				u8g2_SetPowerSave(&u8g2, 0);
				oled_powered_down = 0;
			}

			/* 秒级刷新：时间 + 状态 */
			u8g2_ClearBuffer(&u8g2);
			sprintf(buf, "Time:%02d:%02d:%02d", h, m, s);
			u8g2_DrawStr(&u8g2, 0, 14, buf);
			u8g2_DrawStr(&u8g2, 0, 40, "Adding water...");
			u8g2_DrawStr(&u8g2, 0, 56, "<<Once per cycle>>");
			u8g2_SendBuffer(&u8g2);

			/* LED1 100ms 闪烁 */		
			for (step = 0; step < 10; step++)
			{
				LED1_TOGGLE;
				delay_ms(100);
			}
			
		}
		else
		{
			/* =======================================================
			 * 阶段 B：停止加水（m=1,2）
			 *   s=0：开屏，刷新显示 + (m==1时关LED2) + LED1翻转 → 睡到 s=1
			 *   s=1：刷新显示 → 睡到 s=2   （合计亮 2 秒）
			 *   s=2：关屏；若 s 是 2 的倍数，翻 LED1
			 *   其它：按 2 秒倍数或下一分钟 s=0 跳转 STOP
			 * 重要：STOP 目标必须用"原始秒值"计算，
			 *      不能用取模后的 sec（会指向过去的时刻 → 不睡眠死循环）。
			 * ======================================================= */

			if (s == 0)
			{
				if (oled_powered_down)
				{
					u8g2_SetPowerSave(&u8g2, 0);
					oled_powered_down = 0;
				}

				u8g2_ClearBuffer(&u8g2);
				sprintf(buf, "Time:%02d:%02d:%02d", h, m, s);
				u8g2_DrawStr(&u8g2, 0, 14, buf);
				u8g2_DrawStr(&u8g2, 0, 40, "Stop add water");
				u8g2_DrawStr(&u8g2, 0, 56, "<<Once per cycle>>");
				u8g2_SendBuffer(&u8g2);

				/* m=1 停止加水 → 关 LED2 */
				if (h == 0 && m == 1)
				{
					SET_GPIOA15(0);
				}
				/* s=0 → 2 的倍数 → 翻 LED1 */
				LED1_TOGGLE;

				LowPower_EnterStopUntilSec(sec + 1);   /* 用原始值 +1 */
			}
			else if (s == 1)
			{
				u8g2_ClearBuffer(&u8g2);
				sprintf(buf, "Time:%02d:%02d:%02d", h, m, s);
				u8g2_DrawStr(&u8g2, 0, 14, buf);
				u8g2_DrawStr(&u8g2, 0, 40, "Stop add water");
				u8g2_DrawStr(&u8g2, 0, 56, "<<Once sdfadsf cycle>>");
				u8g2_SendBuffer(&u8g2);

				LowPower_EnterStopUntilSec(sec + 1);   /* 用原始值 +1 */
			}
			else
			{

				/* s=2：关屏 */
				if (s == 2 && !oled_powered_down)
				{
					u8g2_SetPowerSave(&u8g2, 1);
					oled_powered_down = 1;
				}

				/* 2 倍数翻 LED1（s=2,4,6,8；s=0 已在上方 s==0 分支处理） */
				if ((s % 2) == 0)
				{
					LED1_TOGGLE;
				}

				/* STOP 目标用原始秒重新读取，
				 * 目标 = 下一个 2 秒倍数 或 下一分钟 s=0（取更早者）。 */
				LowPower_EnterStopUntilSec(sec + 1);   /* 用原始值 +1 */
			}
		}
	}
}
