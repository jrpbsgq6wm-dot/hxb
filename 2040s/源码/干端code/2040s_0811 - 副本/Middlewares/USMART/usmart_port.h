/**
 ****************************************************************************************************
 * @file        usmart_port.h
 * @author            ()
 * @version     V3.5
 * @date        2020-12-20
 * @brief       USMART                   
 *
 *              USMART                                                      ,          ,                                    
 *                                      ,         .      ,                                          (            (10/16      ,            )
 *                                                              ),                        10               ,                               .
 *              V2.1                  hex   dec            .                                                   .                           
 *                       ,      :
 *                    "hex 100"                                    HEX 0X64.
 *                    "dec 0X64"                                   DEC 100.
 *   @note
 *              USMART                  @MDK 3.80A@2.0         
 *              FLASH:4K~K      (      USMART_USE_HELP   USMART_USE_WRFUNS      )
 *              SRAM:72      (                  )
 *              SRAM            :   SRAM=PARM_LEN+72-4        PARM_LEN                  4.
 *                                         100         .
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
 
#ifndef __USMART_PORT_H
#define __USMART_PORT_H

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"




/******************************************************************************************/
/*                    */


#define MAX_FNAME_LEN           30      /*                                                                             */
#define MAX_PARM                10      /*          10          ,               ,            usmart_exe            . */
#define PARM_LEN                200     /*                                     PARM_LEN         ,                                       (         PARM_LEN) */


#define USMART_ENTIMX_SCAN      1       /*       TIM                        SCAN      ,               0,                                             scan      .
                                         *       :            runtime            ,            USMART_ENTIMX_SCAN   1!!!!
                                         */

#define USMART_USE_HELP         1       /*                            0                  700                                                       */
#define USMART_USE_WRFUNS       1       /*                   ,            ,                              ,                           . */

#define USMART_PRINTF           printf  /*       printf       */

/******************************************************************************************/
/* USMART                 */

# if USMART_ENTIMX_SCAN == 1    /*                               ,                      */

/* TIMX              
 *                   usmart.scan                        ,                     
 *       :                4            ,            TIM1~TIM17                     .
 */
#define USMART_TIMX                     TIM4
#define USMART_TIMX_IRQn                TIM4_IRQn
#define USMART_TIMX_IRQHandler          TIM4_IRQHandler
#define USMART_TIMX_CLK_ENABLE()        do{ __HAL_RCC_TIM4_CLK_ENABLE(); }while(0)  /* TIMX              */

#endif

/******************************************************************************************/


/*                   uint32_t,          */
#ifndef uint32_t
typedef unsigned           char uint8_t;
typedef unsigned short     int  uint16_t;
typedef unsigned           int  uint32_t;
#endif



char * usmart_get_input_string(void);        /*                       */
void usmart_timx_reset_time(void);              /*                    */
uint32_t usmart_timx_get_time(void);            /*                    */
void usmart_timx_init(uint16_t arr, uint16_t psc);   /*                    */

#endif



























