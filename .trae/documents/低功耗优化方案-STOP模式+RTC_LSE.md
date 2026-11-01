# 低功耗优化方案：STOP 模式 + RTC（LSE）唤醒

## Context（背景）

当前 [main.c](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/main.c) 实现了一个 24 小时喂鱼指示循环：每秒用 `delay_ms(1000)` 忙等，在特定时刻翻转 PC13/PC14。问题在于：

- **忙等耗电**：`delay_ms` 内部是 `while(TimingDelay != 0)`（[delay.c:24](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/SYSTEM/delay/delay.c#L24)），CPU 每秒约 999ms 全速空转。
- **主频偏高**：[system_stm32f10x.c](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/system_stm32f10x.c) 启用 `SYSCLK_FREQ_24MHz`，PLL 常开。
- **外设时钟未关**：`RCC_Configuration()` 开了 TIM1 时钟但从未使用（[main.c:16-20](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/main.c#L16-L20)）；GPIO 配置为 50MHz 推挽（LED 应用过驱动）。
- **未用引脚悬空**：所有未用 GPIO 保持复位默认状态，Schmitt 触发器漏电。

预期效果：运行功耗约 30mA → STOP 模式约 20–25µA（降低约 1000 倍）。喂鱼走时由 LSE 32.768kHz 晶振保证（每天约 2 秒误差）。

## 硬件改动（用户已确认）

1. **PC14/PC15 焊 32.768kHz 晶振** + 两颗 12.5pF 负载电容（接 GND）。
2. **LED2 从 PC14 改接到 PA15**（PA15 由禁用 JTAG 释放，无需上拉）。限流电阻沿用原 LED2 的阻值。
3. PC13 上 LED1 保持不变（注意 PC13 驱动能力 ≤3mA、速率 ≤2MHz，需确认限流电阻 ≥1.5kΩ）。

## 软件改动

### 1. [USER/system_stm32f10x.c](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/system_stm32f10x.c) — 主频降为 HSE 8MHz 直供

- 注释掉 `#define SYSCLK_FREQ_24MHz 24000000`，启用 `#define SYSCLK_FREQ_HSE HSE_VALUE`。
- 理由：禁 PLL 省电；STOP 唤醒后系统自动切到 HSI 8MHz，与 HSE 8MHz 数量级一致，`SystemCoreClock` 变量无需刷新，`delay_ms` 在初始化阶段仍近似准确。

### 2. [USER/led.h](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/led.h) — LED2 改宏

- 新增 `LED2(a)` 宏指向 `GPIOA, GPIO_Pin_15`；保留 `LED1(a)` 指向 `GPIOC, GPIO_Pin_13`。
- 增加 `void GPIO_LowPower_Config(void);` 声明。

### 3. [USER/led.c](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/led.c) — 引脚重排 + 未用引脚设为模拟输入

`LED_GPIO_Config()` 改为：
- 开 GPIOC、GPIOA、AFIO 时钟。
- `GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE)` —— 禁 JTAG 释放 PA15/PB3/PB4，**保留 SWD** 供调试。
- PC13：推挽输出，`GPIO_Speed_2MHz`（PC13 限速）。
- PA15：推挽输出，`GPIO_Speed_2MHz`。
- 两路初始电平按"关闭"设置（依硬件决定高/低）。

新增 `GPIO_LowPower_Config()`：将所有未用引脚配为 `GPIO_Mode_AIN`（模拟输入，关 Schmitt 触发器，零漏电）：
- GPIOA：PA0–PA12、PA15 之外的全设模拟（PA13/PA14 保留 SWD 不动）。
- GPIOB：PB0–PB15 全部模拟（含刚释放的 PB3/PB4）。
- GPIOC：仅 PC13 输出；PC14/PC15 留给 LSE 晶振，**不要触碰**（保持复位默认）。

### 4. 新建 `USER/lowpower.h` / `USER/lowpower.c` — RTC+STOP 封装

`lowpower.h` 暴露：
```c
void LowPower_Init(void);          // RTC(LSE)+EXTI17+NVIC 初始化
void LowPower_EnterStopUntilNextSec(void);  // 设下一次闹钟并进 STOP
uint32_t LowPower_GetSeconds(void);         // 读 RTC 计数（=开机以来秒数）
```

`lowpower.c` 关键实现：
- `LowPower_Init()`：
  1. `RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE)`
  2. `PWR_BackupAccessCmd(ENABLE)` → `BKP_DeInit()`
  3. `RCC_LSEConfig(RCC_LSE_ON)` → 等待 `RCC_GetFlagStatus(RCC_FLAG_LSERDY)`（LSE 起振约 1–2s）
  4. `RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE)` → `RCC_RTCCLKCmd(ENABLE)`
  5. `RTC_WaitForSynchro()` → `RTC_WaitForLastTask()`
  6. `RTC_SetPrescaler(32767)` → `RTC_WaitForLastTask()`（32768Hz÷32768=1Hz，计数器每秒 +1）
  7. `RTC_ITConfig(RTC_IT_ALR, ENABLE)` → `RTC_WaitForLastTask()`
  8. EXTI17 配置：`EXTI_InitTypeDef`，line=17，mode=Interrupt，trigger=Rising。
  9. NVIC：`RTCAlarm_IRQn`，先占0/子优先0，enable。
  10. 首次闹钟：`RTC_SetAlarm(1)` → `RTC_WaitForLastTask()`。
- `LowPower_EnterStopUntilNextSec()`：
  ```c
  uint32_t now = RTC_GetCounter();
  RTC_SetAlarm(now + 1);
  RTC_WaitForLastTask();
  RTC_ClearFlag(RTC_FLAG_ALR);
  PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
  // 唤醒后：HSI 8MHz 自动接管，无需重配 PLL
  ```

### 5. [USER/stm32f10x_it.c](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/stm32f10x_it.c) — 增 RTC 闹钟 ISR

新增 `RTCAlarm_IRQHandler`：
```c
void RTCAlarm_IRQHandler(void)
{
    if(RTC_GetITStatus(RTC_IT_ALR) != RESET)
    {
        RTC_ClearITPendingBit(RTC_IT_ALR);
        EXTI_ClearITPendingBit(EXTI_Line17);
    }
}
```

### 6. [USER/main.c](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/main.c) — 主循环改写

- 删除 `RCC_Configuration()`（TIM1 死代码）。
- 初始化序列：`SystemInit()`（已由启动文件调用，可保留冗余调用无害）→ `delay_init()` → `LED_GPIO_Config()` → `GPIO_LowPower_Config()` → `LowPower_Init()`。
- 主循环用 `RTC_GetCounter()` 取秒数，按原 24h 逻辑翻转 LED1/LED2，再 `LowPower_EnterStopUntilNextSec()`：
  ```c
  while(1)
  {
      uint32_t sec = LowPower_GetSeconds();
      uint32_t s = sec % 60;
      uint32_t m = (sec / 60) % 60;
      uint32_t h = (sec / 3600) % 24;

      if(h == 0 && m == 0) {
          LED1_TOGGLE;            // 每秒
          if(s == 0) LED2_TOGGLE; // 0:0:0 翻转
      } else {
          if(h == 0 && m == 1 && s == 0) LED2_TOGGLE;
          if(s % 2 == 0) LED1_TOGGLE;   // 偶数秒翻转
      }
      LowPower_EnterStopUntilNextSec();
  }
  ```
  （行为与原 [main.c:40-61](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/main.c#L40-L61) 完全一致；`sec` 自然在 86400 处绕回，对应原 `for` 三层循环重启。）

### 7. [USER/F103.uvprojx](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/F103.uvprojx) — 把 lowpower.c 加入 USER 组

在 USER 组的 `<Files>` 内，仿照 `led.c` 条目插入：
```xml
<File>
  <FileName>lowpower.c</FileName>
  <FileType>1</FileType>
  <FilePath>.\lowpower.c</FilePath>
</File>
```

### 不改动的部分

- OLED/u8g2 源码保留在工程中但 main.c 不调用（用户已确认不需要 OLED）。SPI1/GPIOA 不初始化，配合 `GPIO_LowPower_Config()` 关漏电。
- [Time_test.c](file:///e:/单片机/b02434STM32F103C8T6开发板例程源代码/标准库代码/1.GPIO输出驱动LED_u8g2_屏幕显示%20-%20自动喂鱼%20-%20简化%20-%20简化/USER/Time_test.c) TIM2 不再被调用，保留不动。
- `delay.c` 保留（初始化阶段仍可用），主循环不再依赖。

## 关键风险与对策

| 风险 | 对策 |
|------|------|
| LSE 起振慢/不起振 | `LowPower_Init` 内 `RCC_GetFlagStatus(RCC_FLAG_LSERDY)` 轮询等待；若超时可加 fallback 到 LSI（本次未实现，先按 LSE）。 |
| PC13 驱动能力弱 | 限速 `GPIO_Speed_2MHz`，限流电阻 ≥1.5kΩ；硬件确认。 |
| 唤醒后时钟切 HSI | 已选 HSE 8MHz 直供，HSI 8MHz 数量级一致，`delay_ms`/`SystemCoreClock` 误差可接受；主循环不依赖 CPU 时钟精度。 |
| JTAG 禁用后无法用 J-Link 调试 | 仅禁 JTAG，**保留 SWD**（PA13/PA14）；可用 SWD 调试。 |
| BKP 域每次上电复位 | 应用本身每次上电从 0 秒开始，与原行为一致；如需断电续时可后续用 BKP 寄存器存秒数（本次不做）。 |

## 验证方法

1. **编译**：Keil 打开 `USER/F103.uvprojx`，确认 lowpower.c 已纳入编译，无错误/警告。
2. **功能验证**（先在开发板按硬件改动接好晶振+LED2）：
   - 烧录后用万用表/示波器观察 PC13：上电第 0 秒应翻转，之后偶数秒翻转；0:1:0 应观察到 PA15 翻转。
   - 24 小时绕回：可临时把 `sec % 86400` 改 `sec % 60` 加速验证，确认绕回逻辑正确后还原。
3. **功耗验证**：
   - 万用表 µA 档串入 3.3V 供电：STOP 期间应测得 20–30µA（含 LSE + 调压器低功耗 + LED 漏电）。
   - 唤醒瞬间用示波器看 PC13 翻转边沿，确认每秒一次。
4. **走时精度**：跑 24 小时对比标准时间，误差应在数秒内（LSE 精度取决于晶振与负载电容匹配）。
