/**
 * @file LHA7220-CMD.c
 * @brief LHA7220 动态指令流解析与通道切换测试用例实现
 */

#include "LHA7220-CMD.h"
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"
#include "lha7220.h"

/*============================================================================
 * 芯片状态全局变量
 *============================================================================*/
static lha7220_cmd_device_state_t g_state;
static volatile uint8_t s_drdy_irq_flag = 0;

/* 双通道预设指令序列 (配置内部 2.048V 参考: 0x48 0x10) */
static const uint8_t s_cmd_preset_ch1[] = {
    0x06, 0x40, 0x06, 0x44, 0x24, 0x48, 0x10, 0x4C, 0x00, 0x08
};

static const uint8_t s_cmd_preset_ch2[] = {
    0x06, 0x40, 0x56, 0x44, 0x24, 0x48, 0x10, 0x4C, 0x00, 0x08
};

/* 接收串口指令的本地备份缓冲区 */
static uint8_t s_uart_rx_cache[RX_MAXLEN];
static uint16_t s_uart_rx_len = 0;

/*============================================================================
 * 通道名称与参数解析辅助函数
 *============================================================================*/
static void get_mux_name(uint8_t mux, char *buf, size_t buf_len)
{
    switch (mux) {
        case 0x00: snprintf(buf, buf_len, "AIN0-AIN1 (Diff)"); break;
        case 0x01: snprintf(buf, buf_len, "AIN0-AIN2 (Diff)"); break;
        case 0x02: snprintf(buf, buf_len, "AIN0-AIN3 (Diff)"); break;
        case 0x03: snprintf(buf, buf_len, "AIN1-AIN2 (Diff)"); break;
        case 0x04: snprintf(buf, buf_len, "AIN1-AIN3 (Diff)"); break;
        case 0x05: snprintf(buf, buf_len, "AIN2-AIN3 (Diff)"); break;
        case 0x06: snprintf(buf, buf_len, "AIN1-AIN0 (Diff)"); break;
        case 0x07: snprintf(buf, buf_len, "AIN3-AIN2 (Diff)"); break;
        case 0x08: snprintf(buf, buf_len, "AIN0-AVSS (Single)"); break;
        case 0x09: snprintf(buf, buf_len, "AIN1-AVSS (Single)"); break;
        case 0x0A: snprintf(buf, buf_len, "AIN2-AVSS (Single)"); break;
        case 0x0B: snprintf(buf, buf_len, "AIN3-AVSS (Single)"); break;
        case 0x0C: snprintf(buf, buf_len, "VREF/4 Monitor"); break;
        case 0x0D: snprintf(buf, buf_len, "AVDD/4 Monitor"); break;
        case 0x0E: snprintf(buf, buf_len, "Inputs Shorted"); break;
        default:   snprintf(buf, buf_len, "Reserved (0x%X)", mux); break;
    }
}

/*============================================================================
 * 动态指令流解析与执行引擎
 *============================================================================*/

/**
 * @brief 动态指令流解析与执行引擎
 * @param cmd_stream: 待执行的 HEX 指令字节流
 * @param len: 指令流长度
 * @retval uint8_t: 1 成功解析执行, 0 失败
 */
