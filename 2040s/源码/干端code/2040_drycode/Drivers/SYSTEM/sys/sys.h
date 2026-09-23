/**
 ****************************************************************************************************
 * @file        sys.h
 * @author            ()
 * @version     V1.0
 * @date        2021-10-14
 * @brief                            (                  /            /GPIO         )
 * @license     Copyright (c) 2020-2032,                                        
 ****************************************************************************************************
 * @attention
 *
 *             :           F407         
 *             :www.yuanzige.com
 *             :www.openedv.com
 *             :www.alientek.com
 *             :openedv.taobao.com
 *
 *                
 * V1.0 20211014

 ****************************************************************************************************
 */

#ifndef _SYS_H
#define _SYS_H

#include "stm32f4xx.h"
#include "core_cm4.h"
#include "stm32f4xx_hal.h"


/**
 * SYS_SUPPORT_OS                                       OS
 * 0,         OS
 * 1,      OS
 */
#define SYS_SUPPORT_OS         1


/*            *******************************************************************************************/

void sys_nvic_set_vector_table(uint32_t baseaddr, uint32_t offset);                         /*                       */
void sys_standby(void);                                                                     /*                    */
void sys_soft_reset(void);                                                                  /*                 */
uint8_t sys_stm32_clock_init(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq);   /*                    */


/*                       */
void sys_wfi_set(void);             /*       WFI       */
void sys_intx_disable(void);        /*                    */
void sys_intx_enable(void);         /*                    */
void sys_msr_msp(uint32_t addr);    /*                    */

#endif

