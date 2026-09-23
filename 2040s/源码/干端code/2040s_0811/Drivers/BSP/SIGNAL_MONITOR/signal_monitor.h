#ifndef __SIGNAL_MONITOR_H
#define __SIGNAL_MONITOR_H

#include "./SYSTEM/sys/sys.h"
#include "FreeRTOS.h"
#include "task.h"

/* 每路串口单帧 DMA 接收缓冲区长度，同时也是单帧最大长度。 */
#define SIGNAL_MONITOR_DMA_RX_SIZE             512U

/* 没有串口事件时，任务用于检查 LED 超时的最长休眠时间。 */
#define SIGNAL_MONITOR_TIMEOUT_CHECK_MS        50U

/* 收到有效协议后，超过该时间未再次收到则信号失效。 */
#define SIGNAL_MONITOR_TIMEOUT_MS              1000U

/* LP5012 RGB LED 的显示亮度。 */
#define SIGNAL_MONITOR_LED_BRIGHT              0x33U

typedef enum
{
    SIGNAL_MONITOR_HEADING = 0,
    SIGNAL_MONITOR_MOTION,
    SIGNAL_MONITOR_GNSS,
    SIGNAL_MONITOR_SVS,
    SIGNAL_MONITOR_MAX
} signal_monitor_channel_t;

typedef struct
{
    uint8_t heading_status;
    uint8_t motion_status;
    uint8_t gnss_status;
    uint8_t svs_status;
} signal_monitor_status_t;

typedef enum
{
    SIGNAL_MONITOR_SYNC_UNCONFIGURED = 0,
    SIGNAL_MONITOR_SYNC_INPUT = 1,
    SIGNAL_MONITOR_SYNC_OUTPUT = 2
} signal_monitor_sync_mode_t;

/**
 * @brief  初始化四路串口的 HAL UART、HAL DMA 和信号监测状态。
 *
 * 参数对应关系：
 * heading_baud: USART1 / PB7 / HEADING
 * motion_baud : USART2 / PA3 / MOTION
 * gnss_baud   : USART3 / PD9 / GNSS
 * svs_baud    : UART5  / PD2 / SVS
 */
void signal_monitor_init(uint32_t heading_baud,
                         uint32_t motion_baud,
                         uint32_t gnss_baud,
                         uint32_t svs_baud);

/* 创建信号监测任务；重复调用不会重复创建。 */
BaseType_t signal_monitor_start(void);

/* 信号监测任务入口，通常由 signal_monitor_start() 创建。 */
void signal_monitor_task(void *pvParameters);

/* 启用或关闭指定的串口信号监测通道。 */
void signal_monitor_enable(signal_monitor_channel_t channel, uint8_t enable);

/* 设置 SVS 是否由湿端输入：1 为湿端蓝灯模式，0 为干端 UART5 监测模式。 */
void signal_monitor_svs_wet_input_set(uint8_t enable);
uint8_t signal_monitor_svs_wet_input_is_enabled(void);

void signal_monitor_get_status(signal_monitor_status_t *status);
void signal_monitor_sync_mode_set(signal_monitor_sync_mode_t mode);
signal_monitor_sync_mode_t signal_monitor_sync_mode_get(void);

/* 设置项目默认的信号输入模式。 */
void def_work_mode_init(void);

#endif
