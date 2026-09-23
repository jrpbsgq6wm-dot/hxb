#ifndef __UART_SET_H
#define __UART_SET_H

#include "./SYSTEM/sys/sys.h"

/* 串口切换相关 GPIO 定义 */
#define UART1_SW_GPIO_PORT GPIOB
#define UART1_SW_GPIO_PIN GPIO_PIN_8
#define UART1_SW_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

#define UART2_SW_GPIO_PORT GPIOB
#define UART2_SW_GPIO_PIN GPIO_PIN_1
#define UART2_SW_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

#define UART3_SW_GPIO_PORT GPIOD
#define UART3_SW_GPIO_PIN GPIO_PIN_8
#define UART3_SW_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

#define PPS_SW_GPIO_PORT GPIOD
#define PPS_SW_GPIO_PIN GPIO_PIN_0
#define PPS_SW_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

#define UART5_232_485_SW_GPIO_PORT GPIOD
#define UART5_232_485_SW_GPIO_PIN GPIO_PIN_4
#define UART5_232_485_SW_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

#define UART5_RE_DE_GPIO_PORT GPIOD
#define UART5_RE_DE_GPIO_PIN GPIO_PIN_3
#define UART5_RE_DE_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

#define UART5_485_SD_RE_DE_GPIO_PORT GPIOD
#define UART5_485_SD_RE_DE_GPIO_PIN GPIO_PIN_1
#define UART5_485_SD_RE_DE_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

#define UART5_6_TTL_SW_GPIO_PORT GPIOD
#define UART5_6_TTL_SW_GPIO_PIN GPIO_PIN_12
#define UART5_6_TTL_SW_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

/* 控制这些切换引脚的封装宏 */
#define UART1_SW_SET(val) BOARD_UART1_SW_SET(val)
#define UART2_SW_SET(val) BOARD_UART2_SW_SET(val)
#define UART3_SW_SET(val) BOARD_UART3_SW_SET(val)
#define PPS_SW_SET(val) BOARD_PPS_SW_SET(val)
#define UART5_232_485_SW_SET(val) BOARD_UART5_232_485_SW_SET(val)
#define UART5_RE_DE_SET(val) BOARD_UART5_RE_DE_SET(val)
#define UART5_485_SD_RE_DE_SET(val) BOARD_UART5_485_SD_RE_DE_SET(val)
#define UART5_6_TTL_SW_SET(val) BOARD_UART5_6_TTL_SW_SET(val)

/* 串口切换初始化 */
void uart_switch_init(void);

/* 按 UART_STAR 配置实际切换串口模式 */
void uart_switch_apply_config(void);

/* USART6 接收 1 个字节后的解析入口 */
void uart_switch_rx_process(uint8_t byte);

/* 配置帧：AA DD + 7 字节参数 + DD */
extern uint8_t UART_STAR[10];

extern void sync_hw_init(void);


#endif

