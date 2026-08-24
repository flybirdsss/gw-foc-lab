# STM32G474RET6 CMSIS-DAP Debug异常与SystemClock_Config恢复总结

日期：2026-08-20

## 1. 问题现象

Keil Debug时报错：

    CMSIS-DAP Cortex-M Error
    Could not stop Cortex-M device!

现象： - 无法进入Debug - Step Over不可用 - DAPLink无法halt MCU

## 2. 排查过程

OpenOCD测试：

    openocd.exe -f interface/cmsis-dap.cfg -f target/stm32g4x.cfg -c "init; reset halt"

结果：

    Cortex-M4 processor detected
    Examination succeed
    halted due to debug-request

说明： - DAPLink正常 - SWD通信正常 - MCU正常响应 - 芯片没有损坏

## 3. 芯片型号确认

实际芯片：

    STM32G474RET6

Flash：

    512KB

Keil Flash Algorithm：

    STM32G47x-8x 512 KB Flash
    08000000H - 0807FFFFH

## 4. SystemClock_Config错误原因

原测试代码存在配置冲突：

### PLL关闭但使用PLL时钟

    PLL OFF
    SYSCLK = PLLCLK

矛盾：

    PLL关闭
    ↓
    选择PLL作为系统时钟
    ↓
    时钟配置异常

### 打开HSI但选择HSE

配置：

    HSI ON

但是：

    SYSCLK = HSE

导致时钟源不匹配。

### 混合配置

同时修改： - HSI - HSE - HSI48 - PLL ON/OFF

导致RCC状态不确定。

## 5. 恢复方案

恢复到已经验证正常的：

    HSI + PLL

配置：

    HSI = 16MHz

    16MHz / 4 = 4MHz

    4MHz × 85 = 340MHz

    340MHz / 2 = 170MHz SYSCLK

核心：

    HSI ON
    PLL ON
    PLL Source = HSI
    SYSCLK Source = PLL

## 6. OpenOCD恢复流程

当错误时钟程序导致Keil无法连接：

连接：

    reset halt

擦除Flash：

    stm32g4x mass_erase 0

返回：

    stm32g4x mass erase complete

说明Flash恢复。

## 7. 重新下载

恢复正确SystemClock_Config：

    Clean Targets
    Rebuild
    Download

下载成功后Debug恢复。

## 8. Keil最终配置

Debug：

    Connect:
    Normal

    Max Clock:
    500kHz~1MHz

    Cache Code:
    Enable

    Cache Memory:
    Enable

Flash Download：

    STM32G47x-8x 512 KB Flash

    Start:
    0x08000000

    Size:
    512KB

## 9. 根因总结

不是：

-   DAPLink故障
-   SWD线路问题
-   STM32损坏
-   Flash损坏

真正原因：

    错误SystemClock_Config
            ↓
    RCC配置冲突
            ↓
    MCU启动状态异常
            ↓
    Keil无法halt Cortex-M
            ↓
    CMSIS-DAP报错

## 10. 经验总结

STM32时钟测试必须保持单一配置：

HSI + PLL：

    HSI ON
    PLL ON
    SYSCLK = PLL

HSE直跑：

    HSE ON
    PLL OFF
    SYSCLK = HSE

HSE + PLL：

    HSE ON
    PLL ON
    SYSCLK = PLL

不要混合修改。

## 11. 标准恢复流程

1.  OpenOCD：

```{=html}
<!-- -->
```
    reset halt

2.  擦除：

```{=html}
<!-- -->
```
    stm32g4x mass_erase 0

3.  恢复正确时钟配置

4.  重新下载

## 最终状态

-   STM32G474RET6 ✅
-   DAPLink ✅
-   Keil Debug ✅
-   Flash Algorithm 512KB ✅
-   HSI+PLL 170MHz ✅
