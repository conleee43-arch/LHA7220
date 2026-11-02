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

#include "lha7220.h"


/**
  * @brief  Software reset of LHA7220 via SPI command.
  *         Sends the RESET command and waits for the device to stabilize.
  * @param  None
  * @retval None
  */
void lha7220_SoftwareReset(void)
{
    lha7220_SendCommand(CMD_RESET);
    delay_us(300);   /* Wait for reset to complete (minimum 300us) */
}

/**
  * @brief  Initialize the LHA7220 ADC device.
  *         Performs a software reset, then writes a default configuration to
  *         CONFIG0..CONFIG3: continuous conversion mode (CM=1), AIN0-AIN1
  *         differential input, PGA gain=1, internal 2.048V reference,
  *         20 SPS normal mode, IDACs off, DRDYM=0 (data-ready indicated on
  *         the dedicated DRDY pin only).
  *         This function does NOT start conversions and does NOT enter
  *         power-down mode. After init, send CMD_START_SYNC to begin
  *         continuous conversions, or rewrite the registers for single-shot
  *         operation. (No example currently calls this function; each
  *         example performs its own reset + configuration.)
  * @param  None
  * @retval None
  */
void lha7220_Init(void)
{
    /* Software reset */
    lha7220_SoftwareReset();

    /* Configure default registers */
    uint8_t configData[4];

    /* CONFIG0: AIN0-AIN1 differential, gain=1 (PGA_BYPASS bit exists for
       ADS1220 compatibility only and has no effect on LHA7220) */
    configData[0] = CONFIG0_MUX_AIN0_AIN1 | CONFIG0_GAIN_1;

    /* CONFIG1: Normal mode, continuous conversion (CM=1), 20 SPS,
       temp sensor off, BCS off */
    configData[1] = CONFIG1_DR_20SPS | CONFIG1_MODE_NORMAL | CONFIG1_CM;

    /* CONFIG2: Internal reference, no 50/60 rejection, switch open, IDACs off */
    configData[2] = CONFIG2_VREF_INTERNAL | CONFIG2_50_60_NONE; /* PSW=0, IDAC=000 */

    /* CONFIG3: IDACs disabled, DRDYM=0 (only dedicated DRDY pin) */
    configData[3] = CONFIG3_I1MUX_DIS | CONFIG3_I2MUX_DIS; /* DRDYM=0 */

    lha7220_WriteRegister(REG_CONFIG0, configData, 4);
}

/**
  * @brief  Write one or more registers of the LHA7220.
  *         Sends the write command (WREG or LWREG) with the starting address
  *         and number of bytes (num-1) encoded in the single command byte.
  * @param  addr: Starting register address (e.g., REG_CONFIG0).
  * @param  data: Pointer to the data bytes to be written.
  * @param  num: Number of registers (bytes) to write (max 4).
  * @retval None
  */
void lha7220_WriteRegister(uint8_t addr, uint8_t *data, uint8_t num)
{
    uint8_t txData[16];
    uint8_t cmd;

    if (num == 0 || num > 4) return;   /* nn field is 2 bits, max num = 4 */

    /* Build single-byte command: CMD_WREG | (rr << 2) | nn */
    /* rr = addr & 0x03, nn = num - 1 */
    if (addr >= 4) {
        cmd = CMD_LWREG | ((addr & 0x03) << 2) | (num - 1);
    } else {
        cmd = CMD_WREG | ((addr & 0x03) << 2) | (num - 1);
    }

    txData[0] = cmd;

    for (uint8_t i = 0; i < num; i++) {
        txData[i + 1] = data[i];
    }

    CSN_LOW;
    HAL_SPI_Transmit(&hspi1, txData, num + 1, HAL_MAX_DELAY);
    CSN_HIGH;
}

/**
  * @brief  Read one or more registers of the LHA7220.
  *         Sends the read command (RREG or LRREG) with the starting address
  *         and number of bytes (num-1) encoded in the single command byte.
  * @param  addr: Starting register address (e.g., REG_CONFIG0).
  * @param  data: Pointer to buffer where the read data will be stored.
  * @param  num: Number of registers (bytes) to read (max 4).
  * @retval None
  */
void lha7220_ReadRegister(uint8_t addr, uint8_t *data, uint8_t num)
{
    uint8_t txData[1];
    uint8_t cmd;

    if (num == 0 || num > 4) return;

    /* Build single-byte command: CMD_RREG | (rr << 2) | nn */
    if (addr >= 4) {
        cmd = CMD_LRREG | ((addr & 0x03) << 2) | (num - 1);
    } else {
        cmd = CMD_RREG | ((addr & 0x03) << 2) | (num - 1);
    }

    txData[0] = cmd;

    CSN_LOW;
    HAL_SPI_Transmit(&hspi1, txData, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, data, num, HAL_MAX_DELAY);
    CSN_HIGH;
}

/**
  * @brief  Send a single-byte command to the LHA7220.
  *         This function handles CS control internally.
  * @param  command: Command byte (see command definitions in lha7220.h).
  * @retval None
  */
void lha7220_SendCommand(uint8_t command)
{
    CSN_LOW;
    HAL_SPI_Transmit(&hspi1, &command, 1, HAL_MAX_DELAY);
    CSN_HIGH;
}

/**
  * @brief  Read a 24-bit conversion result from the ADC.
  *         Reads 3 bytes from the device by clocking out dummy bytes.
  *         Assumes DRDY pin indicates data ready; CS is controlled internally.
  * @param  None
  * @retval uint32_t: Raw ADC code (24-bit data in lower 24 bits).
  */
uint32_t lha7220_ReadData(void)
{
    uint8_t rxData[3];
    uint32_t code = 0;
    uint8_t dummy[3] = {0, 0, 0};

    CSN_LOW;
    /* Transmit dummy bytes to generate clock and receive data */
    HAL_SPI_TransmitReceive(&hspi1, dummy, rxData, 3, HAL_MAX_DELAY);
    CSN_HIGH;

    code = ((uint32_t)rxData[0] << 16) |
           ((uint32_t)rxData[1] << 8)  |
           ((uint32_t)rxData[2] << 0);

    return code;
}

/**
  * @brief  Convert a raw ADC code to a voltage value in millivolts.
  *         The conversion uses the formula:
  *         voltage (mV) = (int32_t)code * 2 * ref / (gain * 2^24)
  *         where code is a 24-bit two's complement value.
  * @param  code: Raw ADC code (32-bit unsigned, only lower 24 bits used).
  * @param  ref: Reference voltage in millivolts (e.g., 2500 for 2.5 V).
  * @param  gain: PGA gain setting (1, 2, 4, ... 128).
  * @retval float: Calculated voltage in millivolts.
  */
float lha7220_CodeToVoltage(uint32_t code, uint32_t ref, uint8_t gain)
{
    int32_t signed_code;

    /* Convert 24-bit two's complement to 32-bit signed */
    if (code & 0x800000) {
        signed_code = (int32_t)(code | 0xFF000000);
    } else {
        signed_code = (int32_t)(code & 0x00FFFFFF);
    }

    float voltage = (float)signed_code * 2.0f * (float)ref / ((float)gain * 16777216.0f);
    return voltage;
}

