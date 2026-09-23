/**
 ****************************************************************************************************
 * @file        usmart.h
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
 ****************************************************************************************************
 */

#ifndef __USMART_H
#define __USMART_H

#include "./USMART/usmart_port.h"


#define USMART_OK               0       /*           */
#define USMART_FUNCERR          1       /*              */
#define USMART_PARMERR          2       /*              */
#define USMART_PARMOVER         3       /*              */
#define USMART_NOFUNCFIND       4       /*                       */

#define SP_TYPE_DEC             0       /* 10                   */
#define SP_TYPE_HEX             1       /* 16                   */


/*                 */
struct _m_usmart_nametab
{
    void *func;             /*              */
    const char *name;       /*          (         ) */
};

/* usmart                */
struct _m_usmart_dev
{
    struct _m_usmart_nametab *funs;     /*                 */

    void (*init)(uint16_t tclk);        /*           */
    uint8_t (*cmd_rec)(char *str);   /*                          */
    void (*exe)(void);                  /*         */
    void (*scan)(void);                 /*        */
    uint8_t fnum;                       /*              */
    uint8_t pnum;                       /*              */
    uint8_t id;                         /*       id */
    uint8_t sptype;                     /*                   (                  ):0,10      ;1,16      ; */
    uint16_t parmtype;                  /*                 */
    uint8_t  plentbl[MAX_PARM];         /*                                */
    uint8_t  parm[PARM_LEN];            /*                 */
    uint8_t runtimeflag;                /* 0,                           ;1,                        ,      :                  USMART_ENTIMX_SCAN               ,          */
    uint32_t runtime;                   /*             ,      :0.1ms,                              CNT      2   *0.1ms */
};

extern struct _m_usmart_nametab usmart_nametab[];   /*    usmart_config.c             */
extern struct _m_usmart_dev usmart_dev;             /*    usmart_config.c             */


void usmart_init(uint16_t tclk);        /*           */
uint8_t usmart_cmd_rec(char*str);    	/*        */
void usmart_exe(void);                  /*        */
void usmart_scan(void);                 /*        */
uint32_t read_addr(uint32_t addr);      /*                          */
void write_addr(uint32_t addr,uint32_t val);/*                                   */

#endif






























