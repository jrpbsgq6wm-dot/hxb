   /**
 ****************************************************************************************************
 * @file        led.h
 * @author            ()
 * @version     V1.0
 * @date        2021-10-14
 * @brief       LED             
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
 *             
 * V1.0 20211014
 *                
 * V1.2 20260716
 *        LED0(PF9)        LED             
 *
 * V1.1 20260715
 *        LED1(PC8)   LED2(PD15)   LED3(PD10)                   
 *
 ****************************************************************************************************
 */
#ifndef __LED_H
#define __LED_H

#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/* LED          LED1=PC8, LED2=PD15, LED3=PD10 */

#define LED1_GPIO_PORT                  GPIOC
#define LED1_GPIO_PIN                   GPIO_PIN_8
#define LED1_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)             /* PC             */

#define LED2_GPIO_PORT                  GPIOD
#define LED2_GPIO_PIN                   GPIO_PIN_15
#define LED2_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)             /* PD             */

#define LED3_GPIO_PORT                  GPIOD
#define LED3_GPIO_PIN                   GPIO_PIN_10
#define LED3_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)             /* PD             */

/******************************************************************************************/

/* LED1   LED2   LED3                   
 *           = 1                 LED   = 0          
 *        led_sync()                          GPIO */
extern uint8_t LED1;
extern uint8_t LED2;
extern uint8_t LED3;

/******************************************************************************************/
/*                    */
void led_init(void);        /* LED                 GPIO                             */
void led_sync(void);        /*                 LED1~LED3                    GPIO */

#endif
