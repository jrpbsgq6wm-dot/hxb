#ifndef __STMFLASH_H
#define __STMFLASH_H
#include "sys.h"

//FLASH起始地址
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH的起始地址
#define FLASH_WAITETIME  50000          //FLASH等待超时时间

//FLASH 扇区的起始地址
#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) 	//扇区0起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) 	//扇区1起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) 	//扇区2起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) 	//扇区3起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) 	//扇区4起始地址, 64 Kbytes  
#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) 	//扇区5起始地址, 128 Kbytes  

 
u32 STMFLASH_ReadWord(u32 faddr);		  	//读出字  
void STMFLASH_Write(u32 WriteAddr,u32 *pBuffer,u32 NumToWrite);		//从指定地址开始写入指定长度的数据
void STMFLASH_Read(u32 ReadAddr,u32 *pBuffer,u32 NumToRead);
/*
 * 在调用者已完成扇区擦除的前提下，直接向 Flash 写入连续字节。
 * 此函数不会自动擦除任何扇区，适合 BOOT 接收到一个 YMODEM 数据包后立即写入，
 * 避免将整份应用程序缓存到 RAM 中。
 *
 * WriteAddr 和 NumToWrite 必须均为 4 字节对齐。
 */
HAL_StatusTypeDef STMFLASH_Write_NoErase(u32 WriteAddr, const u8 *pBuffer, u32 NumToWrite);   		//从指定地址开始读出指定长度的数据
//测试写入
void Test_Write(u32 WriteAddr,u32 WriteData);
HAL_StatusTypeDef Erase_sector(u32 addr);
#endif
