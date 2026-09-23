/**
 ****************************************************************************************************
 * @file        usart.h
 * @author                  ()
 * @version     V1.0
 * @date        2021-10-14
 * @brief                     (          1)      printf
 * @license     Copyright (c) 2020-2032,                           
 ****************************************************************************************************
 * @attention
 *
 *         :         STM32F407      
 *         :www.yuanzige.com
 *         :www.openedv.com
 *         :www..com
 *        :openedv.taobao.com
 *
 *         
 * V1.0 20211014
 *           
 *
 ****************************************************************************************************
 */

#ifndef _USART_H
#define _USART_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"

/*******************************************************************************************************/
/*                   
 *          USART1  .
 *     :           12        ,        USART1~UART7            .
 */

#define USART_TX_GPIO_PORT              GPIOC
#define USART_TX_GPIO_PIN               GPIO_PIN_6
#define USART_TX_GPIO_AF                GPIO_AF8_USART6
#define USART_TX_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /*                  */

#define USART_RX_GPIO_PORT              GPIOC
#define USART_RX_GPIO_PIN               GPIO_PIN_7
#define USART_RX_GPIO_AF                GPIO_AF8_USART6
#define USART_RX_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /*                  */

#define USART_UX                        USART6
#define USART_UX_IRQn                   USART6_IRQn
#define USART_UX_IRQHandler             USART6_IRQHandler
#define USART_UX_CLK_ENABLE()           do{ __HAL_RCC_USART6_CLK_ENABLE(); }while(0)  /* USART6          */

/*******************************************************************************************************/

#define USART_REC_LEN   200                     /*                  200 */
#define USART_EN_RX     1                       /*       1  /      0      1     */
#define RXBUFFERSIZE    1                       /*         */

extern UART_HandleTypeDef g_uart1_handle;       /* UART    */

extern uint8_t  g_usart_rx_buf[USART_REC_LEN];  /*         ,   USART_REC_LEN      .               */
extern uint16_t g_usart_rx_sta;                 /*             */
extern uint8_t g_rx_buffer[RXBUFFERSIZE];       /* HAL  USART    Buffer */


void usart_init(uint32_t baudrate);             /*                */

#endif







