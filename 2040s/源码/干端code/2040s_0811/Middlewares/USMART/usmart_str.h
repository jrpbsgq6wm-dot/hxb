/**
 ****************************************************************************************************
 * @file        usmart_str.h
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
 * V3.5 20201220
 * 1                              AC6         
 *
 ****************************************************************************************************
 */

#ifndef __USMART_STR_H
#define __USMART_STR_H

#include "./USMART/usmart_port.h"


uint8_t usmart_get_parmpos(uint8_t num);                /*                                                     */
uint8_t usmart_strcmp(char *str1, char *str2);    /*                                   */
uint32_t usmart_pow(uint8_t m, uint8_t n);              /* M^N       */
uint8_t usmart_str2num(char *str, uint32_t *res);    /*                       */
uint8_t usmart_get_cmdname(char *str, char *cmdname, uint8_t *nlen, uint8_t maxlen); /*    str                  ,                      */
uint8_t usmart_get_fname(char *str, char *fname, uint8_t *pnum, uint8_t *rval); /*    str                   */
uint8_t usmart_get_aparm(char *str, char *fparm, uint8_t *ptype); /*    str                            */
uint8_t usmart_get_fparam(char *str, uint8_t *parn); /*       str                        . */

#endif











