/**
 * @file example-polling.c
 * @brief 例程1：基础轮询连续差分采集
 * @details 
 *   - 输入通道：AIN0 - AIN1 差分
 *   - 增益设置：Gain = 1
 *   - 采样速率：20 SPS (单次周期 ~50ms)
 *   - 转换模式：连续转换模式 (CM=1)
 *   - 基准电压：内部 2.048V
 */

#include "example-polling.h"
#include "main.h"
#include "stdio.h"
#include "lha7220.h"

void example_polling_init(void)
{
    uint8_t cfg[4];

    printf("\r\n==================================================\r\n");
    printf("[Example 1] Continuous Polling Read Starting...\r\n");
    printf("Config: Diff AIN0-AIN1, Gain=1, DR=20SPS, VREF=2.048V\r\n");
    printf("==================================================\r\n");

    /* 1. 软件复位芯片并等待稳定 */
    lha7220_SoftwareReset();

    /* 2. 配置 CONFIG0..CONFIG3 寄存器 */
    cfg[0] = CONFIG0_MUX_AIN0_AIN1 | CONFIG0_GAIN_1;
    cfg[1] = CONFIG1_DR_20SPS | CONFIG1_MODE_NORMAL | CONFIG1_CM; /* CM=1: 连续模式 */
    cfg[2] = CONFIG2_VREF_INTERNAL | CONFIG2_50_60_NONE | CONFIG2_IDAC_OFF;
    cfg[3] = CONFIG3_I1MUX_DIS | CONFIG3_I2MUX_DIS;

    lha7220_WriteRegister(REG_CONFIG0, cfg, 4);

    /* 3. 发送 START/SYNC 指令启动连续转换 */
    lha7220_SendCommand(CMD_START_SYNC);
    delay_ms(10);
}

void example_polling_loop(void)
{
    /* 20 SPS 周期约 50ms，轮询延时 50ms 进行一次读取 */
    delay_ms(50);

    uint32_t adc_code = lha7220_ReadData();
    float volt_mv = lha7220_CodeToVoltage(adc_code, 2048, 1);

    printf("[POLL] Code: 0x%06lX (%ld) | Voltage: %8.4f mV\r\n", 
           (unsigned long)(adc_code & 0xFFFFFF), 
           (long)(adc_code & 0x800000 ? (int32_t)(adc_code | 0xFF000000) : (int32_t)(adc_code & 0x00FFFFFF)), 
           volt_mv);
}
