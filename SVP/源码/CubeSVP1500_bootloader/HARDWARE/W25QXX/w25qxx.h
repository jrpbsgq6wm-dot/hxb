
#ifndef __W25QXX_H
#define __W25QXX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "sys.h"

//使用4字节寻址时将宏设为1，用3字节寻址时设为0
#define ENABLE_4_BYTE_ADDR 0

//W25X系列/Q系列芯片列表
//W25Q80  ID  0XEF13
//W25Q16  ID  0XEF14
//W25Q32  ID  0XEF15
//W25Q64  ID  0XEF16
//W25Q128 ID  0XEF17

#define W25Q80  0XEF13
#define W25Q16  0XEF14
#define W25Q32  0XEF15
#define W25Q64  0XEF16
#define W25Q128 0XEF17
#define W25Q256 0XEF18
#define NM25Q16     0X6814          /* NM25Q16  芯片ID */
extern u16 W25QXX_TYPE;                 //定义W25QXX芯片型号

#define W25QXX_CS          PAout(15)  //W25QXX的片选信号 
#define W25QXX_USE_MALLOC   0           //定义是否使用动态内存管理

//////////////////////////////////////////////////////////////////////////////////
//指令表
#define W25X_WriteEnable        0x06
#define W25X_WriteDisable       0x04
#define W25X_ReadStatusReg      0x05
#define W25X_WriteStatusReg     0x01
#define W25X_ReadData           0x03
#define W25X_FastReadData       0x0B
#define W25X_FastReadDual       0x3B
#define W25X_PageProgram        0x02
#define W25X_BlockErase         0xD8
#define W25X_SectorErase        0x20
#define W25X_ChipErase          0xC7
#define W25X_PowerDown          0xB9
#define W25X_ReleasePowerDown   0xAB
#define W25X_DeviceID           0xAB
#define W25X_ManufactDeviceID   0x90
#define W25X_JedecDeviceID      0x9F
#define W25X_Enable4ByteAddr    0xB7
#define W25X_Disable4ByteAddr   0xE9

#define GD25Q_Check()           ((W25QXX_ReadID() == NM25Q16) ? 1 : 0)

void W25QXX_Init(void);
u16  W25QXX_ReadID(void);               //读取FLASH ID
u8   W25QXX_ReadSR(void);               //读取状态寄存器
void W25QXX_Write_SR(u8 sr);            //写状态寄存器
void W25QXX_Write_Enable(void);         //写使能
void W25QXX_Write_Disable(void);        //写保护
void W25QXX_Write_NoCheck(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
void W25QXX_Read(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead); //读取flash
void W25QXX_Write(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite); //写入flash
void W25QXX_Erase_Chip(void);           //整片擦除
void W25QXX_Erase_Sector(u32 Dst_Addr); //扇区擦除
void W25QXX_Wait_Busy(void);            //等待空闲
void W25QXX_PowerDown(void);            //进入掉电模式
void W25QXX_WAKEUP(void);               //唤醒
void W25QXX_Test(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