uint8_t lha7220_cmd_parse_and_execute(const uint8_t *cmd_stream, uint16_t len)
{
    if (cmd_stream == NULL || len == 0) return 0;

    printf("\r\n>>> [CMD ENGINE] Processing Command Stream (Length: %u Bytes) >>>\r\n", len);
    printf("    Hex Dump: ");
    for (uint16_t i = 0; i < len; i++) {
        printf("%02X ", cmd_stream[i]);
    }
    printf("\r\n");

    uint16_t idx = 0;
    while (idx < len) {
        uint8_t byte = cmd_stream[idx];

        /* 1. 复位命令 0x06 (CMD_RESET) */
        if (byte == CMD_RESET) { /* 0x06 */
            CSN_LOW;
            HAL_SPI_Transmit(&hspi1, &byte, 1, HAL_MAX_DELAY);
            CSN_HIGH;

            /* 芯片复位后必须延时至少 300us */
            delay_us(300);
            g_state.is_running = 0;
            printf("  [0x06] RESET executed -> delay 300us completed.\r\n");
            idx++;
        }
        /* 2. 启动/同步转换命令 0x08 (CMD_START_SYNC) */
        else if (byte == CMD_START_SYNC) { /* 0x08 */
            s_drdy_irq_flag = 0;

            CSN_LOW;
            HAL_SPI_Transmit(&hspi1, &byte, 1, HAL_MAX_DELAY);
            CSN_HIGH;

            g_state.is_running = 1;
            printf("  [0x08] START/SYNC executed -> ADC conversion started.\r\n");
            idx++;
        }
        /* 3. 进入低功耗休眠命令 0x02 (CMD_POWERDOWN) */
        else if (byte == CMD_POWERDOWN) { /* 0x02 */
            CSN_LOW;
            HAL_SPI_Transmit(&hspi1, &byte, 1, HAL_MAX_DELAY);
            CSN_HIGH;

            g_state.is_running = 0;
            printf("  [0x02] POWERDOWN executed -> ADC in standby mode.\r\n");
            idx++;
        }
        /* 4. 写寄存器命令 WREG: 0x40 ~ 0x4F (格式: 0100 rr nn) */
        else if ((byte & 0xF0) == 0x40) {
            uint8_t reg_start = (byte >> 2) & 0x03; /* rr: 起始寄存器 0..3 */
            uint8_t count = (byte & 0x03) + 1;      /* nn+1: 写入字节数 1..4 */

            /* 校验后面是否有足够的待写数据字节 */
            if (idx + count >= len) {
                printf("  [ERROR] Incomplete WREG data: need %u bytes, stream ended!\r\n", count);
                return 0;
            }

            /* 发送 SPI 写寄存器事务: [WREG命令字, 数据1, 数据2, ...] */
            uint8_t tx_buf[5];
            tx_buf[0] = byte;
            for (uint8_t i = 0; i < count; i++) {
                tx_buf[1 + i] = cmd_stream[idx + 1 + i];
            }

            CSN_LOW;
            HAL_SPI_Transmit(&hspi1, tx_buf, 1 + count, HAL_MAX_DELAY);
            CSN_HIGH;

            /* 动态解析并更新内部设备状态 */
            for (uint8_t i = 0; i < count; i++) {
                uint8_t cur_reg = reg_start + i;
                uint8_t cur_val = cmd_stream[idx + 1 + i];

                if (cur_reg < 4) {
                    g_state.config[cur_reg] = cur_val;
                }

                if (cur_reg == REG_CONFIG0) {
                    /* CONFIG0: 通道 MUX 与 PGA GAIN */
                    g_state.mux = (cur_val >> 4) & 0x0F;
                    uint8_t gain_bits = (cur_val >> 1) & 0x07;
                    g_state.gain = (1 << gain_bits);
                    get_mux_name(g_state.mux, g_state.channel_name, sizeof(g_state.channel_name));
                    printf("  [0x%02X, 0x%02X] Configured CONFIG0 -> Channel: [%s], Gain: [%ux]\r\n", 
                           byte, cur_val, g_state.channel_name, g_state.gain);
                } 
                else if (cur_reg == REG_CONFIG1) {
                    /* CONFIG1: 采样率 DR 与 转换模式 CM */
                    g_state.dr_idx = (cur_val >> 5) & 0x07;
                    g_state.is_continuous = (cur_val & CONFIG1_CM) ? 1 : 0;
                    printf("  [0x%02X, 0x%02X] Configured CONFIG1 -> DR_Index: [%u], Mode: [%s]\r\n",
                           byte, cur_val, g_state.dr_idx, g_state.is_continuous ? "Continuous" : "Single-Shot");
                } 
                else if (cur_reg == REG_CONFIG2) {
                    /* CONFIG2: 基准电压 VREF 与工频抑制 */
                    uint8_t vref_sel = (cur_val >> 6) & 0x03;
                    if (vref_sel == 0) {
                        g_state.vref_mv = LHA7220_CMD_INT_VREF_MV; /* 内部 2.048V */
                    } else {
                        g_state.vref_mv = LHA7220_CMD_EXT_VREF_MV; /* 外部基准 (默认 2.5V) */
                    }
                    printf("  [0x%02X, 0x%02X] Configured CONFIG2 -> VREF: [%.1f mV (VREF_SEL=%u)]\r\n",
                           byte, cur_val, g_state.vref_mv, vref_sel);
                } 
                else if (cur_reg == REG_CONFIG3) {
                    /* CONFIG3: IDAC 与 DRDYM */
                    uint8_t drdym = (cur_val >> 1) & 0x01;
                    printf("  [0x%02X, 0x%02X] Configured CONFIG3 -> DRDYM: [%u (0:Dedicated DRDYN PA1)]\r\n",
                           byte, cur_val, drdym);
                }
            }

            idx += (1 + count);
        }
        else {
            /* 其它单字节指令直接发送 */
            CSN_LOW;
            HAL_SPI_Transmit(&hspi1, &byte, 1, HAL_MAX_DELAY);
            CSN_HIGH;
            printf("  [0x%02X] Sent raw command byte.\r\n", byte);
            idx++;
        }
    }

    printf("<<< [CMD ENGINE] Configuration Applied! Ready for Conversion. <<<\r\n\r\n");
    return 1;
}

