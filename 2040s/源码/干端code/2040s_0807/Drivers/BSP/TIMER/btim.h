   /**
 ****************************************************************************************************
 * @file        btim.h
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
 *         :openedv.taobao.com
 *
 *         
 * V1.0 20211015
 *           
 * V1.1 20211129
 *           3                    
 ****************************************************************************************************
 */

#ifndef __BTIM_H
#define __BTIM_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/*                 */

/* TIMX          
 *           TIM6/TIM7
 *     :           4        ,        TIM1~TIM8              .
 */

#define BTIM_TIM3_INT                       TIM3
#define BTIM_TIM3_INT_IRQn                  TIM3_IRQn
#define BTIM_TIM3_INT_IRQHandler            TIM3_IRQHandler
#define BTIM_TIM3_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM3_CLK_ENABLE(); }while(0)  /* TIM3          */


#define BTIM_TIM6_INT                       TIM6
#define BTIM_TIM6_INT_IRQn                  TIM6_DAC_IRQn
#define BTIM_TIM6_INT_IRQHandler            TIM6_DAC_IRQHandler
#define BTIM_TIM6_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM6_CLK_ENABLE(); }while(0)  /* TIM6          */


/******************************************************************************************/

void btim_tim3_int_init(uint16_t arr, uint16_t psc);    /*                               */
void btim_tim6_int_init(uint16_t arr, uint16_t psc);    /*                               */

#endif





