#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "main.h"  
#include <string.h>


//////////////////////////////////////////////////////////////////////////////////////////////////////
//用户根据自己的需要设置
#define STM32_FLASH_SIZE 64 	 		//所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 1              //使能FLASH写入(0，不是能;1，使能)
//////////////////////////////////////////////////////////////////////////////////////////////////////

#define FLASH_SIZE      (0x10000)
#define PAGE_SIZE       (0X400)

//FLASH起始地址
#define STM32_FLASH_BASE            0x08000000 	    //STM32 FLASH的起始地址
#define FLASH_SAVE_ADDR             0X0800FC00      //设置FLASH 保存地址,version number  14
#define DEVICE_BOUND_ADDR           0X0800FC0E
#define DEVICE_TYPE_ADDR            0X0800FC12  
#define DEVICE_FORMAT_ADDR          0X0800FC14 

#define PROBE_DISTANCE_ADDR         0X0800FC18
#define SOUND_VELOCITY_COE_ADDR     0X0800FC1C
#define OUTLIERS_THRESHOLD_ADDR     0X0800FC20
#define KLAMAN_OBSERVATIONS_ADDR    0X0800FC24
#define DEVICE_FACTORY_TIME_ADDR    0X0800FC28
#define DEVICE_SN_ADDR              0X0800FC2C
#define SOS_FREQ_ADDR               0X0800FC3A
#define UPDATA_FLAG                 0X0800FC28
#define UPDATA_APP_BOOT             0X0800FC2A
#define BOOT_SN_ADDR                0X0800FC48


u16 STMFLASH_ReadHalfWord(u32 faddr);		  //读出半字  
void STMFLASH_WriteLenByte(u32 WriteAddr,u32 DataToWrite,u16 Len);	//指定地址开始写入指定长度的数据
u32 STMFLASH_ReadLenByte(u32 ReadAddr,u16 Len);						//指定地址开始读取指定长度数据
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);		//从指定地址开始写入指定长度的数据
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);   		//从指定地址开始读出指定长度的数据

//测试写入
void Test_Write(u32 WriteAddr,u16 WriteData);								   

void Erase_Flash_Section(uint32_t addr);
void read_flash_flag(uint32_t addr,uint16_t* flag);
void write_flash_flag(uint32_t addr,uint16_t flag);
#endif

















