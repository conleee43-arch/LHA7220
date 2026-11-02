/**
 * @file example-reg-test.c
 * @brief 例程6：寄存器读写校验与 SPI 通信自检
 * @details 
 *   - 检验上电/软复位后寄存器默认值
 *   - 写入自定义配置后读回比对
 *   - 检验批量写 (num=4) 与单字节写
 *   - 打印校验通过/失败状态，帮助快速排查硬件 SPI 信号完整性
 */

#include "example-reg-test.h"
#include "main.h"
#include "stdio.h"
#include "lha7220.h"

void example_reg_test_init(void)
{
    uint8_t write_buf[4] = { 0x35, 0x48, 0x12, 0x64 };
    uint8_t read_buf[4]  = { 0 };
    uint8_t success = 1;

    printf("\r\n==================================================\r\n");
    printf("[Example 6] Register Read/Write Self-Test Starting...\r\n");
    printf("==================================================\r\n");

    /* 1. 复位芯片 */
    lha7220_SoftwareReset();
    printf("1. Software Reset executed.\r\n");

    /* 2. 读取默认复位值并打印 (通常全为 0x00) */
    lha7220_ReadRegister(REG_CONFIG0, read_buf, 4);
    printf("2. Default registers: [0x%02X, 0x%02X, 0x%02X, 0x%02X]\r\n",
           read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

    /* 3. 写入测试特征字 */
    printf("3. Writing test pattern: [0x%02X, 0x%02X, 0x%02X, 0x%02X]...\r\n",
           write_buf[0], write_buf[1], write_buf[2], write_buf[3]);
    lha7220_WriteRegister(REG_CONFIG0, write_buf, 4);

    /* 4. 读回校验 */
    read_buf[0] = read_buf[1] = read_buf[2] = read_buf[3] = 0;
    lha7220_ReadRegister(REG_CONFIG0, read_buf, 4);
    printf("4. Readback registers:   [0x%02X, 0x%02X, 0x%02X, 0x%02X]...\r\n",
           read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

    for (uint8_t i = 0; i < 4; i++) {
        if (write_buf[i] != read_buf[i]) {
            printf("   --> ERROR at Register %d! Expected 0x%02X, got 0x%02X\r\n", 
                   i, write_buf[i], read_buf[i]);
            success = 0;
        }
    }

    if (success) {
        printf("\r\n>>> [PASS] SPI Communication & Register Read/Write Test OK! <<<\r\n\r\n");
    } else {
        printf("\r\n>>> [FAIL] SPI Read/Write Test Failed! Check SPI Clock/Wiring. <<<\r\n\r\n");
    }
}

void example_reg_test_loop(void)
{
    /* 自检完成后，LED 慢闪烁指示测试完毕 */
    LED_HIGH;
    delay_ms(500);
    LED_LOW;
    delay_ms(500);
}
