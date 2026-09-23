   /**
 ****************************************************************************************************
 * @file        SYNC.h
 * @author            ()
 * @version     V1.0
 * @date        2026-07-18
 * @brief       SYNC                      PD11 / PD13   
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 *             :
 *   PD11 - SYNC OUTPUT                                     
 *   PD13 - SYNC INPUT                                      
 *                               USART6                   
 *
 ****************************************************************************************************
 */

#ifndef __SYNC_H
#define __SYNC_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* SYNC OUTPUT          PD11                               */

#define SYNC_OUTPUT_GPIO_PORT                   GPIOD
#define SYNC_OUTPUT_GPIO_PIN                    GPIO_PIN_11
#define SYNC_OUTPUT_GPIO_CLK_ENABLE()           do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

/* SYNC INPUT          PD13                               */

#define SYNC_INPUT_GPIO_PORT                    GPIOD
#define SYNC_INPUT_GPIO_PIN                     GPIO_PIN_13
#define SYNC_INPUT_GPIO_CLK_ENABLE()            do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

/******************************************************************************************/

void sync_init(void);               /* SYNC(PD11/PD13)                                         */

#endif
