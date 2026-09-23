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
 * V1.0 20211104
 *           
 *
 ****************************************************************************************************
 */

#include "./MALLOC/malloc.h"


#if !(__ARMCC_VERSION >= 6010050)   /*     AC6              AC5         */
/*       (64        ) */
static __align(64) uint8_t mem1base[MEM1_MAX_SIZE];                                     /*     SRAM       */
static __align(64) uint8_t mem2base[MEM2_MAX_SIZE] __attribute__((at(0x10000000)));     /*     CCM       */
static __align(64) uint8_t mem3base[MEM3_MAX_SIZE] __attribute__((at(0x68000000)));     /*     SRAM       */

/*            */
static MT_TYPE mem1mapbase[MEM1_ALLOC_TABLE_SIZE];                                                  /*     SRAM      MAP */
static MT_TYPE mem2mapbase[MEM2_ALLOC_TABLE_SIZE] __attribute__((at(0x10000000 + MEM2_MAX_SIZE)));  /*     CCM      MAP */
static MT_TYPE mem3mapbase[MEM3_ALLOC_TABLE_SIZE] __attribute__((at(0x68000000 + MEM3_MAX_SIZE)));  /*     SRAM      MAP */
#else      /*     AC6         */
/*       (64        ) */
static __ALIGNED(64) uint8_t mem1base[MEM1_MAX_SIZE];                                                           /*     SRAM       */
static __ALIGNED(64) uint8_t mem2base[MEM2_MAX_SIZE] __attribute__((section(".bss.ARM.__at_0x10000000")));      /*     CCM       */
static __ALIGNED(64) uint8_t mem3base[MEM3_MAX_SIZE] __attribute__((section(".bss.ARM.__at_0x68000000")));      /*     SRAM       */ 

/*            */
static MT_TYPE mem1mapbase[MEM1_ALLOC_TABLE_SIZE];                                                              /*     SRAM      MAP */
static MT_TYPE mem2mapbase[MEM2_ALLOC_TABLE_SIZE] __attribute__((section(".bss.ARM.__at_0x1000F000")));         /*     CCM      MAP */
static MT_TYPE mem3mapbase[MEM3_ALLOC_TABLE_SIZE] __attribute__((section(".bss.ARM.__at_0x680F0C00")));         /*     SRAM      MAP */
#endif

/*              */
const uint32_t memtblsize[SRAMBANK] = {MEM1_ALLOC_TABLE_SIZE, MEM2_ALLOC_TABLE_SIZE, MEM3_ALLOC_TABLE_SIZE};    /*            */
const uint32_t memblksize[SRAMBANK] = {MEM1_BLOCK_SIZE, MEM2_BLOCK_SIZE, MEM3_BLOCK_SIZE};                      /*              */
const uint32_t memsize[SRAMBANK] = {MEM1_MAX_SIZE, MEM2_MAX_SIZE, MEM3_MAX_SIZE};                               /*            */

/*                */
struct _m_mallco_dev mallco_dev =
{
    my_mem_init,                                /*            */
    my_mem_perused,                             /*            */
    mem1base, mem2base, mem3base,               /*        */
    mem1mapbase, mem2mapbase, mem3mapbase,      /*                */
    0, 0, 0,                                    /*                */
};

/**
 * @brief               
 * @param       *des :         
 * @param       *src :       
 * @param       n    :                   (          )
 * @retval        
 */
void my_mem_copy(void *des, void *src, uint32_t n)
{
    uint8_t *xdes = des;
    uint8_t *xsrc = src;

    while (n--)*xdes++ = *xsrc++;
}

/**
 * @brief                 
 * @param       *s    :           
 * @param       c     :           
 * @param       count :                   (          )
 * @retval        
 */
void my_mem_set(void *s, uint8_t c, uint32_t count)
{
    uint8_t *xs = s;

    while (count--)*xs++ = c;
}

/**
 * @brief                     
 * @param       memx :           
 * @retval        
 */
void my_mem_init(uint8_t memx)
{
    uint8_t mttsize = sizeof(MT_TYPE);  /*     memmap              (uint16_t /uint32_t)*/
    my_mem_set(mallco_dev.memmap[memx], 0, memtblsize[memx]*mttsize); /*                    */
    mallco_dev.memrdy[memx] = 1;        /*               OK */
}