/**
 * @brief 读取 24 位 ADC 转换数据并计算电压
 */
static uint8_t read_conversion_data(lha7220_cmd_sample_result_t *res)
{
    uint8_t rx[3] = {0, 0, 0};
    uint8_t dummy[3] = {0, 0, 0};

    CSN_LOW;
    HAL_SPI_TransmitReceive(&hspi1, dummy, rx, 3, HAL_MAX_DELAY);
    CSN_HIGH;

    res->raw_code = ((uint32_t)rx[0] << 16) | ((uint32_t)rx[1] << 8) | (uint32_t)rx[2];

    /* 24 位二进制补码扩展为 32 位整型 */
    if (res->raw_code & 0x800000) {
        res->signed_code = (int32_t)(res->raw_code | 0xFF000000);
    } else {
        res->signed_code = (int32_t)(res->raw_code & 0x00FFFFFF);
    }

    /* 根据当前解析出的实际增益与参考电压计算 */
    uint8_t gain = (g_state.gain == 0) ? 1 : g_state.gain;
    float vref = (g_state.vref_mv == 0.0f) ? LHA7220_CMD_EXT_VREF_MV : g_state.vref_mv;

    res->voltage_mv = ((float)res->signed_code * 2.0f * vref) / ((float)gain * 16777216.0f);
    return 1;
}

/*============================================================================
 * 串口不定长指令接收回调与管理
 *============================================================================*/

/**
 * @brief STM32 HAL 串口空闲/不定长接收事件回调
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART2) {
        if (Size > 0 && Size <= RX_MAXLEN) {
            memcpy(s_uart_rx_cache, (const void *)usart2_RxBuf, Size);
            s_uart_rx_len = Size;
            flag_usart_receive_cmd = true;
        }
        /* 重新使能不定长接收 */
        HAL_UARTEx_ReceiveToIdle_IT(&huart2, (uint8_t *)usart2_RxBuf, RX_MAXLEN);
    }
}

/*============================================================================
 * 便捷指令调用接口
 *============================================================================*/

void lha7220_cmd_send_preset_ch1(void)
{
    lha7220_cmd_parse_and_execute(s_cmd_preset_ch1, sizeof(s_cmd_preset_ch1));
}

void lha7220_cmd_send_preset_ch2(void)
{
    lha7220_cmd_parse_and_execute(s_cmd_preset_ch2, sizeof(s_cmd_preset_ch2));
}

