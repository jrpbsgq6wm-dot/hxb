#ifndef __W25QXX_H
#define __W25QXX_H

#include "sys.h"

/*
 * 两种硬件使用相同的 SPI3 和片选引脚，差异只在 Flash 型号、容量及地址长度：
 *
 * W25Q128  : 16 MB，使用 3 字节地址；
 * W25Q01JV : 128 MB，使用 4 字节地址。
 *
 * FatFs 对两种芯片统一使用 512B 逻辑扇区。物理擦除单元始终为 4KB，
 * 所以每个物理擦除单元对应 8 个 FatFs 逻辑扇区。
 */
#define W25QXX_PAGE_SIZE                256U
#define W25QXX_ERASE_SECTOR_SIZE        4096U
#define W25QXX_FATFS_SECTOR_SIZE        512U
#define W25QXX_FATFS_SECTORS_PER_ERASE  (W25QXX_ERASE_SECTOR_SIZE / W25QXX_FATFS_SECTOR_SIZE)

#define W25Q128_TOTAL_CAPACITY          (16UL * 1024UL * 1024UL)
#define W25Q01JV_TOTAL_CAPACITY         (128UL * 1024UL * 1024UL)

/* 通过 0x9F 命令读取到的三字节 JEDEC ID。 */
#define W25Q128_JEDEC_ID                0xEF4018UL
#define W25Q01JV_JEDEC_ID               0xEF4021UL

/* 0x90 兼容读 ID 命令中，旧版 W25Q128 返回的厂商/设备 ID。 */
#define W25Q128_LEGACY_ID               0xEF17U

typedef enum {
    W25QXX_TYPE_NONE = 0,
    W25QXX_TYPE_W25Q128,
    W25QXX_TYPE_W25Q01JV
} W25QXX_Type;

/*
 * 该结构体由驱动在 W25QXX_Init() 成功后填写。
 * diskio.c 只通过该结构体获取真实容量，避免把某一种型号的容量写死。
 */
typedef struct {
    W25QXX_Type type;
    uint32_t jedec_id;
    uint32_t capacity_bytes;
    uint32_t page_size;
    uint32_t erase_sector_size;
    uint16_t fatfs_sector_size;
    uint8_t address_bytes;
    uint8_t initialized;
} W25QXX_Info;

/*
 * 初始化 CS、复位 Flash、读取 ID 并选择对应的地址长度和 SPI 速率。
 * 返回 1 表示识别到受支持芯片，返回 0 表示未检测到或型号不受支持。
 */
uint8_t W25QXX_Init(void);

/* 返回当前已识别芯片的静态信息。未初始化时 type 为 W25QXX_TYPE_NONE。 */
const W25QXX_Info *W25QXX_GetInfo(void);

/* 查询 Flash 是否已被本驱动成功识别并完成初始化。 */
uint8_t W25QXX_IsReady(void);

/* 读取原始 Flash ID，供初始化和现场排查使用。 */
uint32_t W25QXX_ReadJEDECID(void);
uint16_t W25QXX_ReadLegacyID(void);

/*
 * 通用字节读写接口。
 *
 * W25QXX_WriteWithErase() 按 4KB 物理擦除块执行读-改-擦-写，
 * 可安全处理任意 512B FatFs 扇区写入及跨擦除块写入。
 */
uint8_t W25QXX_ReadData(uint32_t addr, uint8_t *buffer, uint32_t len);
uint8_t W25QXX_WriteWithErase(uint32_t addr, const uint8_t *buffer, uint32_t len);

/* 等待当前擦除或页编程完成，内部带超时，成功返回 1。 */
uint8_t W25QXX_WaitBusy(void);

#endif
