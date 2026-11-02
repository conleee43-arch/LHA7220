/**
 * @file LHA7220-CMD.h
 * @brief LHA7220 动态指令流解析与通道切换测试用例
 * @details
 *   支持接收动态指令流（通过串口 UART2 接收或软件指令流传入），
 *   完全根据接收到的指令动态配置 CONFIG0~CONFIG3 并控制芯片运行：
 *   - 指令解析器自动识别：
 *     * 0x06 (RESET): 执行软复位并延时 300us
 *     * 0x40~0x4F (WREG): 动态写入 CONFIG0~CONFIG3 寄存器
 *     * 0x08 (START/SYNC): 启动转换
 *     * 0x02 (POWERDOWN): 芯片休眠
 *   - 动态提取：输入通道 (MUX)、增益倍数 (GAIN)、采样速率 (DR)、转换模式 (单次/连续)
 *   - 在连续转换运行期间，一旦收到新指令流，立即重新按照新指令重新配置并切换！
 */

#ifndef __LHA7220_CMD_H__
#define __LHA7220_CMD_H__

#include "main.h"
#include "stdio.h"
#include "stdbool.h"
#include "lha7220.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 默认参数宏定义
 *============================================================================*/
/* 外部基准电压 (mV) - 默认外部参考源 2500mV (可根据硬件实际供电/基准调整) */
#ifndef LHA7220_CMD_EXT_VREF_MV
#define LHA7220_CMD_EXT_VREF_MV     2500.0f
#endif

/* 内部基准电压 (mV) */
#define LHA7220_CMD_INT_VREF_MV     2048.0f

/* 单次等待 DRDYN 超时时间 (ms) */
#define LHA7220_CMD_TIMEOUT_MS      100

/*============================================================================
 * 芯片运行状态与配置跟踪结构体
 *============================================================================*/
typedef struct {
    uint8_t  config[4];         /* 当前 CONFIG0 ~ CONFIG3 寄存器的实际值 */
    uint8_t  mux;               /* 输入通道 MUX (0~15) */
    uint8_t  gain;              /* 增益倍数 (1, 2, 4, 8, 16, 32, 64, 128) */
    uint8_t  dr_idx;            /* 采样率索引 */
    uint8_t  is_continuous;     /* 1: 连续转换模式 (CM=1); 0: 单次转换模式 (CM=0) */
    uint8_t  is_running;        /* 1: 当前正在转换运行中; 0: 空闲或休眠 */
    float    vref_mv;           /* 当前使用的参考电压 (mV) */
    char     channel_name[24];  /* 当前通道文本描述 (如 "AIN0-AIN1", "AIN2-AIN3") */
} lha7220_cmd_device_state_t;

/* 采集结果结构体 */
typedef struct {
    uint32_t raw_code;          /* 24 位无符号原始码值 */
    int32_t  signed_code;       /* 符号扩展后的 32 位有符号整型 */
    float    voltage_mv;        /* 计算得到的电压 (mV) */
} lha7220_cmd_sample_result_t;

/*============================================================================
 * 外部接口函数声明
 *============================================================================*/

/**
 * @brief 测试例程初始化 (开启串口接收中断、打印说明、初始注入第1组指令)
 */
void lha7220_cmd_init(void);

/**
 * @brief 测试例程主循环 (监听串口指令、检测 PA1 DRDYN、读取并输出转换数据)
 */
void lha7220_cmd_loop(void);

/**
 * @brief DRDYN 硬件外部中断回调接口 (PA1 下降沿)
 */
void lha7220_cmd_on_drdy_irq(void);

/**
 * @brief 动态指令流解析与执行引擎
 * @param cmd_stream: 待执行的 HEX 指令字节流
 * @param len: 指令流长度
 * @retval uint8_t: 1 成功解析并执行; 0 失败或指令不完整
 */
uint8_t lha7220_cmd_parse_and_execute(const uint8_t *cmd_stream, uint16_t len);

/**
 * @brief 软件直接触发切换为通道 1 指令流 [0x06, 0x40, 0x06, 0x44, 0x24, 0x48, 0x10, 0x4C, 0x00, 0x08] (内部 2.048V 参考)
 */
void lha7220_cmd_send_preset_ch1(void);

/**
 * @brief 软件直接触发切换为通道 2 指令流 [0x06, 0x40, 0x56, 0x44, 0x24, 0x48, 0x10, 0x4C, 0x00, 0x08] (内部 2.048V 参考)
 */
void lha7220_cmd_send_preset_ch2(void);

/**
 * @brief 获取当前设备状态指针 (供外部查询当前配置)
 */
const lha7220_cmd_device_state_t* lha7220_cmd_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* __LHA7220_CMD_H__ */
