/**
 * \copyright Copyright (C) 2020 Legendsemi Technology Co., Ltd - http://www.legendsemi.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Legendsemi Technology Co., Ltd nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef LHA7220_H
#define LHA7220_H

#include "main.h"
#include "spi.h"

/*============================================================================
 * Command definitions (Table 18)
 *============================================================================*/
#define CMD_RESET       0x06    /* 0000 011x */
#define CMD_START_SYNC  0x08    /* 0000 100x */
#define CMD_POWERDOWN   0x02    /* 0000 001x */
#define CMD_RDATA       0x10    /* 0001 xxxx */
#define CMD_RREG        0x20    /* 0010 rrnn */
#define CMD_WREG        0x40    /* 0100 rrnn */
#define CMD_LRREG       0x30    /* 0011 rrnn (extended read) */
#define CMD_LWREG       0x50    /* 0101 rrnn (extended write) */

/*============================================================================
 * Register addresses (Section 9.6)
 *============================================================================*/
#define REG_CONFIG0     0x00
#define REG_CONFIG1     0x01
#define REG_CONFIG2     0x02
#define REG_CONFIG3     0x03
#define REG_CONFIG4     0x04    /* extended register */
#define REG_CONFIG5     0x05    /* extended register */
#define REG_CONFIG6     0x06    /* extended register */

/*============================================================================
 * CONFIG0 Register (offset 0x00) [reset=0x00]
 *============================================================================*/
#define CONFIG0_MUX_MASK        (0x0F << 4)
#define CONFIG0_MUX_AIN0_AIN1   (0x00 << 4)   /* 0000 */
#define CONFIG0_MUX_AIN0_AIN2   (0x01 << 4)   /* 0001 */
#define CONFIG0_MUX_AIN0_AIN3   (0x02 << 4)   /* 0010 */
#define CONFIG0_MUX_AIN1_AIN2   (0x03 << 4)   /* 0011 */
#define CONFIG0_MUX_AIN1_AIN3   (0x04 << 4)   /* 0100 */
#define CONFIG0_MUX_AIN2_AIN3   (0x05 << 4)   /* 0101 */
#define CONFIG0_MUX_AIN1_AIN0   (0x06 << 4)   /* 0110 */
#define CONFIG0_MUX_AIN3_AIN2   (0x07 << 4)   /* 0111 */
#define CONFIG0_MUX_AIN0_AVSS   (0x08 << 4)   /* 1000 */
#define CONFIG0_MUX_AIN1_AVSS   (0x09 << 4)   /* 1001 */
#define CONFIG0_MUX_AIN2_AVSS   (0x0A << 4)   /* 1010 */
#define CONFIG0_MUX_AIN3_AVSS   (0x0B << 4)   /* 1011 */
#define CONFIG0_MUX_MONITOR_REF (0x0C << 4)   /* 1100 (Vref/4) */
#define CONFIG0_MUX_MONITOR_AVDD (0x0D << 4)  /* 1101 (AVDD/4) */
#define CONFIG0_MUX_SHORT_MID   (0x0E << 4)   /* 1110 (inputs shorted to mid) */
#define CONFIG0_MUX_RESERVED    (0x0F << 4)   /* 1111 */

#define CONFIG0_GAIN_MASK       (0x07 << 1)
#define CONFIG0_GAIN_1          (0x00 << 1)
#define CONFIG0_GAIN_2          (0x01 << 1)
#define CONFIG0_GAIN_4          (0x02 << 1)
#define CONFIG0_GAIN_8          (0x03 << 1)
#define CONFIG0_GAIN_16         (0x04 << 1)
#define CONFIG0_GAIN_32         (0x05 << 1)
#define CONFIG0_GAIN_64         (0x06 << 1)
#define CONFIG0_GAIN_128        (0x07 << 1)

#define CONFIG0_PGA_BYPASS      (0x01 << 0)   /* compatibility only, has no effect */

/*============================================================================
 * CONFIG1 Register (offset 0x01) [reset=0x00]
 *============================================================================*/
#define CONFIG1_DR_MASK         (0x07 << 5)
/* Data rates depend on MODE setting, see Table 22 */
#define CONFIG1_DR_20SPS        (0x00 << 5)   /* Normal mode */
#define CONFIG1_DR_45SPS        (0x01 << 5)
#define CONFIG1_DR_90SPS        (0x02 << 5)
#define CONFIG1_DR_175SPS       (0x03 << 5)
#define CONFIG1_DR_330SPS       (0x04 << 5)
#define CONFIG1_DR_600SPS       (0x05 << 5)
#define CONFIG1_DR_1000SPS      (0x06 << 5)
#define CONFIG1_DR_2000SPS      (0x07 << 5)

#define CONFIG1_MODE_MASK       (0x03 << 3)
#define CONFIG1_MODE_NORMAL     (0x00 << 3)   /* 00 */
#define CONFIG1_MODE_DUTY_CYCLE (0x01 << 3)   /* 01 */
#define CONFIG1_MODE_TURBO      (0x02 << 3)   /* 10 */
#define CONFIG1_MODE_RESERVED   (0x03 << 3)   /* 11 */

#define CONFIG1_CM              (0x01 << 2)   /* 0: single-shot, 1: continuous */
#define CONFIG1_TS              (0x01 << 1)   /* temperature sensor enable */
#define CONFIG1_BCS             (0x01 << 0)   /* burn-out current sources */

