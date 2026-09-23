   /**
 ****************************************************************************************************
 * @file        SYNC.c
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

#include "./BSP/SYNC/SYNC.h"

/**
 * @brief              USART6                                                 
 * @param       str:     null                   
 * @retval         
 * @note                                                           
 */
static void sync_uart6_send_str(const char *str)
{
    while (*str)
    {
        while ((USART6->SR & USART_SR_TXE) == 0);   /*                          */
        USART6->DR = (uint8_t)(*str++);
    }
}

/**
 * @brief       EXTI15_10                      PD11 / PD13          
 * @param          
 * @retval         
 * @note               PD11     PD13                                           
 */
void EXTI15_10_IRQHandler(void)
{
    /*        PD11 (SYNC OUTPUT)              */
    if (__HAL_GPIO_EXTI_GET_IT(SYNC_OUTPUT_GPIO_PIN) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(SYNC_OUTPUT_GPIO_PIN);
        sync_uart6_send_str("SYNC OUTPUT\r\n");
    }

    /*        PD13 (SYNC INPUT)              */
    if (__HAL_GPIO_EXTI_GET_IT(SYNC_INPUT_GPIO_PIN) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(SYNC_INPUT_GPIO_PIN);
        sync_uart6_send_str("SYNC INPUT\r\n");
    }
}

/**
 * @brief                 SYNC(PD11/PD13)
 * @param          
 * @retval         
 * @note                                                                          
 *              PD11/PD13        EXTI15_10             
 */
void sync_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    SYNC_OUTPUT_GPIO_CLK_ENABLE();                                  /* GPIOD              */
    SYNC_INPUT_GPIO_CLK_ENABLE();                                   /* GPIOD              */

    /*        PD11 - SYNC OUTPUT */
    gpio_init_struct.Pin = SYNC_OUTPUT_GPIO_PIN;                    /* PD11 */
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;                    /*                                */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                          /*                          */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                  /*        */
    HAL_GPIO_Init(SYNC_OUTPUT_GPIO_PORT, &gpio_init_struct);        /*           PD11 */

    /*        PD13 - SYNC INPUT */
    gpio_init_struct.Pin = SYNC_INPUT_GPIO_PIN;                     /* PD13 */
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;                    /*                                */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                          /*                          */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                  /*        */
    HAL_GPIO_Init(SYNC_INPUT_GPIO_PORT, &gpio_init_struct);         /*           PD13 */

    /*        EXTI15_10                            PD11/PD13                    */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
