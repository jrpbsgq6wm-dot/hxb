/**
 ****************************************************************************************************
 * @file        malloc.c
 * @author                  ()
 * @version     V1.0
 * @date        2021-11-04
 * @brief                    
 * @license     Copyright (c) 2020-2032,                           
 ****************************************************************************************************
 * @attention
 *
 *         :         STM32      
 *         :www.yuanzige.com
 *         :www.openedv.com
 *         :www..com
 *         :openedv.taobao.com
 *
 *         
 * V1.0 2011104
 *           
 *
 ****************************************************************************************************
 */

#ifndef __MALLOC_H
#define __MALLOC_H

#include "./SYSTEM/sys/sys.h"

/*                */
#define SRAMIN                  0                               /*            */
#define SRAMCCM                 1                               /* CCM      (      SRAM    CPU        !!!) */
#define SRAMEX                  2                               /*            */

#define SRAMBANK                3                               /*           SRAM     */


/*                   ,      SDRAM                uint32_t                    uint16_t                 */
#define MT_TYPE     uint16_t


/*                                                     
 * size = MEM1_MAX_SIZE + (MEM1_MAX_SIZE / MEM1_BLOCK_SIZE) * sizeof(MT_TYPE)
 *   SRAMEX      size = 963 * 1024 + (963 * 1024 / 32) * 2 = 1047744    1023KB

 *               (size)                            
 * MEM1_MAX_SIZE = (MEM1_BLOCK_SIZE * size) / (MEM1_BLOCK_SIZE + sizeof(MT_TYPE))
 *   CCM    , MEM2_MAX_SIZE = (32 * 64) / (32 + 2) = 60.24KB    60KB
 */
 
/* mem1            .mem1            SRAM     */
#define MEM1_BLOCK_SIZE         32                              /*             32     */
#define MEM1_MAX_SIZE           20*1024                         /*              100K */
#define MEM1_ALLOC_TABLE_SIZE   MEM1_MAX_SIZE/MEM1_BLOCK_SIZE   /*            */

/* mem2            .mem2    CCM,        CCM(        ,      SRAM,  CPU        !!) */
#define MEM2_BLOCK_SIZE         32                              /*             32     */
#define MEM2_MAX_SIZE           60 *1024                        /*             60K */
#define MEM2_ALLOC_TABLE_SIZE   MEM2_MAX_SIZE/MEM2_BLOCK_SIZE   /*            */

/* mem3            .mem3      SRAM */
#define MEM3_BLOCK_SIZE         32                              /*             32     */
#define MEM3_MAX_SIZE           500 *1024                       /*             963K */
#define MEM3_ALLOC_TABLE_SIZE   MEM3_MAX_SIZE/MEM3_BLOCK_SIZE   /*            */


/*             NULL,     NULL */
#ifndef NULL
#define NULL 0
#endif



/*                */
struct _m_mallco_dev
{
    void (*init)(uint8_t);              /*        */
    uint16_t (*perused)(uint8_t);       /*            */
    uint8_t *membase[SRAMBANK];         /*            SRAMBANK             */
    MT_TYPE *memmap[SRAMBANK];          /*                */
    uint8_t  memrdy[SRAMBANK];          /*                  */
};

extern struct _m_mallco_dev mallco_dev; /*   mallco.c         */


/*              */
void my_mem_init(uint8_t memx);                     /*                   (  /        ) */
uint16_t my_mem_perused(uint8_t memx) ;             /*               (  /        ) */
void my_mem_set(void *s, uint8_t c, uint32_t count);/*              */
void my_mem_copy(void *des, void *src, uint32_t n); /*              */

void myfree(uint8_t memx, void *ptr);               /*         (        ) */
void *mymalloc(uint8_t memx, uint32_t size);        /*         (        ) */
void *myrealloc(uint8_t memx, void *ptr, uint32_t size);    /*             (        ) */

#endif





