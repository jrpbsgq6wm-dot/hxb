/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "w25qxx.h"
#include "rtc.h"
#include "malloc.h"


///* Definitions of physical drive number for each drive */
//#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
//#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
//#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */

#define SPI_FLASH   0

#define PAGE_SIZE       256     
#define SECTOR_SIZE     4096  
#define SECTOR_COUNT    4096 
#define BLOCK_SIZE      16        


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = STA_NOINIT;
    switch(pdrv){
        case SPI_FLASH:
            if(W25Q128 == W25QXX_ReadID()){
                stat  = RES_OK;    //检测成功
            }else{
                stat = STA_NOINIT;
            }
            break;
    }
    
    return stat;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = STA_NOINIT;
    uint16_t i;
    switch(pdrv){
        case SPI_FLASH:
            /*flash 初始化*/
            W25QXX_Init();                //SPI norflash初始化 --> SPI2_Init()
            /*延时一小会儿*/
            i=500;
            while(i--);
            /*唤醒flash*/
            //Norflash_WAKEUP();
            stat = disk_status(SPI_FLASH);
            break;
    }
    
    return stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	if(!count){
        return RES_PARERR;  //count 不能等于0
    }
    switch(pdrv){
        case SPI_FLASH:
            
                W25QXX_Read(buff,sector*SECTOR_SIZE,SECTOR_SIZE);
				return RES_OK;
    }
    return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
    if (!count)
    {
        return RES_PARERR;//count不能等于0，否则返回参数错误		 	 
    }
    
	switch (pdrv)
	{
		case SPI_FLASH://外部flash
            W25QXX_Write((u8*)buff,sector*SECTOR_SIZE,SECTOR_SIZE);
                return RES_OK;
    }
    return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	switch (pdrv) {
        case SPI_FLASH:
            switch(cmd){
                case CTRL_SYNC:
                    return RES_OK;
                
                /*扇区个数*/
                case GET_SECTOR_COUNT:
                    *(DWORD*)buff = SECTOR_COUNT;
                    return RES_OK;

                /*扇区大小*/
                case GET_SECTOR_SIZE:
                    *(DWORD*)buff = SECTOR_SIZE;
                    return RES_OK;

                /*同时擦除扇区的个数*/
                case GET_BLOCK_SIZE:
                    *(DWORD*)buff = BLOCK_SIZE;
                    return RES_OK;
      
            }
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

//动态分配内存
void *ff_memalloc (UINT size)			
{
	return (void*)mymalloc(size);
}
//释放内存
void ff_memfree (void* mf)		 
{
	myfree(mf);
}
