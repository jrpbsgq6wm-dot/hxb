   /**
 ****************************************************************************************************
 * @file        PPS.c
 * @author            ()
 * @version     V1.0
 * @date        2026-07-18
 * @brief       PPS                   PD6   
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 *             :
 *   PD6 - PPS                                     
 *                               USART6        "PPS OK"
 *
 ****************************************************************************************************
 */

#include "./BSP/PPS/PPS.h"

/**
 * @brief              USART6                                                 
 * @param       str:     null                   
 * @retval         
 * @note                                                           
 */
static void pps_uart6_send_str(const char *str)
{
    while (*str)
    {
        while ((USART6->SR & USART_SR_TXE) == 0);   /*                          */
        USART6->DR = (uint8_t)(*str++);
    }
}

/**
 * @brief       EXTI9_5                      PD6          
 * @param          
 * @retval         
 * @note                                          USART6        "PPS OK"
 */
void EXTI9_5_IRQHandler(void)
{
    /*        PD6              */
    if (__HAL_GPIO_EXTI_GET_IT(PPS_GPIO_PIN) != RESET)
    {
        /*                    */
        __HAL_GPIO_EXTI_CLEAR_IT(PPS_GPIO_PIN);

        /*        USART6        PPS OK */
        pps_uart6_send_str("PPS OK\r\n");
    }
}

/**
 * @brief                 PPS(PD6)
 * @param          
 * @retval         
 * @note                                                                 
 */
void pps_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    PPS_GPIO_CLK_ENABLE();                                          /* GPIOD              */

    gpio_init_struct.Pin = PPS_GPIO_PIN;                            /* PD6 */
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;                    /*                                */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                          /*                          */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                  /*        */
    HAL_GPIO_Init(PPS_GPIO_PORT, &gpio_init_struct);                /*           PD6 */

    /*        EXTI9_5                            PD6        EXTI     6          EXTI9_5                 */
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}
