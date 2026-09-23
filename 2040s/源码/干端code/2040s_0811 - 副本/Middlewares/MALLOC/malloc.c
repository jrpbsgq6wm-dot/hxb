#include "./MALLOC/malloc.h"

#if !(__ARMCC_VERSION >= 6010050)   /* AC5 */
    /* 内存区，按 64 字节对齐 */
    static __align(64) uint8_t mem1base[MEM1_MAX_SIZE];                                     /* 内部 SRAM */
    static __align(64) uint8_t mem2base[MEM2_MAX_SIZE] __attribute__((at(0x10000000)));     /* CCM RAM */
    static __align(64) uint8_t mem3base[MEM3_MAX_SIZE] __attribute__((at(0x68000000)));     /* 外部 SRAM */

    /* 分配表 */
    static MT_TYPE mem1mapbase[MEM1_ALLOC_TABLE_SIZE];                                                  /* 内部 SRAM 分配表 */
    static MT_TYPE mem2mapbase[MEM2_ALLOC_TABLE_SIZE] __attribute__((at(0x10000000 + MEM2_MAX_SIZE)));  /* CCM 分配表 */
    static MT_TYPE mem3mapbase[MEM3_ALLOC_TABLE_SIZE] __attribute__((at(0x68000000 + MEM3_MAX_SIZE)));  /* 外部 SRAM 分配表 */
#else
    /* AC6 */
    static __ALIGNED(64) uint8_t mem1base[MEM1_MAX_SIZE];                                                     /* 内部 SRAM */
    static __ALIGNED(64) uint8_t mem2base[MEM2_MAX_SIZE] __attribute__((section(".bss.ARM.__at_0x10000000"))); /* CCM RAM */
    static __ALIGNED(64) uint8_t mem3base[MEM3_MAX_SIZE] __attribute__((section(".bss.ARM.__at_0x68000000"))); /* 外部 SRAM */

    /* 分配表 */
    static MT_TYPE mem1mapbase[MEM1_ALLOC_TABLE_SIZE];                                                        /* 内部 SRAM 分配表 */
    static MT_TYPE mem2mapbase[MEM2_ALLOC_TABLE_SIZE] __attribute__((section(".bss.ARM.__at_0x1000F000")));   /* CCM 分配表 */
    static MT_TYPE mem3mapbase[MEM3_ALLOC_TABLE_SIZE] __attribute__((section(".bss.ARM.__at_0x680F0C00")));   /* 外部 SRAM 分配表 */
#endif

/* 各内存池的块数、块大小、总大小 */
const uint32_t memtblsize[SRAMBANK] = {MEM1_ALLOC_TABLE_SIZE, MEM2_ALLOC_TABLE_SIZE, MEM3_ALLOC_TABLE_SIZE};
const uint32_t memblksize[SRAMBANK] = {MEM1_BLOCK_SIZE, MEM2_BLOCK_SIZE, MEM3_BLOCK_SIZE};
const uint32_t memsize[SRAMBANK] = {MEM1_MAX_SIZE, MEM2_MAX_SIZE, MEM3_MAX_SIZE};

/* 内存管理器对象 */
struct _m_mallco_dev mallco_dev =
{
    my_mem_init,
    my_mem_perused,
    mem1base, mem2base, mem3base,
    mem1mapbase, mem2mapbase, mem3mapbase,
    0, 0, 0,
};

/**
 * @brief  内存拷贝
 * @param  des : 目标地址
 * @param  src : 源地址
 * @param  n   : 拷贝字节数
 */
void my_mem_copy(void *des, void *src, uint32_t n)
{
    uint8_t *xdes = des;
    uint8_t *xsrc = src;

    while (n--)
    {
        *xdes++ = *xsrc++;
    }
}

/**
 * @brief  内存填充
 * @param  s     : 目标地址
 * @param  c     : 填充值
 * @param  count : 填充字节数
 */
void my_mem_set(void *s, uint8_t c, uint32_t count)
{
    uint8_t *xs = s;

    while (count--)
    {
        *xs++ = c;
    }
}

/**
 * @brief  初始化某个内存池
 * @param  memx : 内存池编号
 */
