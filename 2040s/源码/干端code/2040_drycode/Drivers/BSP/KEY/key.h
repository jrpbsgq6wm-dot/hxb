   /**
 ****************************************************************************************************
 * @file        key.h
 * @author            ()
 * @version     V1.0
 * @date        2026-07-16
 * @brief                      MCU_KEY   
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 *             :           F407         
 *             :www.yuanzige.com
 *             :www.openedv.com
 *             :www..com
 *             :openedv.taobao.com
 *
 ****************************************************************************************************
 */
#ifndef __KEY_H
#define __KEY_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* MCU_KEY          PC0                                                                */

#define MCU_KEY_GPIO_PORT                   GPIOC
#define MCU_KEY_GPIO_PIN                    GPIO_PIN_0
#define MCU_KEY_GPIO_CLK_ENABLE()           do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)
#define MCU_KEY_READ()                      HAL_GPIO_ReadPin(MCU_KEY_GPIO_PORT, MCU_KEY_GPIO_PIN)

/******************************************************************************************/

void mcu_key_init(void);                /* MCU_KEY(PC0)                                         */
void mcu_key_scan(void);                /* MCU_KEY                                                                 */
void mcu_key_tick_handler(void);        /* SysTick              SysTick_Handler                                */

#endif
