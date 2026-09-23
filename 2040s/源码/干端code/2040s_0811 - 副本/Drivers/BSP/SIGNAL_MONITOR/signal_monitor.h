#ifndef __SIGNAL_MONITOR_H
#define __SIGNAL_MONITOR_H

#include "./SYSTEM/sys/sys.h"
#include "FreeRTOS.h"
#include "task.h"

/* 串口 DMA 接收缓冲区大小 */
#define SIGNAL_MONITOR_DMA_RX_SIZE       512U

/* 信号监测任务周期 */
#define SIGNAL_MONITOR_TASK_PERIOD_MS    5U

/* 有效数据超时时间，超过 1s 没收到有效数据就灭灯 */
#define SIGNAL_MONITOR_TIMEOUT_MS        1000U

/* LED 点亮亮度 */
#define SIGNAL_MONITOR_LED_BRIGHT        0x33U

/* 需要监测的业务通道 */
typedef enum
{
    SIGNAL_MONITOR_HEADING = 0,
    SIGNAL_MONITOR_MOTION,
    SIGNAL_MONITOR_GNSS,
    SIGNAL_MONITOR_SVS,
    SIGNAL_MONITOR_MAX
} signal_monitor_channel_t;

/* 初始化信号监测串口 DMA 和内部状态
 * 参数顺序：
 * heading_baud -> HEADING / USART1_RX / PB7
 * motion_baud  -> MOTION  / USART2_RX / PA3
 * gnss_baud    -> GNSS    / USART3_RX / PD9
 * svs_baud     -> SVS     / UART5_RX  / PD2
 */
void signal_monitor_init(uint32_t heading_baud,
                         uint32_t motion_baud,
                         uint32_t gnss_baud,
                         uint32_t svs_baud);

/* 创建信号监测 FreeRTOS 任务 */
BaseType_t signal_monitor_start(void);

/* 信号监测任务函数，如果不使用 signal_monitor_start()，也可以自己 xTaskCreate() */
void signal_monitor_task(void *pvParameters);

/* 使能或关闭某一路串口信号监测 */
void signal_monitor_enable(signal_monitor_channel_t channel, uint8_t enable);

/* 使能或关闭 SYNC 脉冲监测 */
void signal_monitor_sync_enable(uint8_t enable);

/* 串口 IDLE 中断处理函数；如果已有对应 IRQHandler，就在里面调用这个函数 */
void signal_monitor_uart_idle_irq(USART_TypeDef *uart);

/* SVS 湿端输入显示模式
 * enable = 1：SVS 由湿端输入，干端不监测 SVS，SVS 灯常亮蓝色
 * enable = 0：SVS 恢复干端输入监测，等待有效数据时显示红色
 */
void signal_monitor_svs_wet_input_set(uint8_t enable);


void def_work_mode_init(void);

#endif
