/**
 ****************************************************************************************************
 * @file        usmart_port.c
 * @author            ()
 * @version     V3.5
 * @date        2020-12-20
 * @brief       USMART             
 *
 *                                   ,                  USMART                     
 *                 :USMART_ENTIMX_SCAN == 0   ,               : usmart_get_input_string      .
 *                 :USMART_ENTIMX_SCAN == 1   ,               4         :
 *              usmart_timx_reset_time
 *              usmart_timx_get_time
 *              usmart_timx_init
 *              USMART_TIMX_IRQHandler
 *
 * @license     Copyright (c) 2020-2032,                                        
 ****************************************************************************************************
 * @attention
 *
 *             :www.yuanzige.com
 *             :www.openedv.com
 *             :www.alientek.com
 *             :openedv.taobao.com
 *
 *             
 *
 * V3.4                                 USMART               :readme.txt
 *
 * V3.4 20200324
 * 1,       usmart_port.c   usmart_port.h,            USMART         ,            
 * 2,                            : uint8_t, uint16_t, uint32_t
 * 3,       usmart_reset_runtime   usmart_timx_reset_time
 * 4,       usmart_get_runtime   usmart_timx_get_time
 * 5,       usmart_scan                  ,         usmart_get_input_string               
 * 6,       printf         USMART_PRINTF         
 * 7,                               ,                     ,            
 *
 * V3.5 20201220
 * 1                              AC6         
 *
 ****************************************************************************************************
 */

#include "./USMART/usmart.h"
#include "./USMART/usmart_port.h"

TIM_HandleTypeDef g_timx_usmart_handle;      /*                 */

/**
 * @brief                            (         )
 *   @note      USMART                                                                           
 * @param          
 * @retval
 *   @arg       0,                       
 *   @arg             ,                  (         0)
 */
char *usmart_get_input_string(void)
{
    uint8_t len;
    char *pbuf = 0;

    if (g_usart_rx_sta & 0x8000)        /*                       */
    {
        len = g_usart_rx_sta & 0x3fff;  /*                                      */
        g_usart_rx_buf[len] = '\0';     /*                         . */
        pbuf = (char*)g_usart_rx_buf;
        g_usart_rx_sta = 0;             /*                       */
    }

    return pbuf;
}

/*                               ,                             */
#if USMART_ENTIMX_SCAN == 1

/**
 *             :            stm32      ,                        mcu,                  .
 * usmart_reset_runtime,                        ,                                                            .                              ,                                    .
 * usmart_get_runtime,                        ,            CNT         ,      usmart                              ,                                 ,                  
 *             2   CNT      ,                  +            ,               2   ,            ,                  ,         :2*         CNT*0.1ms.   STM32      ,   :13.1s      
 *          :USMART_TIMX_IRQHandler   Timer4_Init,            MCU                  .                              :10Khz      .      ,                                          !!
 */

/**
 * @brief             runtime
 *   @note                                 MCU                              
 * @param          
 * @retval         
 */
void usmart_timx_reset_time(void)
{
    __HAL_TIM_CLEAR_FLAG(&g_timx_usmart_handle, TIM_FLAG_UPDATE); /*                       */
    __HAL_TIM_SET_AUTORELOAD(&g_timx_usmart_handle, 0XFFFF);      /*                                */
    __HAL_TIM_SET_COUNTER(&g_timx_usmart_handle, 0);              /*                   CNT */
    usmart_dev.runtime = 0;
}

/**
 * @brief             runtime      
 *   @note                                 MCU                              
 * @param          
 * @retval                  ,      :0.1ms,                              CNT      2   *0.1ms
 */
uint32_t usmart_timx_get_time(void)
{
    if (__HAL_TIM_GET_FLAG(&g_timx_usmart_handle, TIM_FLAG_UPDATE) == SET)  /*                ,                         */
    {
        usmart_dev.runtime += 0XFFFF;
    }
    usmart_dev.runtime += __HAL_TIM_GET_COUNTER(&g_timx_usmart_handle);
    return usmart_dev.runtime;                                 /*                 */
}

/**
 * @brief                               
 * @param       arr:                  
 *              psc:                     
 * @retval         
 */ 
void usmart_timx_init(uint16_t arr, uint16_t psc)
{
    USMART_TIMX_CLK_ENABLE();
    
    g_timx_usmart_handle.Instance = USMART_TIMX;                 /*                4 */
    g_timx_usmart_handle.Init.Prescaler = psc;                   /*              */
    g_timx_usmart_handle.Init.CounterMode = TIM_COUNTERMODE_UP;  /*                 */
    g_timx_usmart_handle.Init.Period = arr;                      /*                 */
    g_timx_usmart_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&g_timx_usmart_handle);
    HAL_TIM_Base_Start_IT(&g_timx_usmart_handle);                /*                                   */
    HAL_NVIC_SetPriority(USMART_TIMX_IRQn,3,3);                  /*                                        3               3 */
    HAL_NVIC_EnableIRQ(USMART_TIMX_IRQn);                        /*       ITM       */ 
}

/**
 * @brief       USMART                           
 * @param          
 * @retval         
 */
void USMART_TIMX_IRQHandler(void)
{
    if(__HAL_TIM_GET_IT_SOURCE(&g_timx_usmart_handle,TIM_IT_UPDATE)==SET)/*              */
    {
        usmart_dev.scan();                                   /*       usmart       */
        __HAL_TIM_SET_COUNTER(&g_timx_usmart_handle, 0);;    /*                   CNT */
        __HAL_TIM_SET_AUTORELOAD(&g_timx_usmart_handle, 100);/*                       */
    }
    
    __HAL_TIM_CLEAR_IT(&g_timx_usmart_handle, TIM_IT_UPDATE);/*                       */
}

#endif
