/**
 * @brief                     
 * @param       memx :           
 * @retval            (      10  ,0~1000,    0.0%~100.0%)
 */
uint16_t my_mem_perused(uint8_t memx)
{
    uint32_t used = 0;
    uint32_t i;

    for (i = 0; i < memtblsize[memx]; i++)
    {
        if (mallco_dev.memmap[memx][i])used++;
    }

    return (used * 1000) / (memtblsize[memx]);
}

/**
 * @brief               (        )
 * @param       memx :           
 * @param       size :                 (    )
 * @retval                  
 *   @arg       0 ~ 0xFFFFFFFE :                   
 *   @arg       0xFFFFFFFF     :                   
 */
static uint32_t my_mem_malloc(uint8_t memx, uint32_t size)
{
    signed long offset = 0;
    uint32_t nmemb;     /*                */
    uint32_t cmemb = 0; /*                */
    uint32_t i;

    if (!mallco_dev.memrdy[memx])
    {
        mallco_dev.init(memx);          /*         ,             */
    }
    
    if (size == 0) return 0xFFFFFFFF;   /*            */

    nmemb = size / memblksize[memx];    /*                            */

    if (size % memblksize[memx]) nmemb++;

    for (offset = memtblsize[memx] - 1; offset >= 0; offset--)  /*                    */
    {
        if (!mallco_dev.memmap[memx][offset])
        {
            cmemb++;            /*                    */
        }
        else 
        {
            cmemb = 0;          /*                */
        }
        
        if (cmemb == nmemb)     /*           nmemb           */
        {
            for (i = 0; i < nmemb; i++) /*                */
            {
                mallco_dev.memmap[memx][offset + i] = nmemb;
            }

            return (offset * memblksize[memx]); /*              */
        }
    }

    return 0xFFFFFFFF;  /*                            */
}

/**
 * @brief               (        )
 * @param       memx   :           
 * @param       offset :             
 * @retval              
 *   @arg       0,         ;
 *   @arg       1,         ;
 *   @arg       2,         (    );
 */
static uint8_t my_mem_free(uint8_t memx, uint32_t offset)
{
    int i;

    if (!mallco_dev.memrdy[memx])   /*         ,             */
    {
        mallco_dev.init(memx);
        return 1;                   /*          */
    }

    if (offset < memsize[memx])     /*               . */
    {
        int index = offset / memblksize[memx];      /*                    */
        int nmemb = mallco_dev.memmap[memx][index]; /*            */

        for (i = 0; i < nmemb; i++)                 /*            */
        {
            mallco_dev.memmap[memx][index + i] = 0;
        }

        return 0;
    }
    else
    {
        return 2;   /*           . */
    }
}

/**
 * @brief               (        )
 * @param       memx :           
 * @param       ptr  :           
 * @retval        
 */
void myfree(uint8_t memx, void *ptr)
{
    uint32_t offset;

    if (ptr == NULL)return;     /*       0. */

    offset = (uint32_t)ptr - (uint32_t)mallco_dev.membase[memx];
    my_mem_free(memx, offset);  /*          */
}

/**
 * @brief               (        )
 * @param       memx :           
 * @param       size :                 (    )
 * @retval                        .
 */
void *mymalloc(uint8_t memx, uint32_t size)
{
    uint32_t offset;
    offset = my_mem_malloc(memx, size);

    if (offset == 0xFFFFFFFF)   /*          */
    {
        return NULL;            /*       (0) */
    }
    else    /*           ,            */
    {
        return (void *)((uint32_t)mallco_dev.membase[memx] + offset);
    }
}

/**
 * @brief                   (        )
 * @param       memx :           
 * @param       *ptr :             
 * @param       size :                 (    )
 * @retval                          .
 */
void *myrealloc(uint8_t memx, void *ptr, uint32_t size)
{
    uint32_t offset;
    offset = my_mem_malloc(memx, size);

    if (offset == 0xFFFFFFFF)   /*          */
    {
        return NULL;            /*       (0) */
    }
    else    /*           ,            */
    {
        my_mem_copy((void *)((uint32_t)mallco_dev.membase[memx] + offset), ptr, size); /*                        */
        myfree(memx, ptr);  /*            */
        return (void *)((uint32_t)mallco_dev.membase[memx] + offset);   /*                  */
    }
}



