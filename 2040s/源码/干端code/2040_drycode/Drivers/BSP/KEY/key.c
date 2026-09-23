   /**
 ****************************************************************************************************
 * @file        key.c
 * @author            ()
 * @version     V1.0
 * @date        2026-07-16
 * @brief                      MCU_KEY = PC0   
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

#include "./BSP/KEY/key.h"
#include "stdio.h"

/*                          */
#define MCU_KEY_DEBOUNCE_MS     10

static volatile uint8_t g_key_pending = 0;      /* EXTI                          */
static volatile uint8_t g_key_event = 0;        /*                                   */
static volatile uint32_t g_key_tick = 0;        /*                       */

/**
 * @brief                 MCU_KEY(PC0)
 * @param          
 * @retval         
 * @note                                                                 
 */
void mcu_key_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    MCU_KEY_GPIO_CLK_ENABLE();                                     /* GPIOC              */

    gpio_init_struct.Pin = MCU_KEY_GPIO_PIN;                       /* PC0 */
    gpio_init_struct.Mode = GPIO_MODE_IT_FALLING;                  /*                                */
    gpio_init_struct.Pull = GPIO_PULLUP;                           /*                          */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                 /*        */
    HAL_GPIO_Init(MCU_KEY_GPIO_PORT, &gpio_init_struct);           /*           PC0 */

    /*        EXTI0                          */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/**
 * @brief       EXTI0                   
 * @param          
 * @retval         
 * @note                                 EXTI                                  
 */
void EXTI0_IRQHandler(void)
{
    /*                    */
    __HAL_GPIO_EXTI_CLEAR_IT(MCU_KEY_GPIO_PIN);

    /*                    EXTI Line0                 IMR                             */
    EXTI->IMR &= ~(MCU_KEY_GPIO_PIN);

    g_key_tick = HAL_GetTick();
    g_key_pending = 1;
}

/**
 * @brief       SysTick                    1ms     SysTick_Handler             
 * @param          
 * @retval         
 * @note                                                                                
 */
void mcu_key_tick_handler(void)
{
    if (g_key_pending == 0)
        return;

    /*                    */
    if ((HAL_GetTick() - g_key_tick) < MCU_KEY_DEBOUNCE_MS)
        return;

    g_key_pending = 0;

    /*                                               */
    if (MCU_KEY_READ() == 0)
    {
        g_key_event = 1;            /*                                      */
    }
    else
    {
        EXTI->IMR |= MCU_KEY_GPIO_PIN;  /*                                      */
    }
}

/**
 * @brief       MCU_KEY                                  
 * @param          
 * @retval         
 * @note                                                                     EXTI       
 */
void mcu_key_scan(void)
{
    if (g_key_event == 0)
        return;

    g_key_event = 0;

    /*              EXTI                         */
    EXTI->IMR |= MCU_KEY_GPIO_PIN;

    printf("mcu_key input\r\n");
}
