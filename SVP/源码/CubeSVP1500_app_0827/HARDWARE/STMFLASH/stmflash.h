/*仅用作程序升级*/

#ifndef __STMFLASH_H
#define __STMFLASH_H
#include "sys.h"
#include "malloc.h"

//FLASH起始地址
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH的起始地址
#define STM32_FLASH_END  0x08040000     //STM32 FLASH的结束地址
#define FLASH_WAITETIME  50000          //FLASH等待超时时间

//系统扇区大小
#define SECTOR_0_3_SIZE         0x4000
#define SECTOR_4_SIZE           0x10000
#define SECTOR_5_SIZE           0x20000

//STM32系统FLASH 扇区的起始地址
#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) 	//扇区0起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) 	//扇区1起始地址, 16 Kbytes 
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) 	//扇区2起始地址, 16 Kbytes 
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) 	//扇区3起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) 	//扇区4起始地址, 64 Kbytes  
#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) 	//扇区5起始地址, 128 Kbytes 

//STM32用户FLASH扇区起始地址
#define USER_FLASH_SECTOR_0     ((u32)0x08000000)   //Bootloader
#define USER_FLASH_SECTOR_1     ((u32)0x08008000)   //BootloaderBackUp
#define USER_FLASH_SECTOR_2     ((u32)0x08010000)   //SvpApp
#define USER_FLASH_SECTOR_3     ((u32)0x08020000)   //SvpAppBackUp
#define USER_FLASH_SECTOR_4     ((u32)0x08030000)   //SysConfig 预留

//用户指定扇区大小
#define USER_SECTOR_0_SIZE      32*1024
#define USER_SECTOR_1_SIZE      32*1024
#define USER_SECTOR_2_SIZE      64*1024
#define USER_SECTOR_3_SIZE      64*1024
#define USER_SECTOR_4_SIZE      64*1024


u32 STMFLASH_ReadWord(u32 faddr);	
u16 STMFLASH_ReadHalfWord(u32 faddr);//读出字  
HAL_StatusTypeDef STMFLASH_Write_SysConfig_32(u32 WriteAddr,u32 *pBuffer,u32 NumToWrite);		//从指定地址开始写入指定长度的数据
HAL_StatusTypeDef STMFLASH_Write_SysConfig_16(u32 WriteAddr,u16 *pBuffer,u32 NumToWrite);
void STMFLASH_Read_2BYTE(u32 ReadAddr,u16 *pBuffer,u16 NumToRead) ; //从指定地址开始读出指定长度的2BYTE数据
void STMFLASH_Read_WORD(u32 ReadAddr,u32 *pBuffer,u32 NumToRead);   //从指定地址开始读出指定长度的4BYTE数据
extern HAL_StatusTypeDef Erase_sector(u32 addr);        //擦除系统扇区
//测试写入
void Test_Write(u32 WriteAddr,u32 WriteData);	
#endif
