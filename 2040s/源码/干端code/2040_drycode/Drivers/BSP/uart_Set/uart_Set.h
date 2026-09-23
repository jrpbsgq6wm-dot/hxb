   /**
 ****************************************************************************************************
 * @file        uart_Set.h
 * @author            ()
 * @version     V1.0
 * @date        2026-07-17
 * @brief       UART                            
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 *             :
 *   PB8  - UART1_232/422_SW                                
 *   PB1  - UART2_232/422_SW                                
 *   PD8  - UART3_232/422_SW                                
 *   PD0  - PPS_SW                                          
 *   PD4  - UART5_232_485_SW                                 
 *   PD3  - UART5_232/485_RE_DE                             
 *   PD1  - UART5_485_SD_RE_DE                              
 *   PD12 - UART5/6_TTL_SW                                  
 *
 ****************************************************************************************************
 */

#ifndef __UART_SET_H
#define __UART_SET_H

#include "./SYSTEM/sys/sys.h"

/*              */
#define UART1_SW_GPIO_PORT              GPIOB
#define UART1_SW_GPIO_PIN               GPIO_PIN_8
#define UART1_SW_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define UART2_SW_GPIO_PORT              GPIOB
#define UART2_SW_GPIO_PIN               GPIO_PIN_1
#define UART2_SW_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define UART3_SW_GPIO_PORT              GPIOD
#define UART3_SW_GPIO_PIN               GPIO_PIN_8
#define UART3_SW_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

#define PPS_SW_GPIO_PORT                GPIOD
#define PPS_SW_GPIO_PIN                 GPIO_PIN_0
#define PPS_SW_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

#define UART5_232_485_SW_GPIO_PORT           GPIOD
#define UART5_232_485_SW_GPIO_PIN            GPIO_PIN_4
#define UART5_232_485_SW_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

#define UART5_RE_DE_GPIO_PORT           GPIOD
#define UART5_RE_DE_GPIO_PIN            GPIO_PIN_3
#define UART5_RE_DE_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

#define UART5_485_SD_RE_DE_GPIO_PORT           GPIOD
#define UART5_485_SD_RE_DE_GPIO_PIN            GPIO_PIN_1
#define UART5_485_SD_RE_DE_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

#define UART5_6_TTL_SW_GPIO_PORT         GPIOD
#define UART5_6_TTL_SW_GPIO_PIN          GPIO_PIN_12
#define UART5_6_TTL_SW_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

/*           */
#define UART1_SW_SET(val)               HAL_GPIO_WritePin(UART1_SW_GPIO_PORT, UART1_SW_GPIO_PIN, val)
#define UART2_SW_SET(val)               HAL_GPIO_WritePin(UART2_SW_GPIO_PORT, UART2_SW_GPIO_PIN, val)
#define UART3_SW_SET(val)               HAL_GPIO_WritePin(UART3_SW_GPIO_PORT, UART3_SW_GPIO_PIN, val)
#define PPS_SW_SET(val)                 HAL_GPIO_WritePin(PPS_SW_GPIO_PORT, PPS_SW_GPIO_PIN, val)
#define UART5_232_485_SW_SET(val)     HAL_GPIO_WritePin(UART5_232_485_SW_GPIO_PORT, UART5_232_485_SW_GPIO_PIN, val)
#define UART5_RE_DE_SET(val)            HAL_GPIO_WritePin(UART5_RE_DE_GPIO_PORT, UART5_RE_DE_GPIO_PIN, val)
#define UART5_485_SD_RE_DE_SET(val)     HAL_GPIO_WritePin(UART5_485_SD_RE_DE_GPIO_PORT, UART5_485_SD_RE_DE_GPIO_PIN, val)
#define UART5_6_TTL_SW_SET(val)          HAL_GPIO_WritePin(UART5_6_TTL_SW_GPIO_PORT, UART5_6_TTL_SW_GPIO_PIN, val)

/*              */
void uart_switch_init(void);
void uart_switch_apply_config(void);        /*        UART_STAR                                      */
void uart_switch_rx_process(uint8_t byte);  /* USART6                                      */

/* UART_STAR                10      : AA DD + 7       + DD    */
extern uint8_t UART_STAR[10];

#endif
