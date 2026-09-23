#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "main.h"  


//////////////////////////////////////////////////////////////////////////////////////////////////////
//用户根据自己的需要设置
#define STM32_FLASH_SIZE 64 	 		//所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 1              //使能FLASH写入(0，不是能;1，使能)
//////////////////////////////////////////////////////////////////////////////////////////////////////

//FLASH起始地址
#define STM32_FLASH_BASE            0X08000000 	    //STM32 FLASH的起始地址
/*flash基础配置地址*/
#define APP_EDITION_ADDR            0X0800FC00      //APP程序版本号
#define BOOT_EDITION_ADDR           0X0800FC0E      //BOOT程序版本号
#define DEVICE_BOUND_ADDR           0X0800FC1C      //波特率
#define DEVICE_TYPE_ADDR            0X0800FC20      //接口类型
#define DEVICE_FORMAT_ADDR          0X0800FC22      //数据协议
#define DEVICE_FACTORY_TIME_ADDR    0X0800FC24      //出厂时间
#define UPDATA_FLAG                 0X0800FC28      //程序更新标志位1
#define UPDATA_BOOT_FLAG            0X0800FC2A      //程序更新标志位2

/*flash TDC配置地址*/
#define PROBE_DISTANCE_ADDR         0X0800FC30      //探头距离挡片距离
#define OUTLIERS_THRESHOLD_ADDR     0X0800FC34      //阈值
#define KLAMAN_OBSERVATIONS_ADDR    0X0800FC38      //MAX_OBSERVATIONS kalman滤波样本大小
#define SOS_FREQ_ADDR               0X0800FC3C      //声速测量的频率
        /*TDC系数*/
#define SOUND_VELOCITY_COE_A1_ADDR  0X0800FC40      //系数A1
#define SOUND_VELOCITY_COE_B1_ADDR  0X0800FC44      //系数B1
#define SOUND_VELOCITY_COE_A2_ADDR  0X0800FC48      //系数A2
#define SOUND_VELOCITY_COE_B2_ADDR  0X0800FC4C      //系数B2
#define SOUND_VELOCITY_COE_A3_ADDR  0X0800FC50      //系数A3
#define SOUND_VELOCITY_COE_B3_ADDR  0X0800FC54      //系数B3
        /*应用系数的范围*/
#define COE_USE_A1_B1_UL            0X0800FC58      //应用A1 A2系数的上限             
#define COE_USE_A1_B1_LL            0X0800FC5C      //应用A1 A2系数的下限  
#define COE_USE_A2_B2_UL            0X0800FC60      //应用A1 A2系数的上限             
#define COE_USE_A2_B2_LL            0X0800FC64      //应用A1 A2系数的下限 
#define COE_USE_A3_B3_UL            0X0800FC68      //应用A1 A2系数的上限             
#define COE_USE_A3_B3_LL            0X0800FC6C      //应用A1 A2系数的下限 
        
#define FRIST_WAVE_VOLTAGE          0X0800FC70      //TDC第一波阈值电压配置
#define FRIE_COUNT                  0X0800FC74      //TDC发射脉冲个数

#define DEVICE_SN_ADDR              0x0800FC80      //sn
u16 STMFLASH_ReadHalfWord(u32 faddr);		  //读出半字  
void STMFLASH_WriteLenByte(u32 WriteAddr,u32 DataToWrite,u16 Len);	//指定地址开始写入指定长度的数据
u32 STMFLASH_ReadLenByte(u32 ReadAddr,u16 Len);						//指定地址开始读取指定长度数据
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);		//从指定地址开始写入指定长度的数据
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);   		//从指定地址开始读出指定长度的数据
extern void Erase_Flash_Section(uint32_t addr);
//测试写入
void Test_Write(u32 WriteAddr,u16 WriteData);								   
#endif

















