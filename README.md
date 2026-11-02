# LHA7220 24-Bit ADC Firmware & Demo Suite

基于 **STM32H750** 微控制器与 **LHA7220**（24位低功耗、高精度 Delta-Sigma ADC，兼容 ADS1220 指令集）的完整固件驱动与测试例程包。

---

## 目录
- [功能特性](#功能特性)
- [硬件连接](#硬件连接)
- [例程架构](#例程架构)
- [动态指令通道切换 (LHA7220-CMD)](#动态指令通道切换-lha7220-cmd)
- [开发环境与编译](#开发环境与编译)

---

## 功能特性

1. **高精度 24-Bit 采集**：支持内部 2.048V 参考与外部基准源，提供完整的补码符号扩展与电压物理量转换。
2. **丰富的测试用例集**：
   - 例程 1：基础轮询连续差分采集 (`example-polling`)
   - 例程 2：DRDY 硬件外部中断采集 (`example-interrupt`)
   - 例程 3：芯片内部温度传感器测量 (`example-temperature`)
   - 例程 4：多通道循环切换采样 (`example-multichannel`)
   - 例程 5：单次触发与低功耗待机模式 (`example-single-shot`)
   - 例程 6：寄存器读写校验与通信自检 (`example-reg-test`)
   - 例程 7：**动态指令解析与通道切换测试 (`LHA7220-CMD`)**
3. **动态指令解析引擎**：
   - 支持通过串口（USART2, 1.5 Mbps）直接下发十六进制原生指令流动态控制芯片。
   - 自动解析复位（`0x06`）、写寄存器（`0x40~0x4F`）、启动转换（`0x08`）等。
   - 实现**不间断连续转换**，直到收到新指令立即重新配置并无缝切换通道。

---

## 硬件连接

| 芯片信号 (LHA7220) | MCU 引脚 (STM32H750) | 功能说明 |
| :--- | :--- | :--- |
| **SCLK** | `PA5` (SPI1_SCK) | SPI 串行时钟 (Mode 1: CPOL=0, CPHA=1) |
| **DIN (MOSI)** | `PA7` (SPI1_MOSI) | MCU 写入命令/配置数据 |
| **DOUT (MISO)** | `PA6` (SPI1_MISO) | MCU 读取 24 位 ADC 转换数据 |
| **CSN** | `PA4` (GPIO Output) | 片选引脚 (低电平有效) |
| **DRDYN** | `PA1` (EXTI1 / GPIO Input) | 数据准备就绪引脚 (低电平有效，下降沿指示就绪) |
| **TX / RX** | `PA2 / PA3` (USART2) | 调试打印与外部指令下发 (波特率: `1500000`) |

---

## 动态指令通道切换 (LHA7220-CMD)

在 `LHA7220-CMD` 测试用例中，寄存器配置完全由指令流动态驱动，无需硬编码。

### 1. 通道 1 配置指令流 (AIN0-AIN1 差分, 内部 2.048V 参考, 45 SPS 连续)
```text
06 40 06 44 24 48 10 4C 00 08
```
- `0x06`: 软复位并自动延时 300us
- `0x40, 0x06`: 配置 CONFIG0 -> 通道 AIN0-AIN1 差分, PGA 增益 Gain=8
- `0x44, 0x24`: 配置 CONFIG1 -> 45 SPS, Normal Mode, 连续转换模式 (CM=1)
- `0x48, 0x10`: 配置 CONFIG2 -> 内部 2.048V 参考, 50Hz/60Hz 工频陷波抑制
- `0x4C, 0x00`: 配置 CONFIG3 -> IDAC 关闭, DRDYM=0 (专用 DRDYN PA1 引脚)
- `0x08`: 启动转换命令 (CMD_START_SYNC)

### 2. 通道 2 切换指令流 (切换为 AIN2-AIN3 差分)
```text
06 40 56 44 24 48 10 4C 00 08
```
- `CONFIG0` 配置为 `0x56`，通道动态切换至 AIN2-AIN3 差分（Gain=8），其余保持连续转换。

---

## 开发环境与编译

- **IDE**: Keil MDK-ARM v5.24+ (ARMClang / AC6)
- **工程路径**: `MDK-ARM/firmware.uvprojx`
- **例程切换**: 在 `Example/example.h` 中修改 `ACTIVE_EXAMPLE` 宏定义即可选择运行的测试例程：
  ```c
  #define ACTIVE_EXAMPLE          EXAMPLE_CMD
  ```
