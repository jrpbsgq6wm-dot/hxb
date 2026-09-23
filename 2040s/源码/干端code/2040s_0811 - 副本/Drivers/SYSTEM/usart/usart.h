#ifndef _USART_H
#define _USART_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"

/******************************************************************************************************/
/* 调试打印串口配置
 *
 * 说明：
 * 1. 本文件封装工程中的调试串口。
 * 2. printf 最终通过 fputc() 输出到 USART_UX。
 * 3. 当前 USART_UX 配置为 USART6：
 *      TX: PC6, AF8
 *      RX: PC7, AF8
 * 4. 如需更换调试串口，只修改下面这一组宏即可。
 */
/******************************************************************************************************/

#define USART_TX_GPIO_PORT              GPIOC
#define USART_TX_GPIO_PIN               GPIO_PIN_6
#define USART_TX_GPIO_AF                GPIO_AF8_USART6
#define USART_TX_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define USART_RX_GPIO_PORT              GPIOC
#define USART_RX_GPIO_PIN               GPIO_PIN_7
#define USART_RX_GPIO_AF                GPIO_AF8_USART6
#define USART_RX_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define USART_UX                        USART6
#define USART_UX_IRQn                   USART6_IRQn
#define USART_UX_IRQHandler             USART6_IRQHandler
#define USART_UX_CLK_ENABLE()           do{ __HAL_RCC_USART6_CLK_ENABLE(); }while(0)

/******************************************************************************************************/
/* 接收缓冲配置
 *
 * USART_REC_LEN:
 *      调试串口接收缓存长度。
 *
 * USART_EN_RX:
 *      1 = 使能接收中断
 *      0 = 不使能接收
 *
 * RXBUFFERSIZE:
 *      HAL_UART_Receive_IT() 单次接收字节数。
 *      当前为 1，表示每次接收 1 字节后进入回调。
 */
/******************************************************************************************************/

#define USART_REC_LEN                   200
#define USART_EN_RX                     1
#define RXBUFFERSIZE                    1

extern UART_HandleTypeDef g_uart1_handle;

extern uint8_t  g_usart_rx_buf[USART_REC_LEN];
extern uint16_t g_usart_rx_sta;
extern uint8_t  g_rx_buffer[RXBUFFERSIZE];

void usart_init(uint32_t baudrate);

#endif


