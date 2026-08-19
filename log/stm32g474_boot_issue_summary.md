# STM32G474RET6 启动异常排查与修复总结

> 项目：GW-FOC-Lab  
> 日期：2026-08-19  
> 目的：记录本次 STM32G474RET6 无法正常进入 `main()`、误进入 System Memory Bootloader 的原因、排查过程、修复方法与 OpenOCD 指令。

## 1. 故障现象

在 Keil + DAPLink 调试时：

- `main()` 断点无法正常命中；
- CPU 的 PC 不在用户 Flash 区域；
- Disassembly / Show Next Statement 显示程序运行在：

```text
0x1FFFxxxx
```

正常用户程序应从：

```text
0x08000000
```

附近启动。

因此判断：MCU 没有从 Main Flash 启动，而是进入了 STM32 内部的 System Memory Bootloader。

## 2. 同时发现的工程型号问题

板上实际 MCU：

```text
STM32G474RET6
```

最初 CubeMX / Keil 工程按：

```text
STM32G474RBT6
```

建立。

主要区别：

```text
RBT6：128 KB Flash
RET6：512 KB Flash
```

因此重新迁移到 STM32G474RET6 工程，并确认：

```text
Keil Device：STM32G474RETx

IROM1 Start = 0x08000000
IROM1 Size  = 0x00080000
```

其中：

```text
0x80000 = 512 KB
```

Flash Download Algorithm 也改为：

```text
STM32G47x-8x 512 KB Flash
0x08000000 ~ 0x0807FFFF
```

这个问题必须修复，但它不是 CPU 进入 `0x1FFFxxxx` 的直接原因。

## 3. 真正导致无法进入 main() 的原因

原理图中 MCU 的 PB8 同时具有：

```text
PB8 / BOOT0
```

并连接为：

```text
CAN1_RXD → PB8 / BOOT0
```

也就是说，PB8 同时承担：

1. MCU 启动时的 BOOT0 功能；
2. 运行后的 FDCAN1_RX 功能。

当时读取到：

```text
FLASH->OPTR = 0xFFEFF8AA
```

其中：

```text
nSWBOOT0 = 1
```

这意味着复位时 BOOT0 使用 PB8 引脚的实际电平。

因此可能形成以下故障链：

```text
CAN 收发器 RXD 空闲输出高电平
            ↓
PB8 / BOOT0 = 1
            ↓
MCU 复位时读取 BOOT0
            ↓
选择 System Memory
            ↓
进入 STM32 ROM Bootloader
            ↓
PC = 0x1FFFxxxx
            ↓
main() 根本没有执行
```

## 4. 正确的解决目标

目标是：

```text
nSWBOOT0 = 0
nBOOT0   = 1
```

含义：

```text
nSWBOOT0 = 0
→ BOOT0 不再读取 PB8 的实际电平

nBOOT0 = 1
→ 由 Option Byte 固定选择 Main Flash
```

修复后的启动过程：

```text
MCU 复位
   ↓
忽略 PB8 / BOOT0 的物理电平
   ↓
读取 Option Byte
   ↓
从 Main Flash 启动
   ↓
PC = 0x080xxxxx
   ↓
进入 main()
```

## 5. 本次实际修复过程

### 5.1 临时从用户 Flash 启动

因为 MCU 当时已经进入 `0x1FFFxxxx` Bootloader，使用 Keil Command Window 手动把执行环境切换到用户 Flash。

设置 MSP：

```text
R13 = _RDWORD(0x08000000)
```

设置向量表：

```text
_WDWORD(0xE000ED08, 0x08000000)
```

设置 PC：

```text
$ = _RDWORD(0x08000004) & 0xFFFFFFFE
```

作用分别是：

```text
0x08000000 → 用户 Flash 向量表第 0 项：初始栈顶
0x08000004 → 用户 Flash 向量表第 1 项：Reset_Handler
0xE000ED08 → SCB->VTOR
```

之后成功进入：

```text
Reset_Handler
→ SystemInit
→ main()
```

## 6. Fix_Boot_OptionBytes() 的作用

之后通过 HAL 尝试修改 Option Bytes。

核心配置：

```c
FLASH_OBProgramInitTypeDef ob = {0};

HAL_FLASH_Unlock();
HAL_FLASH_OB_Unlock();

ob.OptionType = OPTIONBYTE_USER;

ob.USERType =
    OB_USER_nSWBOOT0 |
    OB_USER_nBOOT0;

ob.USERConfig =
    OB_BOOT0_FROM_OB |
    OB_nBOOT0_SET;

HAL_FLASHEx_OBProgram(&ob);
```

目标就是：

```text
nSWBOOT0 = 0
nBOOT0   = 1
```

虽然当时 `HAL_FLASHEx_OBProgram()` 后续返回了错误并进入 `Error_Handler()`，但之后读取 Option Byte 发现目标位已经被成功修改。

因此目前能确认的是：

