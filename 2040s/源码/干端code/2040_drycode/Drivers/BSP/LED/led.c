   /**
 ****************************************************************************************************
 * @file        led.c
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

#include "./BSP/LED/led.h"


/* LED1   LED2   LED3                                               */
uint8_t LED1 = 0;   /* PC8                   */
uint8_t LED2 = 0;   /* PD15                   */
uint8_t LED3 = 0;   /* PD10                   */


/**
 * @brief                 LED        IO    
 * @param          
 * @retval         
 */
void led_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    /*           LED1 (PC8) */
    LED1_GPIO_CLK_ENABLE();                                 /* GPIOC              */

    gpio_init_struct.Pin = LED1_GPIO_PIN;                   /* LED1        = PC8 */
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /*              */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /*        */
    HAL_GPIO_Init(LED1_GPIO_PORT, &gpio_init_struct);       /*           LED1        */

    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_RESET);  /*                   LED1        */
    LED1 = 0;

    /*           LED2 (PD15) */
    LED2_GPIO_CLK_ENABLE();                                 /* GPIOD                                            */

    gpio_init_struct.Pin = LED2_GPIO_PIN;                   /* LED2        = PD15 */
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /*              */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /*        */
    HAL_GPIO_Init(LED2_GPIO_PORT, &gpio_init_struct);       /*           LED2        */

    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_GPIO_PIN, GPIO_PIN_RESET);  /*                   LED2        */
    LED2 = 0;

    /*           LED3 (PD10) */
    LED3_GPIO_CLK_ENABLE();                                 /* GPIOD                                            */

    gpio_init_struct.Pin = LED3_GPIO_PIN;                   /* LED3        = PD10 */
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /*              */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /*        */
    HAL_GPIO_Init(LED3_GPIO_PORT, &gpio_init_struct);       /*           LED3        */

    HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_GPIO_PIN, GPIO_PIN_RESET);  /*                   LED3        */
    LED3 = 0;
}


/**
 * @brief                       LED1~LED3                    GPIO
 * @param          
 * @retval         
 * @note                  LED1/LED2/LED3                                              
 *                        LED1 = 1; LED2 = 0; led_sync();
 */
void led_sync(void)
{
    /*        LED1                 PC8          1=      (   )   0=      (   ) */
    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, LED1 ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /*        LED2                 PD15        */
    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_GPIO_PIN, LED2 ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /*        LED3                 PD10        */
    HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_GPIO_PIN, LED3 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
