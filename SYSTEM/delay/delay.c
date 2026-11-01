#include "delay.h"




//��ʼ���ӳٺ���
//��ʹ��OS��ʱ��,�˺������ʼ��OS��ʱ�ӽ���
//���õȴ�ʱ�䵥λΪ1ms
void delay_init(void)
{
	if(SysTick_Config(SystemCoreClock / 1000))//systick�ж����ã�1ms�����ж�һ�Σ�stm32f10x_it.c �ļ��е�SysTick_Handler�����е��ü���
  { 	
		
  }
}								    


static __IO uint32_t TimingDelay;
static __IO uint32_t TimingDelayWFI;
//1ms忙等
void delay_ms(__IO uint32_t  nTime)
{	 		  	  
  TimingDelay = nTime;
  while(TimingDelay != 0);
}

/* 低功耗版 1ms 延时：用 WFI 挂起 CPU，SysTick 中断唤醒后递减计数。
 * 期间 CPU 不做空循环，节省几 mA 动态功耗，用于加水时段 100ms 粒度快闪。 */
void delay_ms_wfi(uint32_t nTime)
{
    TimingDelayWFI = nTime;
    while (TimingDelayWFI != 0)
    {
        __WFI();
    }
}

//1ms递减一次（SysTick 中断内调用）
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
  if (TimingDelayWFI != 0x00)
  {
    TimingDelayWFI--;
  }
}













