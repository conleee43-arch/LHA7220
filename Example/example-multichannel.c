/**
 * @file example-multichannel.c
 * @brief 例程4：单次转换模式 (Single-Shot Mode) 下通过 DOUT/DRDY 下降沿判断的双通道差分轮询采集
 * @details 
 *   一、关键配置前提与硬件规则：
 *     1. DOUT/DRDY 引脚复用：
 *        - 在 STM32 硬件连接中，DOUT/DRDY 接入 PA6 (SPI1_MISO)。
 *        - 当 PA6 处于 SPI Alternate Function (AF5) 时，内部 Input Buffer 处于使能状态，
 *          可通过读取 GPIOA->IDR 寄存器直接获取引脚物理电平，无需频繁切换 GPIO 模式。
 *     2. DRDYM = 1 (使能 DOUT/DRDY Data Ready 功能)：
 *        - 配置 CONFIG3 (03h) 的 Bit 1 = 1 (写入 0x02)，使能 DOUT/DRDY 在转换完成时拉低指示。
 *     3. 严格遵守 CS 片选规则：
 *        - 当 CS 为高时，DOUT/DRDY 进入高阻态 (Hi-Z)，无法输出电平或跳变信号。
 *        - 发送 WREG 命令写入 CONFIG0 后，必须保持 CS 为低电平，等待 DOUT/DRDY 下降沿。
 *     4. 单次模式自动触发机制：
 *        - 设置 CONFIG1 的 CM = 0 (单次转换模式)。
 *        - 在单次模式下，每次写入任意配置寄存器 (WREG) 后，ADC 自动开启一次新的单次转换。
 * 
 *   二、通道定义与增益配置：
 *     - CH1: AIN0 - AIN1 差分，增益 8 倍，CONFIG0 = 0x06 (MUX=0000, GAIN=011)
 *     - CH2: AIN2 - AIN3 差分，增益 8 倍，CONFIG0 = 0x56 (MUX=0101, GAIN=011)
 */

#include "example-multichannel.h"
#include "main.h"
#include "stdio.h"
#include "lha7220.h"

/*============================================================================
 * 私有宏与常量定义
 *============================================================================*/
#define CONFIG0_CH1_VAL     (CONFIG0_MUX_AIN0_AIN1 | MULTICH_GAIN_CONFIG) /* 0x00 | 0x06 = 0x06 */
#define CONFIG0_CH2_VAL     (CONFIG0_MUX_AIN2_AIN3 | MULTICH_GAIN_CONFIG) /* 0x50 | 0x06 = 0x56 */

#define CONVERSION_TIMEOUT_MS   100  /* 20 SPS 单次转换典型周期 ~50ms，设定 100ms 超时 */

/*============================================================================
 * 底层时序辅助函数
 *============================================================================*/

/**
 * @brief  写入 CONFIG0 寄存器并保持 CS 为低电平，以触发单次转换并等待 DOUT/DRDY
 * @param  config0_val: 写入 CONFIG0 的数值 (配置通道 MUX 与 PGA GAIN)
 * @retval None
 */
static void write_config0_and_keep_cs_low(uint8_t config0_val)
{
    uint8_t txData[2];

    /* 构造 WREG 命令: CMD_WREG | (REG_CONFIG0 << 2) | (1 - 1) = 0x40 */
    txData[0] = CMD_WREG | ((REG_CONFIG0 & 0x03) << 2) | 0x00;
    txData[1] = config0_val;

    /* 1. 拉低片选 CS */
    CSN_LOW;

    /* 2. 发送 WREG 命令及配置字 */
    HAL_SPI_Transmit(&hspi1, txData, 2, HAL_MAX_DELAY);

    /* 3. 【核心时序要求】：
     *    写入完成后，ADC 自动开始该通道的单次转换。
     *    此处切勿拉高 CS！必须保持 CS 为低，以便 DOUT/DRDY 引脚能够驱动电平并产生下降沿。
     */
}

/**
 * @brief  轮询等待 DOUT/DRDY 引脚 (PA6) 产生下降沿 (从 High 跳变为 Low)
 * @param  timeout_ms: 超时等待时间 (毫秒)
 * @retval 1: 成功捕获下降沿 (转换完成)；0: 超时失败
 */
static uint8_t wait_dout_drdy_falling_edge(uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();

    /* 
     * 阶段 1：等待 DOUT/DRDY 处于高电平
     * (写入配置寄存器后，ADC 进入转换状态，DOUT/DRDY 引脚会被拉高)
     */
    while ((GPIOA->IDR & GPIO_PIN_6) == 0) {
        if ((HAL_GetTick() - start_tick) > 15) {
            /* 若超过 15ms 仍未检测到高电平，可能转换已极速完成或总线已就绪，跳入阶段2 */
            break;
        }
    }

    /* 
     * 阶段 2：等待 DOUT/DRDY 引脚电平跳变至低电平 (下降沿，标志单次转换结束)
     */
    while ((GPIOA->IDR & GPIO_PIN_6) != 0) {
        if ((HAL_GetTick() - start_tick) > timeout_ms) {
            return 0; /* 超时 */
        }
    }

    return 1; /* 成功检测到数据就绪下降沿 */
}

/**
 * @brief  发送 24 个 SCLK 读取转换数据，并附带 8 个 SCLK 将 DOUT/DRDY 硬件拉高 (手册 Figure 38)
 * @param  None
 * @retval 24-bit 原始采样码值
 */
