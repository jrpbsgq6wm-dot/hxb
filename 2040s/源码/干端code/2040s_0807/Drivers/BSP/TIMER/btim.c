   /**
 ****************************************************************************************************
 * @file        btim.c
 * @author                  ()
 * @version     V1.1
 * @date        2021-11-29
 * @brief                          
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
 * V1.0 20211015
 *           
 * V1.1 20211129
 *           3                    
 ****************************************************************************************************
 */

#include "./BSP/TIMER/btim.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led.h"


extern uint32_t lwip_localtime;         /* lwip             ,    :ms */

TIM_HandleTypeDef g_tim3_handler;       /*               */
TIM_HandleTypeDef g_tim6_handler;       /*               */

/**
 * @brief                 TIMX          
 * @param         
 * @retval        
 */
void BTIM_TIM3_INT_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_tim3_handler);  /*                */
}

/* TIM6 handler moved to TMP175.c */

/**
 * @brief                                     
 * @param         
 * @retval        
 */
/* HAL_TIM_PeriodElapsedCallback moved to TMP175.c (unified timer callback handling) */

/**
 * @brief                 TIMX                  
 * @note
 *                                  APB1,  PPRE1    2          
 *                                APB1      2  ,   APB1  42M,                = 84Mhz
 *                                  : Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=              ,    :Mhz
 *
 * @param       arr:             
 * @param       psc:             
 * @retval        
 */
void btim_tim3_int_init(uint16_t arr, uint16_t psc)
{
    g_tim3_handler.Instance = BTIM_TIM3_INT;                      /*           X */
    g_tim3_handler.Init.Prescaler = psc;                          /*               */
    g_tim3_handler.Init.CounterMode = TIM_COUNTERMODE_UP;         /*            */
    g_tim3_handler.Init.Period = arr;                             /*            */
    g_tim3_handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;   /*              */
    HAL_TIM_Base_Init(&g_tim3_handler);

    HAL_TIM_Base_Start_IT(&g_tim3_handler);                       /*               x               TIM_IT_UPDATE */
}

/**
 * @brief                 TIMX                  
 * @note
 *                                  APB1,  PPRE1    2          
 *                                APB1      2  ,   APB1  36M,                = 72Mhz
 *                                  : Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=              ,    :Mhz
 *
 * @param       arr:             
 * @param       psc:             
 * @retval        
 */
void btim_tim6_int_init(uint16_t arr, uint16_t psc)
{
    g_tim6_handler.Instance = BTIM_TIM6_INT;                      /*           X */
    g_tim6_handler.Init.Prescaler = psc;                          /*               */
    g_tim6_handler.Init.CounterMode = TIM_COUNTERMODE_UP;         /*            */
    g_tim6_handler.Init.Period = arr;                             /*            */
    g_tim6_handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;   /*              */
    HAL_TIM_Base_Init(&g_tim6_handler);

    HAL_TIM_Base_Start_IT(&g_tim6_handler);                       /*               x               TIM_IT_UPDATE */
}

/**
 * @brief                                               
                          HAL_TIM_Base_Init()        
 * @param         
 * @retval        
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == BTIM_TIM3_INT)
    {
        BTIM_TIM3_INT_CLK_ENABLE();                     /*     TIM     */
        HAL_NVIC_SetPriority(BTIM_TIM3_INT_IRQn, 1, 3); /*     1          3    2 */
        HAL_NVIC_EnableIRQ(BTIM_TIM3_INT_IRQn);         /*     ITM3     */
    }
    if (htim->Instance == BTIM_TIM6_INT)
    {
        BTIM_TIM6_INT_CLK_ENABLE();                     /*     TIM     */
        HAL_NVIC_SetPriority(BTIM_TIM6_INT_IRQn, 0, 3); /*     1          3    2 */
        HAL_NVIC_EnableIRQ(BTIM_TIM6_INT_IRQn);         /*     ITM3     */
    }
}

