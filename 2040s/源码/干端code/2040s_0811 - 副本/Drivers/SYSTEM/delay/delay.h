/**
 ****************************************************************************************************
 * @file        delay.h
 * @author            ()
 * @version     V1.0
 * @date        2021-10-14
 * @brief             SysTick                                          (      ucosii)
 *                    delay_init                   delay_us   delay_ms               
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
 *                
 *
 ****************************************************************************************************
 */
 
#ifndef __DELAY_H
#define __DELAY_H

#include "./SYSTEM/sys/sys.h"


void delay_init(uint16_t sysclk);           /*                       */
void delay_ms(uint16_t nms);                /*       nms */
void delay_us(uint32_t nus);                /*       nus */

#if (!SYS_SUPPORT_OS)                       /*             Systick       */
    void HAL_Delay(uint32_t Delay);         /* HAL                     SDIO                */
#endif

#endif

