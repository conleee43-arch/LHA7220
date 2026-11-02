/**
 * @file example-single-shot.c
 * @brief 例程5：单次触发采样与低功耗待机模式
 * @details 
 *   - 配置单次转换模式 (CM=0)
 *   - 平时发送 CMD_POWERDOWN 令芯片进入超低功耗休眠
 *   - 采样周期到达时，发送 CMD_START_SYNC 唤醒并启动单次转换
 *   - 转换完成读取数据后立即重新休眠，极大降低平均功耗
 */

#include "example-single-shot.h"
#include "main.h"
#include "stdio.h"
#include "lha7220.h"

void example_single_shot_init(void)
{
    uint8_t cfg[4];

    printf("\r\n==================================================\r\n");
    printf("[Example 5] Single-Shot & Low Power Starting...\r\n");
    printf("Mode: Single-Shot (CM=0) + CMD_POWERDOWN\r\n");
    printf("==================================================\r\n");

    /* 1. 复位芯片 */
    lha7220_SoftwareReset();

    /* 2. 配置单次转换模式 (CM=0) */
    cfg[0] = CONFIG0_MUX_AIN0_AIN1 | CONFIG0_GAIN_1;
    cfg[1] = CONFIG1_DR_20SPS | CONFIG1_MODE_NORMAL; /* 无 CONFIG1_CM，即单次模式 */
    cfg[2] = CONFIG2_VREF_INTERNAL | CONFIG2_50_60_NONE | CONFIG2_IDAC_OFF;
    cfg[3] = CONFIG3_I1MUX_DIS | CONFIG3_I2MUX_DIS;

    lha7220_WriteRegister(REG_CONFIG0, cfg, 4);

    /* 3. 初始进入休眠模式 */
    lha7220_SendCommand(CMD_POWERDOWN);
    printf("ADC entered PowerDown mode.\r\n");
}

void example_single_shot_loop(void)
{
    /* 模拟长采样周期休眠：每隔 2 秒唤醒采集一次 */
    delay_ms(2000);

    printf("\r\n[WAKEUP] Trigger single-shot conversion...\r\n");

    /* 1. 发送 START/SYNC 唤醒并开始一次单次采样 */
    lha7220_SendCommand(CMD_START_SYNC);

    /* 2. 等待采样完成 (20 SPS 周期约 50ms) */
    delay_ms(55);

    /* 3. 读取采样结果 */
    uint32_t adc_code = lha7220_ReadData();
    float volt_mv = lha7220_CodeToVoltage(adc_code, 2048, 1);

    printf("[RESULT] Code: 0x%06lX | Voltage: %8.4f mV\r\n", 
           (unsigned long)(adc_code & 0xFFFFFF), volt_mv);

    /* 4. 再次进入低功耗待机 */
    lha7220_SendCommand(CMD_POWERDOWN);
    printf("[SLEEP] ADC returned to PowerDown mode.\r\n");
}
