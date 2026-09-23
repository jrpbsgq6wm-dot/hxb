#ifndef __MALLOC_H
#define __MALLOC_H

#include "./SYSTEM/sys/sys.h"

/* 内存池编号 */
#define SRAMIN      0   /* 内部 SRAM */
#define SRAMCCM     1   /* CCM RAM（CPU 可直接访问，DMA 不可访问） */
#define SRAMEX      2   /* 外部 SRAM */

#define SRAMBANK    3   /* 一共有 3 个内存池 */

/* 内存块类型：
 * 外部 SDRAM/大容量内存时可用 uint32_t，这里用 uint16_t 即可
 */
#define MT_TYPE     uint16_t

/*
 * 分块内存管理说明：
 * - 每个内存池被切成很多固定大小的块
 * - 分配表 memmap[] 记录每个块的占用情况
 *
 * 计算关系：
 *   分配表大小 = 总容量 / 块大小
 *   内存池总实际占用 = 数据区 + 分配表
 */

/* mem1：内部 SRAM */
#define MEM1_BLOCK_SIZE         32
#define MEM1_MAX_SIZE           (20 * 1024)
#define MEM1_ALLOC_TABLE_SIZE   (MEM1_MAX_SIZE / MEM1_BLOCK_SIZE)

/* mem2：CCM RAM */
#define MEM2_BLOCK_SIZE         32
#define MEM2_MAX_SIZE           (60 * 1024)
#define MEM2_ALLOC_TABLE_SIZE   (MEM2_MAX_SIZE / MEM2_BLOCK_SIZE)

/* mem3：外部 SRAM */
#define MEM3_BLOCK_SIZE         32
#define MEM3_MAX_SIZE           (500 * 1024)
#define MEM3_ALLOC_TABLE_SIZE   (MEM3_MAX_SIZE / MEM3_BLOCK_SIZE)

/* 如果系统里没有定义 NULL，这里补一个 */
#ifndef NULL
#define NULL 0
#endif

/* 内存管理器控制结构 */
struct _m_mallco_dev
{
    void (*init)(uint8_t);             /* 初始化某个内存池 */
    uint16_t (*perused)(uint8_t);      /* 查询某个内存池使用率 */
    uint8_t *membase[SRAMBANK];        /* 各内存池起始地址 */
    MT_TYPE *memmap[SRAMBANK];         /* 各内存池分配表 */
    uint8_t memrdy[SRAMBANK];          /* 内存池是否已初始化 */
};

extern struct _m_mallco_dev mallco_dev;

/* 对外接口 */
void my_mem_init(uint8_t memx);                      /* 初始化指定内存池 */
uint16_t my_mem_perused(uint8_t memx);               /* 查询指定内存池使用率 */
void my_mem_set(void *s, uint8_t c, uint32_t count); /* 内存填充 */
void my_mem_copy(void *des, void *src, uint32_t n);  /* 内存拷贝 */

extern void myfree(uint8_t memx, void *ptr);                /* 释放内存 */
extern void *mymalloc(uint8_t memx, uint32_t size);         /* 申请内存 */
void *myrealloc(uint8_t memx, void *ptr, uint32_t size); /* 重新申请内存 */

#endif
