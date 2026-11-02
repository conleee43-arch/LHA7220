/**
 * @file example-temperature.c
 * @brief 例程3：芯片内部温度传感器读取
 * @details 
 *   - LHA7220 内置高精度温度传感器，设置 CONFIG1 寄存器中 TS=1 开启
 *   - 转换数据为 14 位有符号数，左对齐存放在 24 位结果的高 14 位中 (D[23:10])
 *   - 分辨率为 0.03125 ℃/LSB (即 1/32 ℃)
 */

#include "example-temperature.h"
#include "main.h"
#include "stdio.h"
#include "lha7220.h"

void example_temperature_init(void)
{
    uint8_t cfg[4];

    printf("\r\n==================================================\r\n");
    printf("[Example 3] Internal Temperature Sensor Starting...\r\n");
    printf("Config: TS=1 (Temp Sensor ON), Continuous Mode\r\n");
    printf("==================================================\r\n");

    /* 1. 复位芯片 */
    lha7220_SoftwareReset();

    /* 2. 配置芯片：开启温度传感器 (CONFIG1_TS) */
    cfg[0] = CONFIG0_MUX_AIN0_AIN1 | CONFIG0_GAIN_1;
    cfg[1] = CONFIG1_DR_20SPS | CONFIG1_MODE_NORMAL | CONFIG1_CM | CONFIG1_TS;
    cfg[2] = CONFIG2_VREF_INTERNAL | CONFIG2_50_60_NONE | CONFIG2_IDAC_OFF;
    cfg[3] = CONFIG3_I1MUX_DIS | CONFIG3_I2MUX_DIS;

    lha7220_WriteRegister(REG_CONFIG0, cfg, 4);

    /* 3. 启动连续转换 */
    lha7220_SendCommand(CMD_START_SYNC);
    delay_ms(50);
}

void example_temperature_loop(void)
{
    delay_ms(500);

    uint32_t raw_code = lha7220_ReadData();

    /* 
     * 温度结果位于 24-bit 的高 14 位 (右移 10 位):
     */
    int32_t temp_code = (int32_t)(raw_code >> 10);

    /* 14 位补码符号扩展 */
    if (temp_code & 0x2000) {
        temp_code |= 0xFFFFC000;
    }

    /* 1 LSB = 0.03125 ℃ (1/32 ℃) */
    float temperature_c = (float)temp_code * 0.03125f;

    printf("[TEMP] Raw: 0x%06lX | Code: %6ld | Temp: %6.2f degC\r\n", 
           (unsigned long)(raw_code & 0xFFFFFF), 
           (long)temp_code, 
           temperature_c);
}
