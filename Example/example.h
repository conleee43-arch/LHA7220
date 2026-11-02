/**
 * @file example.h
 * @brief LHA7220 测试例程统一管理与选择中枢
 */

#ifndef __EXAMPLE_H__
#define __EXAMPLE_H__

#include "main.h"
#include "stdio.h"
#include "lha7220.h"

/*============================================================================
 * 测试例程编号定义
 *============================================================================*/
#define EXAMPLE_POLLING         1  /* 例程1：基础轮询连续采集 (AIN0-AIN1, 20SPS) */
#define EXAMPLE_INTERRUPT       2  /* 例程2：DRDY 硬件外部中断采集 (EXTI1 中断) */
#define EXAMPLE_TEMPERATURE     3  /* 例程3：芯片内部温度传感器测量 (TS=1) */
#define EXAMPLE_MULTICHANNEL    4  /* 例程4：多通道循环切换采样 (AIN0-AIN1, AIN1-AIN2等) */
#define EXAMPLE_SINGLE_SHOT     5  /* 例程5：单次触发与低功耗待机模式 (Single-Shot + Powerdown) */
#define EXAMPLE_REG_TEST        6  /* 例程6：寄存器读写校验与通信自检 (SPI Integrity Test) */
#define EXAMPLE_CMD             7  /* 例程7：指令序列通道切换测试 (0x06->0x40..->0x08->DRDYN PA1) */

/*============================================================================
 * >>>>>>>>>>>>>>>>>>>>> 请在此处选择要运行的例程 <<<<<<<<<<<<<<<<<<<<<
 * 修改下面的宏定义为你需要运行的例程编号即可，无需修改 main.c
 *============================================================================*/
#define ACTIVE_EXAMPLE          EXAMPLE_CMD

/*============================================================================
 * 统一接口声明 (供 main.c 调用)
 *============================================================================*/
void example_init(void);
void example_loop(void);
void example_on_drdy_irq(void);

#endif /* __EXAMPLE_H__ */
