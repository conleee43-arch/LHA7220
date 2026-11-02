/**
 * @file example-multichannel.h
 * @brief 例程4：单次转换模式 (Single-Shot) 下利用 DOUT/DRDY 下降沿的双通道差分轮询采集
 * @details 
 *   - 通道 1: AIN0 - AIN1 差分
 *   - 通道 2: AIN2 - AIN3 差分
 *   - 转换模式: CM = 0 (Single-Shot Mode)
 *   - 数据就绪指示: DOUT/DRDY (PA6 / SPI1_MISO) 下降沿，DRDYM = 1
 *   - 触发方式: 写 CONFIG0 自动触发单次转换
 */

#ifndef __EXAMPLE_MULTICHANNEL_H__
#define __EXAMPLE_MULTICHANNEL_H__

#include "main.h"

/* PGA 增益倍数设置 (默认 8 倍) */
#define MULTICH_GAIN_VAL        8
#define MULTICH_GAIN_CONFIG     CONFIG0_GAIN_8  /* 0x06 */

/* 参考基准电压 (mV) */
#define MULTICH_VREF_MV         2048

void example_multichannel_init(void);
void example_multichannel_loop(void);

#endif /* __EXAMPLE_MULTICHANNEL_H__ */