> `Fix_Boot_OptionBytes()` 的执行过程中实际上已经改变了目标 Option Byte；但 HAL 为什么最终返回错误，当时没有继续读取 `HAL_FLASH_GetError()`，所以原因没有进一步确认。

## 7. DAPLink + OpenOCD 验证

### 7.1 OpenOCD 环境确认

```bat
openocd.exe --version
```

### 7.2 读取 FLASH_OPTR

最终使用：

```bat
openocd.exe -s ..\share\openocd\scripts -f interface\cmsis-dap.cfg -c "transport select swd" -f target\stm32g4x.cfg -c "init" -c "reset halt" -c "stm32g4x.cpu mdw 0x40022020 1" -c "shutdown"
```

其中：

```text
0x40022020 = FLASH_OPTR
```

OpenOCD 实际读取到：

```text
0x40022020: fbeff8aa
```

即：

```text
OPTR = 0xFBEFF8AA
```

原来：

```text
0xFFEFF8AA
```

后来：

```text
0xFBEFF8AA
```

关键变化：

```text
bit26：1 → 0
nSWBOOT0：1 → 0
```

这证明启动配置已经被修改。

## 8. 为什么断电后问题真正消失

Option Byte 被编程后，还需要重新加载才能真正影响启动。

后来板子经历了断电重新上电：

```text
Option Byte 已写入
        ↓
Power Reset
        ↓
Option Bytes 重新加载
        ↓
nSWBOOT0 = 0 生效
        ↓
PB8 不再作为 BOOT0 输入
        ↓
MCU 正常从 Main Flash 启动
```

之后 Keil 调试可以正常进入 `main()`。

## 9. OpenOCD 能否直接修复

可以。

OpenOCD 可以直接修改 STM32G4 Option Bytes。

如果以后遇到相同问题，目标仍然是：

```text
nSWBOOT0 = 0
nBOOT0   = 1
```

对应：

```text
value = 0x08000000
mask  = 0x0C000000
```

完整命令：

```bat
openocd.exe -s ..\share\openocd\scripts -f interface\cmsis-dap.cfg -c "transport select swd" -f target\stm32g4x.cfg -c "init; reset halt; stm32l4x option_write 0 0x20 0x08000000 0x0C000000; stm32l4x option_load 0; shutdown"
```

其中：

```text
mask = 0x0C000000
```

只修改：

```text
bit27 = nBOOT0
bit26 = nSWBOOT0
```

而：

```text
value = 0x08000000
```

表示：

```text
bit27 = 1
bit26 = 0
```

注意：

> 当前板子的 OPTR 已经是正确的 `0xFBEFF8AA`，不要再次执行 `option_write`。该命令仅作为以后故障恢复记录。

## 10. 最终故障链总结

### 问题 1：工程型号错误

```text
实际 MCU：STM32G474RET6
旧工程： STM32G474RBT6
```

导致：

```text
Flash Size / Device / Flash Algorithm 不匹配
```

处理：

```text
迁移到 STM32G474RET6
→ IROM1 = 0x08000000 / 0x80000
→ 512 KB Flash Algorithm
```

### 问题 2：真正导致无法进入 main()

```text
PB8 同时连接：
BOOT0 + CAN1_RXD
        +
nSWBOOT0 = 1
        ↓
复位时读取 PB8
        ↓
PB8 被 CAN_RX 拉高
        ↓
进入 System Memory Bootloader
        ↓
PC = 0x1FFFxxxx
```

处理：

```text
nSWBOOT0 = 0
nBOOT0   = 1
        ↓
忽略 PB8 的启动电平
        ↓
固定从 Main Flash 启动
        ↓
PC = 0x080xxxxx
        ↓
main() 正常执行
```

## 11. 本次最重要的学习点

1. `0x080xxxxx` 通常代表用户 Main Flash 程序区域。
2. `0x1FFFxxxx` 是重要调试线索，通常说明 MCU 进入 System Memory / ROM Bootloader。
3. CubeMX 配置正确并不代表 MCU 一定能进入 `main()`，启动配置发生在应用代码执行之前。
4. BOOT0 与普通 GPIO / 外设引脚复用时，要特别注意复位瞬间的引脚电平。
5. Option Bytes 属于 MCU 非易失配置，不等同于普通 GPIO 配置。
6. DAPLink 不只能配合 Keil，也可以配合 OpenOCD 做更底层的 Flash / Option Byte 调试。
7. 调试时应先判断“CPU 到底跑在哪里”，再判断 SPI、ADC、RTOS 等应用层代码。
8. OpenOCD 可以作为 DAPLink 用户非常有价值的底层调试工具。

## 12. 当前状态

目前已确认：

```text
MCU       = STM32G474RET6
Flash     = 512 KB
OPTR      = 0xFBEFF8AA
nSWBOOT0  = 0
nBOOT0    = 1
启动地址   = Main Flash
main()    = 可正常进入
```

因此启动问题已经解决，可以继续：

```text
DRV8323 SPI 自检
→ 电流 ADC 验证
→ PWM 验证
→ 开环电机测试
→ 后续 FOC
```