/*============================================================================
 * 例程初始化与主循环
 *============================================================================*/

void lha7220_cmd_init(void)
{
    memset(&g_state, 0, sizeof(g_state));
    g_state.vref_mv = LHA7220_CMD_INT_VREF_MV;
    g_state.gain = 1;
    snprintf(g_state.channel_name, sizeof(g_state.channel_name), "Unconfigured");

    printf("\r\n======================================================================\r\n");
    printf("   LHA7220 动态指令解析与通道切换测试系统 (LHA7220-CMD)\r\n");
    printf("----------------------------------------------------------------------\r\n");
    printf(" 说明：\r\n");
    printf(" 1. 寄存器参数非死代码，完全由收到的 HEX 指令流动态解析与控制！\r\n");
    printf(" 2. 支持通过串口 USART2 发送任意十六进制指令序列直接控制芯片。\r\n");
    printf(" 3. 初始自动执行指令流 1: [06 40 06 44 24 48 10 4C 00 08] (AIN0-AIN1, 内部 2.048V 参考)\r\n");
    printf(" 4. 处于不间断的连续转换状态，直到收到下一条切换指令！\r\n");
    printf(" 5. 数据准备引脚: STM32H750 PA1 (DRDYN 低电平有效)\r\n");
    printf("======================================================================\r\n\r\n");

    /* 启动串口接收中断 */
    HAL_UARTEx_ReceiveToIdle_IT(&huart2, (uint8_t *)usart2_RxBuf, RX_MAXLEN);

    /* 首次加载并执行第一组指令 */
    lha7220_cmd_parse_and_execute(s_cmd_preset_ch1, sizeof(s_cmd_preset_ch1));
}

void lha7220_cmd_on_drdy_irq(void)
{
    s_drdy_irq_flag = 1;
}

const lha7220_cmd_device_state_t* lha7220_cmd_get_state(void)
{
    return &g_state;
}

void lha7220_cmd_loop(void)
{
    static uint32_t s_sample_count = 0;

    /*------------------------------------------------------------------------
     * 1. 优先检查是否从串口接收到了外部上位机下发的新指令流
     *------------------------------------------------------------------------*/
    if (flag_usart_receive_cmd) {
        flag_usart_receive_cmd = false;
        printf("\r\n[EVENT] External UART Command Packet Received! (%u Bytes)\r\n", s_uart_rx_len);
        lha7220_cmd_parse_and_execute(s_uart_rx_cache, s_uart_rx_len);
        s_sample_count = 0; /* 收到新指令切换后，重置采样计数器 */
    }

    /*------------------------------------------------------------------------
     * 2. 检查是否有数据就绪 (监听 H750 PA1 - DRDYN 引脚)
     *------------------------------------------------------------------------*/
    if (g_state.is_running) {
        uint8_t data_ready = 0;

        /* 检测 PA1 是否为有效低电平，或是否触发了外部中断 */
        if (HAL_GPIO_ReadPin(DRDYN_GPIO_Port, DRDYN_Pin) == GPIO_PIN_RESET) {
            data_ready = 1;
        } else if (s_drdy_irq_flag) {
            data_ready = 1;
        }

        if (data_ready) {
            s_drdy_irq_flag = 0;

            /* 读取 24 位转换数据 */
            lha7220_cmd_sample_result_t res;
            read_conversion_data(&res);

            s_sample_count++;
            printf("[DATA #%04lu] %-18s | Raw: 0x%06lX | Signed: %9ld | Volt: %8.4f mV | Gain=%ux\r\n",
                   (unsigned long)s_sample_count,
                   g_state.channel_name,
                   (unsigned long)res.raw_code,
                   (long)res.signed_code,
                   res.voltage_mv,
                   g_state.gain);

            /* 单次模式则停止运行，连续模式下 ADC 自动进行下一次采样，持续不间断 */
            if (!g_state.is_continuous) {
                g_state.is_running = 0;
                printf("[STATUS] Single-shot conversion finished.\r\n");
            }
        }
    }
}