/*============================================================================
 * CONFIG2 Register (offset 0x02) [reset=0x00]
 *============================================================================*/
#define CONFIG2_VREF_MASK       (0x03 << 6)
#define CONFIG2_VREF_INTERNAL   (0x00 << 6)   /* internal 2.048V */
#define CONFIG2_VREF_EXTERNAL0  (0x01 << 6)   /* REFP0/REFN0 */
#define CONFIG2_VREF_EXTERNAL1  (0x02 << 6)   /* AIN0/REFP1 & AIN3/REFN1 */
#define CONFIG2_VREF_AVDD       (0x03 << 6)   /* AVDD-AVSS */

#define CONFIG2_50_60_MASK      (0x03 << 4)
#define CONFIG2_50_60_NONE      (0x00 << 4)   /* no rejection */
#define CONFIG2_50_60_BOTH      (0x01 << 4)   /* simultaneous 50/60 Hz rejection */
#define CONFIG2_50_60_50HZ      (0x02 << 4)   /* 50 Hz only */
#define CONFIG2_50_60_60HZ      (0x03 << 4)   /* 60 Hz only */

#define CONFIG2_PSW             (0x01 << 3)   /* low-side power switch control */

#define CONFIG2_IDAC_MASK       (0x07 << 0)
#define CONFIG2_IDAC_OFF        (0x00 << 0)
#define CONFIG2_IDAC_10uA       (0x01 << 0)
#define CONFIG2_IDAC_50uA       (0x02 << 0)
#define CONFIG2_IDAC_100uA      (0x03 << 0)
#define CONFIG2_IDAC_250uA      (0x04 << 0)
#define CONFIG2_IDAC_500uA      (0x05 << 0)
#define CONFIG2_IDAC_1000uA     (0x06 << 0)
#define CONFIG2_IDAC_1500uA     (0x07 << 0)

/*============================================================================
 * CONFIG3 Register (offset 0x03) [reset=0x00]
 *============================================================================*/
#define CONFIG3_I1MUX_MASK      (0x07 << 5)
#define CONFIG3_I1MUX_DIS       (0x00 << 5)
#define CONFIG3_I1MUX_AIN0      (0x01 << 5)
#define CONFIG3_I1MUX_AIN1      (0x02 << 5)
#define CONFIG3_I1MUX_AIN2      (0x03 << 5)
#define CONFIG3_I1MUX_AIN3      (0x04 << 5)
#define CONFIG3_I1MUX_REFP0     (0x05 << 5)
#define CONFIG3_I1MUX_REFN0     (0x06 << 5)
#define CONFIG3_I1MUX_RESERVED  (0x07 << 5)

#define CONFIG3_I2MUX_MASK      (0x07 << 2)
#define CONFIG3_I2MUX_DIS       (0x00 << 2)
#define CONFIG3_I2MUX_AIN0      (0x01 << 2)
#define CONFIG3_I2MUX_AIN1      (0x02 << 2)
#define CONFIG3_I2MUX_AIN2      (0x03 << 2)
#define CONFIG3_I2MUX_AIN3      (0x04 << 2)
#define CONFIG3_I2MUX_REFP0     (0x05 << 2)
#define CONFIG3_I2MUX_REFN0     (0x06 << 2)
#define CONFIG3_I2MUX_RESERVED  (0x07 << 2)

#define CONFIG3_DRDYM           (0x01 << 1)   /* DOUT/DRDY pin used as data-ready indicator */

/*============================================================================
 * CONFIG4 Register (offset 0x04) [reset=0x00] (extended)
 *============================================================================*/
#define CONFIG4_VBIAS_SEL_MASK  (0x0F << 2)
#define CONFIG4_VBIAS_DIS       (0x00 << 2)
#define CONFIG4_VBIAS_AIN0      (0x01 << 2)
#define CONFIG4_VBIAS_AIN1      (0x02 << 2)
#define CONFIG4_VBIAS_AIN2      (0x04 << 2)
#define CONFIG4_VBIAS_AIN3      (0x08 << 2)

#define CONFIG4_SPI_TIMEOUT_EN  (0x01 << 1)
#define CONFIG4_SPI_CRC_EN      (0x01 << 0)

/*============================================================================
 * CONFIG5 Register (offset 0x05) [reset=0x00] (extended) read/clear bits
 *============================================================================*/
#define CONFIG5_PGA_UV          (0x01 << 5)
#define CONFIG5_PGA_OV          (0x01 << 4)
#define CONFIG5_ADC_UV          (0x01 << 3)
#define CONFIG5_ADC_OV          (0x01 << 2)
#define CONFIG5_CRC_ERR         (0x01 << 1)   /* write 1 to clear */
#define CONFIG5_DRDY            (0x01 << 0)   /* read clears it */

/*============================================================================
 * CONFIG6 Register (offset 0x06) [reset=0x00] (extended)
 *============================================================================*/
#define CONFIG6_USERKEY_MASK    (0xFF << 0)

/*============================================================================
 * Function prototypes
 *============================================================================*/
void lha7220_SoftwareReset(void);
void lha7220_Init(void);
void lha7220_WriteRegister(uint8_t addr, uint8_t *data, uint8_t num);
void lha7220_ReadRegister(uint8_t addr, uint8_t *data, uint8_t num);
void lha7220_SendCommand(uint8_t command);
uint32_t lha7220_ReadData(void);
float lha7220_CodeToVoltage(uint32_t code, uint32_t ref, uint8_t gain);

#endif /* LHA7220_H */