/*-----------------------------------------------------------------------*/
/* FatFs 底层磁盘接口：SPI Flash 适配层                                */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"
#include "rtc.h"
#include "w25qxx.h"
#include "string.h"


/*
 * 两种板卡都把 SPI Flash 映射为物理盘 0，并且都向 FatFs 提供 512B 逻辑扇区。
 * W25Q128 与 W25Q01JV 的总扇区数由运行时识别到的实际容量决定，不能写死。
 */
#define DEV_QSPI                    0U
#define FLASH_SECTOR_SIZE_BYTES     W25QXX_FATFS_SECTOR_SIZE

/*
 * 每次 disk_write 后按 512B 逐扇区回读校验。该缓冲区无需大于逻辑扇区，
 * 因为 4KB 的读-改-擦-写保护由 w25qxx.c 内部的静态缓冲区完成。
 */
static BYTE disk_verify_buf[FLASH_SECTOR_SIZE_BYTES];


static uint32_t Disk_GetSectorCount(void)
{
    const W25QXX_Info *flash_info = W25QXX_GetInfo();

    if (flash_info->initialized == 0U) {
        return 0U;
    }

    return flash_info->capacity_bytes / FLASH_SECTOR_SIZE_BYTES;
}

DSTATUS disk_status (
	BYTE pdrv
)
{
    if (pdrv != DEV_QSPI) {
        return STA_NOINIT;
    }

    /*
     * 状态查询不能每次都重新发 ID 命令，更不能重新初始化 Flash。
     * 初始化结果保存在通用驱动中，后续只判断该结果即可。
     */
    return W25QXX_IsReady() ? 0U : STA_NOINIT;
}


DSTATUS disk_initialize (
	BYTE pdrv
)
{
    if (pdrv != DEV_QSPI) {
        return STA_NOINIT;
    }

    /*
     * 首次挂载时读取 JEDEC ID，选择 W25Q128 或 W25Q01JV 的命令集。
     * 已初始化时直接返回，避免 f_mount()、f_mkfs() 等操作重复复位正在使用的 Flash。
     */
    if ((W25QXX_IsReady() == 0U) && (W25QXX_Init() == 0U)) {
        return STA_NOINIT;
    }

    return 0U;
}


DRESULT disk_read (
	BYTE pdrv,
	BYTE *buff,
	LBA_t sector,
	UINT count
)
{
    uint32_t sector_count;
    uint32_t addr;
    uint32_t len;

    if ((buff == NULL) || (count == 0U)) {
        return RES_PARERR;
    }

    if (pdrv != DEV_QSPI) {
        return RES_PARERR;
    }

    if (W25QXX_IsReady() == 0U) {
        return RES_NOTRDY;
    }

    sector_count = Disk_GetSectorCount();
    if ((sector >= sector_count) || (count > (sector_count - sector))) {
        return RES_PARERR;
    }

    /*
     * 经过上面的扇区范围检查后，地址和长度均不会超过最大 128MB，
     * 可以安全转换为 uint32_t 交给底层 Flash 驱动。
     */
    addr = (uint32_t)sector * FLASH_SECTOR_SIZE_BYTES;
    len = (uint32_t)count * FLASH_SECTOR_SIZE_BYTES;

    return W25QXX_ReadData(addr, buff, len) ? RES_OK : RES_ERROR;
}


#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,
	const BYTE *buff,
	LBA_t sector,
	UINT count
)
{
    uint32_t sector_count;
    uint32_t addr;
    uint32_t len;
    UINT index;

    if ((buff == NULL) || (count == 0U)) {
        return RES_PARERR;
    }

    if (pdrv != DEV_QSPI) {
        return RES_PARERR;
    }

    if (W25QXX_IsReady() == 0U) {
        return RES_NOTRDY;
    }

    sector_count = Disk_GetSectorCount();
    if ((sector >= sector_count) || (count > (sector_count - sector))) {
        return RES_PARERR;
    }

    addr = (uint32_t)sector * FLASH_SECTOR_SIZE_BYTES;
    len = (uint32_t)count * FLASH_SECTOR_SIZE_BYTES;

    /*
     * 底层会按 4KB 物理块执行读-改-擦-写，因此这里即使只写一个 512B 扇区，
     * 也不会损坏同一物理块内未参与本次写入的其余 7 个 FatFs 扇区。
     */
    if (W25QXX_WriteWithErase(addr, buff, len) == 0U) {
        return RES_ERROR;
    }

    /*
     * 保留原工程已有的写后校验。Flash 擦写出现异常时让 FatFs 立即得到错误，
     * 不把损坏的数据当作成功写入，以免后续文件目录或记录数据悄然失效。
     */
    for (index = 0U; index < count; index++) {
        addr = (uint32_t)(sector + index) * FLASH_SECTOR_SIZE_BYTES;

        if (W25QXX_ReadData(addr, disk_verify_buf, FLASH_SECTOR_SIZE_BYTES) == 0U) {
            return RES_ERROR;
        }

        if (memcmp(disk_verify_buf, &buff[index * FLASH_SECTOR_SIZE_BYTES],
                   FLASH_SECTOR_SIZE_BYTES) != 0) {
            return RES_ERROR;
        }
    }

    return RES_OK;
}

#endif


DRESULT disk_ioctl (
	BYTE pdrv,
	BYTE cmd,
	void *buff
)
{
    if (pdrv != DEV_QSPI) {
        return RES_PARERR;
    }

    if (W25QXX_IsReady() == 0U) {
        return RES_NOTRDY;
    }

    switch (cmd) {
        case CTRL_SYNC:
            /* 所有页编程和擦除都已完成后，FatFs 才能继续提交目录或 FAT 更新。 */
            return W25QXX_WaitBusy() ? RES_OK : RES_ERROR;

        case GET_SECTOR_COUNT:
            if (buff == NULL) {
                return RES_PARERR;
            }
            *(DWORD *)buff = (DWORD)Disk_GetSectorCount();
            return RES_OK;

        case GET_SECTOR_SIZE:
            if (buff == NULL) {
                return RES_PARERR;
            }
            *(WORD *)buff = (WORD)FLASH_SECTOR_SIZE_BYTES;
            return RES_OK;

        case GET_BLOCK_SIZE:
            if (buff == NULL) {
                return RES_PARERR;
            }
            /*
             * 返回“一个擦除块包含多少个逻辑扇区”，而非字节数。
             * 8 x 512B = 4096B，与两种 Flash 的最小物理擦除单位一致。
             */
            *(DWORD *)buff = (DWORD)W25QXX_FATFS_SECTORS_PER_ERASE;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}


DWORD get_fattime(void){
    DWORD fattime;
    fattime = (DS3231_Time.year + 2000 - 1980) << 25;
    fattime |= DS3231_Time.mon << 21;
    fattime |= DS3231_Time.date << 16;
    fattime |= DS3231_Time.hour << 11;
    fattime |= DS3231_Time.min << 5;
    fattime |= DS3231_Time.sec;
    return fattime;
}