static uint32_t read_conversion_data_24bit(void)
{
    uint8_t rxData[4] = {0};
    uint8_t dummy[4]  = {0, 0, 0, 0};

    /* 
     * 此时 CS 依然保持为低：
     * 前 24 个 SCLK 读取 3 字节转换结果；
     * 第 25~32 个 SCLK (多发 1 字节 dummy, DIN 保持低) 触发芯片内部将 DOUT/DRDY 引脚硬件拉高 (见手册 9.5.6 节图 38)
     */
    HAL_SPI_TransmitReceive(&hspi1, dummy, rxData, 4, HAL_MAX_DELAY);

    /* 数据读取与电平复位完毕，拉高片选 CS */
    CSN_HIGH;

    uint32_t raw_code = ((uint32_t)rxData[0] << 16) |
                        ((uint32_t)rxData[1] << 8)  |
                        ((uint32_t)rxData[2] << 0);
    return raw_code;
}

/**
 * @brief  执行单个通道的单次触发、等待下降沿与数据读取全流程
 * @param  channel_name: 通道名称字符串
 * @param  config0_val:  该通道对应的 CONFIG0 寄存器值
 * @param  p_volt_mv:    输出计算所得电压值 (毫伏)
 * @retval 1: 采集成功；0: 超时失败
 */
static uint8_t sample_single_channel(const char *channel_name, uint8_t config0_val, float *p_volt_mv)
{
    /* 步骤 1: 拉低 CS，写入 CONFIG0 (修改 MUX 并设置 Gain=8)，自动触发单次转换并保持 CS 为低 */
    write_config0_and_keep_cs_low(config0_val);

    /* 步骤 2: 保持 CS 为低，监测等待 DOUT/DRDY (PA6) 下降沿 */
    if (!wait_dout_drdy_falling_edge(CONVERSION_TIMEOUT_MS)) {
        CSN_HIGH; /* 超时后拉高 CS 保护总线 */
        printf("[%s] Error: Wait DOUT/DRDY falling edge timeout!\r\n", channel_name);
        return 0;
    }

    /* 步骤 3: 捕获到下降沿后，发送 24 个 SCLK 读出 3 字节转换结果，并拉高 CS */
    uint32_t raw_code = read_conversion_data_24bit();

    /* 步骤 4: 换算为毫伏物理电压 (参考电压 2048mV，增益 8) */
    *p_volt_mv = lha7220_CodeToVoltage(raw_code, MULTICH_VREF_MV, MULTICH_GAIN_VAL);

    /* 打印单通道采集详情 */
    int32_t signed_code = (raw_code & 0x800000) ? (int32_t)(raw_code | 0xFF000000) : (int32_t)raw_code;
    printf("[%s] Code: 0x%06lX (%8ld) | Volt: %8.4f mV\r\n", 
           channel_name, 
           (unsigned long)(raw_code & 0xFFFFFF), 
           (long)signed_code, 
           *p_volt_mv);

    return 1;
}

/*============================================================================
 * 对外接口实现
 *============================================================================*/

/**
 * @brief  初始化双通道差分单次采样例程
 */
void example_multichannel_init(void)
{
    uint8_t cfg_val;

    printf("\r\n==================================================================\r\n");
    printf("[Example 4] Single-Shot Multi-Channel Polling via DOUT/DRDY Edge\r\n");
    printf("Hardware Rule: DRDYM=1, CS kept LOW during conversion, Auto-trigger by WREG\r\n");
    printf("Channels: CH1 (AIN0-AIN1, Gain=8), CH2 (AIN2-AIN3, Gain=8)\r\n");
    printf("==================================================================\r\n");

    /* 1. 软件复位芯片并等待稳定 */
    lha7220_SoftwareReset();

    /* 
     * 2. 配置 CONFIG1 (01h):
     *    DR = 20 SPS, Mode = Normal, CM = 0 (单次转换模式 Single-Shot), TS=0, BCS=0
     *    写入 0x00
     */
    cfg_val = CONFIG1_DR_20SPS | CONFIG1_MODE_NORMAL; /* 无 CONFIG1_CM 即 CM=0 */
    lha7220_WriteRegister(REG_CONFIG1, &cfg_val, 1);

    /* 
     * 3. 配置 CONFIG2 (02h):
     *    选用内部基准 VREF = 2.048V, 无滤波, IDAC 关闭
     *    写入 0x00
     */
    cfg_val = CONFIG2_VREF_INTERNAL | CONFIG2_50_60_NONE | CONFIG2_IDAC_OFF;
    lha7220_WriteRegister(REG_CONFIG2, &cfg_val, 1);

    /* 
     * 4. 【核心配置】：设置 CONFIG3 (03h)
     *    Bit 1 (DRDYM) = 1 (写入 0x02)，使能 DOUT/DRDY 引脚在转换结束时输出数据就绪脉冲
     */
    cfg_val = CONFIG3_DRDYM; /* 0x02 */
    lha7220_WriteRegister(REG_CONFIG3, &cfg_val, 1);

    printf("Initialization Complete. CONFIG1=0x00(CM=0), CONFIG2=0x00(VREF=2.048V), CONFIG3=0x02(DRDYM=1)\r\n");
}

/**
 * @brief  循环执行两组差分通道的轮流采样流程
 */
void example_multichannel_loop(void)
{
    float ch1_mv = 0.0f;
    float ch2_mv = 0.0f;

    printf("\r\n--- Scanning Channels ---\r\n");

    /* 采集通道 1: AIN0 - AIN1 (CONFIG0 = 0x06, MUX=0000, Gain=8) */
    sample_single_channel("CH1 (AIN0-AIN1)", CONFIG0_CH1_VAL, &ch1_mv);

    /* 采集通道 2: AIN2 - AIN3 (CONFIG0 = 0x56, MUX=0101, Gain=8) */
    sample_single_channel("CH2 (AIN2-AIN3)", CONFIG0_CH2_VAL, &ch2_mv);

    /* 两轮循环之间延时，方便上位机或串口助手观察 */
    delay_ms(800);
}
