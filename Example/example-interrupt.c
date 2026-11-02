/**
 * @file example-interrupt.c
 * @brief 例程2：DRDY 硬件外部中断触发数据读取
 * @details 
 *   - LHA7220 完成单次转换后将 DRDYn 引脚拉低
 *   - STM32 的 EXTI1 下降沿中断捕获该信号并置位标志位
 *   - 主循环无阻塞检测到标志位后立即读取数据，保证采样时效性
 */

#include "example-interrupt.h"
#include "main.h"
#include "stdio.h"
#include "lha7220.h"

static volatile uint8_t s_data_ready_flag = 0;

void example_interrupt_init(void)
{
    uint8_t cfg[4];

    printf("\r\n==================================================\r\n");
    printf("[Example 2] Hardware DRDY Interrupt Read Starting...\r\n");
    printf("Config: Diff AIN0-AIN1, Gain=1, DR=20SPS, EXTI1(DRDY) Trigger\r\n");
    printf("==================================================\r\n");

    s_data_ready_flag = 0;

    /* 1. 复位芯片 */
    lha7220_SoftwareReset();

    /* 2. 配置芯片参数：连续采样模式，20 SPS */
    cfg[0] = CONFIG0_MUX_AIN0_AIN1 | CONFIG0_GAIN_1;
    cfg[1] = CONFIG1_DR_20SPS | CONFIG1_MODE_NORMAL | CONFIG1_CM;
    cfg[2] = CONFIG2_VREF_INTERNAL | CONFIG2_50_60_NONE | CONFIG2_IDAC_OFF;
    cfg[3] = CONFIG3_I1MUX_DIS | CONFIG3_I2MUX_DIS;

    lha7220_WriteRegister(REG_CONFIG0, cfg, 4);

    /* 3. 启动转换 */
    lha7220_SendCommand(CMD_START_SYNC);
}

void example_interrupt_on_drdy(void)
{
    /* 由 main.c 中的 EXTI1 回调调用 */
    s_data_ready_flag = 1;
}

void example_interrupt_loop(void)
{
    if (s_data_ready_flag) {
        s_data_ready_flag = 0;

        uint32_t adc_code = lha7220_ReadData();
        float volt_mv = lha7220_CodeToVoltage(adc_code, 2048, 1);

        printf("[INT] Code: 0x%06lX | Voltage: %8.4f mV\r\n", 
               (unsigned long)(adc_code & 0xFFFFFF), volt_mv);
    }
}
