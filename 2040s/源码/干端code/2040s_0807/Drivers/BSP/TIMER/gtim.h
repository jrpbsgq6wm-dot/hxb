   /**
 ****************************************************************************************************
 * @file        gtim.h
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
 *         :openedv.taobao.com
 *
 *         
 * V1.0 20211015
 *           
 *
 ****************************************************************************************************
 */

#ifndef __GTIM_H
#define __GTIM_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/*                 */

/* TIMX          
 *           TIM2~TIM5.
 *     :           4        ,        TIM1~TIM8              .
 */
 
#define GTIM_TIMX_INT                       TIM3
#define GTIM_TIMX_INT_IRQn                  TIM3_IRQn
#define GTIM_TIMX_INT_IRQHandler            TIM3_IRQHandler
#define GTIM_TIMX_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM3_CLK_ENABLE(); }while(0)  /* TIM3          */

/******************************************************************************************/

void gtim_timx_int_init(uint16_t arr, uint16_t psc);        /*                               */



#endif

