void my_mem_init(uint8_t memx)
{
    uint8_t mttsize = sizeof(MT_TYPE);

    my_mem_set(mallco_dev.memmap[memx], 0, memtblsize[memx] * mttsize);
    mallco_dev.memrdy[memx] = 1;
}

/**
 * @brief  查询某个内存池的使用率
 * @param  memx : 内存池编号
 * @retval 使用率，范围 0~1000，表示 0.0%~100.0%
 */
uint16_t my_mem_perused(uint8_t memx)
{
    uint32_t used = 0;
    uint32_t i;

    for (i = 0; i < memtblsize[memx]; i++)
    {
        if (mallco_dev.memmap[memx][i])
        {
            used++;
        }
    }

    return (used * 1000) / memtblsize[memx];
}

/**
 * @brief  申请内存块
 * @param  memx : 内存池编号
 * @param  size : 申请字节数
 * @retval 偏移地址
 *   @arg 0 ~ 0xFFFFFFFE : 成功
 *   @arg 0xFFFFFFFF     : 失败
 */
static uint32_t my_mem_malloc(uint8_t memx, uint32_t size)
{
    signed long offset = 0;
    uint32_t nmemb;     /* 需要多少个块 */
    uint32_t cmemb = 0; /* 当前连续空块数 */
    uint32_t i;

    if (!mallco_dev.memrdy[memx])
    {
        mallco_dev.init(memx);
    }

    if (size == 0)
    {
        return 0xFFFFFFFF;
    }

    nmemb = size / memblksize[memx];
    if (size % memblksize[memx])
    {
        nmemb++;
    }

    /* 从后往前找连续空块 */
    for (offset = memtblsize[memx] - 1; offset >= 0; offset--)
    {
        if (!mallco_dev.memmap[memx][offset])
        {
            cmemb++;
        }
        else
        {
            cmemb = 0;
        }

        if (cmemb == nmemb)
        {
            for (i = 0; i < nmemb; i++)
            {
                mallco_dev.memmap[memx][offset + i] = nmemb;
            }

            return (offset * memblksize[memx]);
        }
    }
    return 0xFFFFFFFF; 
}


/**
 * @brief
 * @param     
 * @param
 * @retval              
 *   @arg
 *   @arg
 *   @arg 
 */
static uint8_t my_mem_free(uint8_t memx, uint32_t offset)
{
    int i;

    if (!mallco_dev.memrdy[memx])
    {
        mallco_dev.init(memx);
        return 1;
    }

    if (offset < memsize[memx])
    {
        int index = offset / memblksize[memx]; 
        int nmemb = mallco_dev.memmap[memx][index];

        for (i = 0; i < nmemb; i++)    
        {
            mallco_dev.memmap[memx][index + i] = 0;
        }

        return 0;
    }
    else
    {
        return 2; 
    }
}

/**
 * @brief 
 * @param  
 * @param 
 * @retval        
 */
void myfree(uint8_t memx, void *ptr)
{
    uint32_t offset;

    if (ptr == NULL)return;

    offset = (uint32_t)ptr - (uint32_t)mallco_dev.membase[memx];
    my_mem_free(memx, offset); 
}

/**
 * @brief 
 * @param    
 * @param 
 * @retval
 */
void *mymalloc(uint8_t memx, uint32_t size)
{
    uint32_t offset;
    offset = my_mem_malloc(memx, size);

    if (offset == 0xFFFFFFFF)  
    {
        return NULL;
    }
    else
    {
        return (void *)((uint32_t)mallco_dev.membase[memx] + offset);
    }
}

/**
 * @brief
 * @param           
 * @param           
 * @param       
 * @retval   
 */
void *myrealloc(uint8_t memx, void *ptr, uint32_t size)
{
    uint32_t offset;
    offset = my_mem_malloc(memx, size);

    if (offset == 0xFFFFFFFF)
    {
        return NULL;         
    }
    else    
    {
        my_mem_copy((void *)((uint32_t)mallco_dev.membase[memx] + offset), ptr, size); 
        myfree(memx, ptr); 
        return (void *)((uint32_t)mallco_dev.membase[memx] + offset); 
    }
}


