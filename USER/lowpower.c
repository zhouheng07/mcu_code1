/**************************************
 * 文件名  ：lowpower.c
 * 描述    ：基于 RTC（LSE）+ STOP 模式的低功耗框架
 *          每秒由 RTC 闹钟（EXTI17）唤醒一次，CPU 在两次唤醒之间
 *          进入 STOP 模式，功耗约 20–25uA（较全速运行降低约 1000 倍）。
 *
 * 实验平台：STM32F103C8T6
 * 硬件依赖：PC14/PC15 接 32.768kHz 晶振 + 两颗 12.5pF 负载电容
 *
 *********************************************************/

#include "lowpower.h"
#include "delay.h"

/* 初始化 RTC：LSE 时钟源，预分频 32768 → 计数器 1Hz 跳变；
 * 开启闹钟中断，经 EXTI Line17（上升沿）唤醒 STOP 模式。 */
void LowPower_Init(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 1. 使能 PWR / BKP 时钟并打开后备域写访问 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    /* 2. 复位后备域（每次上电从 0 秒开始，与原行为一致） */
    BKP_DeInit();

    /* 3. 开启 LSE 32.768kHz 外部晶振并等待就绪（典型起振 1–2s） */
    RCC_LSEConfig(RCC_LSE_ON);
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
    {
        ;  /* 此处为一次性等待，主循环不会进入忙等 */
    }

    /* 4. 选择 LSE 作为 RTC 时钟源并使能 RTC */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);

    /* 5. 等待 RTC 寄存器与 APB 同步、上次写操作完成 */
    RTC_WaitForSynchro();
    RTC_WaitForLastTask();

    /* 6. 预分频 32767 → 32768 / (32767+1) = 1Hz，计数器每秒 +1 */
    RTC_SetPrescaler(32767);
    RTC_WaitForLastTask();

    /* 7. 使能 RTC 闹钟中断 */
    RTC_ITConfig(RTC_IT_ALR, ENABLE);
    RTC_WaitForLastTask();

    /* 8. 配置 EXTI Line17（RTC Alarm）上升沿中断，用于唤醒 STOP */
    EXTI_ClearITPendingBit(EXTI_Line17);
    EXTI_InitStructure.EXTI_Line    = EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* 9. 使能 RTCAlarm 中断（NVIC） */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    NVIC_InitStructure.NVIC_IRQChannel                   = RTCAlarm_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 10. 计数器从 0 开始，设首次闹钟在第 1 秒 */
    RTC_SetAlarm(1);
    RTC_WaitForLastTask();
}

uint32_t LowPower_GetSeconds(void)
{
    return RTC_GetCounter();
}

void LowPower_EnterStopUntilNextSec(void)
{
    LowPower_EnterStopUntilSec(RTC_GetCounter() + 1);
}

void LowPower_EnterStopUntilSec(uint32_t target_sec)
{
    uint32_t now;
    volatile uint32_t wait_loop;

    now = RTC_GetCounter();
    if (target_sec <= now)
    {
        return;  /* 目标已过，不进入休眠 */
    }

    /* 提前清 EXTI Line17 挂起位（写 1 清），防止之前休眠的残留脉冲
     * 导致 WFI 一进去就立刻误唤醒、或反过来"挂起位一直低，闹钟到了却不醒"。
     * 此乃 STM32F1 文档 Errata 里 RTC Alarm/EXTI 同步 race 的规避。 */
    EXTI_ClearITPendingBit(EXTI_Line17);

    RTC_SetAlarm(target_sec);
    RTC_WaitForLastTask();

    /* 清 ALRF：必须在 SetAlarm 之后，且清完后再等一次 RTOFF 握手 */
    RTC_ClearITPendingBit(RTC_IT_ALR);
    RTC_WaitForLastTask();

    /* 读 RTC_CRL 的 ALRF 位：如果仍然是 1，说明 SetAlarm 时 RTC 计数器
     * 刚好跳变、比较器已在"过去的时间"触发了一次 ALRF。此时若直接进 STOP，
     * EXTI 上升沿已发过、不会重复发，CPU 会死在 STOP。保守做法：空跑几个
     * RTCCLK tick（~61µs/tick）后再清一次 ALRF + 再清 EXTI，
     * 再重读 now。若 now 已经到达 target_sec，则直接返回不睡。 */
    if (RTC_GetFlagStatus(RTC_FLAG_ALR) == SET)
    {
        for (wait_loop = 0; wait_loop < 6; wait_loop++)
        {
            /* 约等 1 个 RTCCLK（30.5µs）—— 死等 ~250 个 8MHz 周期 */
            volatile uint32_t j;
            for (j = 0; j < 250; j++) { __NOP(); }
        }
        RTC_ClearITPendingBit(RTC_IT_ALR);
        RTC_WaitForLastTask();
        EXTI_ClearITPendingBit(EXTI_Line17);
        now = RTC_GetCounter();
        if (now >= target_sec)
        {
            return;  /* 目标秒数已到，不睡了 */
        }
    }

    /* 再次确认 PR17 为 0，否则清掉 */
    if (EXTI_GetITStatus(EXTI_Line17) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line17);
    }

    /* 进入 STOP 模式（调压器低功耗）。唤醒源：RTC Alarm -> EXTI Line17。 */
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

    /* 唤醒返回：
     * 1. EXTI17/RTC 标志由 RTCAlarm_IRQHandler 清除。
     * 2. STOP 期间 Cortex-M3 硬件自动清零 SysTick->CTRL.ENABLE，
     *    导致 delay_ms_wfi/任何 SysTick 依赖功能失效。
     *    因此唤醒后第一时间重新初始化 SysTick。 */
    delay_init();
}
