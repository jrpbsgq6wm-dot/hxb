   /**
 ****************************************************************************************************
 * @file        gtim.c
 * @author                  ()
 * @version     V1.0
 * @date        2021-10-15
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
 *
 ****************************************************************************************************
 */

#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"

/******************************************************************************************/

/*                    */
TIM_HandleTypeDef g_timx_handle; /*       x    */

/******************************************************************************************/
/* HAL                  */



/**
 * @brief                             
 * @param        htim:             
 * @note                                          
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
void gtim_timx_int_init(uint16_t arr, uint16_t psc)
{
    GTIM_TIMX_INT_CLK_ENABLE(); /*     TIMx     */

    g_timx_handle.Instance = GTIM_TIMX_INT;                     /*           x */
    g_timx_handle.Init.Prescaler = psc;                         /*      */
    g_timx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;        /*            */
    g_timx_handle.Init.Period = arr;                            /*            */
    g_timx_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;  /*              */
    HAL_TIM_Base_Init(&g_timx_handle);

    HAL_NVIC_SetPriority(GTIM_TIMX_INT_IRQn, 1, 3);             /*                           1          3 */
    HAL_NVIC_EnableIRQ(GTIM_TIMX_INT_IRQn);                     /*     ITMx     */

    HAL_TIM_Base_Start_IT(&g_timx_handle);                      /*           x        x         */
}

/**
 * @brief                       
 * @param         
 * @retval        
 */
void GTIM_TIMX_INT_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_handle);
}

