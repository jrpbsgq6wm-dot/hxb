#include "stmflash.h"
#include "delay.h"


//读取指定地址的字(32位数据) 
//faddr:读地址 
//返回值:对应数据.
u32 STMFLASH_ReadWord(u32 faddr)
{
	return *(vu32*)faddr; 
}

//读取指定地址的半字(16位数据)
//faddr:读地址(此地址必须为2的倍数!!)
//返回值:对应数据.
u16 STMFLASH_ReadHalfWord(u32 faddr)
{
	return *(vu16*)faddr; 
}

//获取某个地址所在的flash扇区
//addr:flash地址
//返回值:0~11,即addr所在的扇区
u8 STMFLASH_GetFlashSector(u32 addr)
{
	if(addr<ADDR_FLASH_SECTOR_1)return FLASH_SECTOR_0;
	else if(addr<ADDR_FLASH_SECTOR_2)return FLASH_SECTOR_1;
	else if(addr<ADDR_FLASH_SECTOR_3)return FLASH_SECTOR_2;
	else if(addr<ADDR_FLASH_SECTOR_4)return FLASH_SECTOR_3;
	else if(addr<ADDR_FLASH_SECTOR_5)return FLASH_SECTOR_4;
	
	return FLASH_SECTOR_5;	
}

/*
    擦除指定扇区
*/
HAL_StatusTypeDef Erase_sector(u32 addr){
    FLASH_EraseInitTypeDef FlashEraseInit;
    u32 SectorError=0;
    
    FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //擦除类型，扇区擦除 
    FlashEraseInit.Sector=STMFLASH_GetFlashSector(addr);   //要擦除的扇区
    FlashEraseInit.NbSectors=1;                             //一次只擦除一个扇区
    FlashEraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;      //电压范围，VCC=2.7~3.6V之间!!
    if(HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError) != HAL_OK){
        printf("Erase_sector failed\r\n");
        return HAL_ERROR;//发生错误了	
    }
    if(FLASH_WaitForLastOperation(FLASH_WAITETIME) == HAL_OK){    //等待上次操作完成
        return HAL_OK;
    }                
    return HAL_ERROR;
}


/*        
    用于向STM32的系统配置扇区写入数据
*/
//WriteAddr:起始地址(此地址必须为4的倍数!!)
//pBuffer:数据指针
//NumToWrite:字(32位)数(就是要写入的32位数据的个数.) 
//成功返回HAL_OK  
HAL_StatusTypeDef STMFLASH_Write_SysConfig_32(u32 WriteAddr,u32 *pBuffer,u32 NumToWrite)	
{ 
    //判断是否是写入系统配置扇区的地址
    if((WriteAddr >= USER_FLASH_SECTOR_4 ) && WriteAddr < 0x0803FFFF){
        return HAL_ERROR;
    }
    if(WriteAddr % 4 == 0){
        uint32_t *sector_data;
        uint32_t sector_start_addr = USER_FLASH_SECTOR_4;    //目前所在用户扇区的起始地址
        uint32_t sector_size = USER_SECTOR_4_SIZE;
        uint32_t i;
        uint32_t offset ;
        
        //解锁flash
        HAL_FLASH_Unlock();           
        
        //将原有flash内容读取到缓冲区
        sector_data = (uint32_t *)mymalloc(16*1024);
        for(i=0;i<sector_size/4;i++){
            sector_data[i] = STMFLASH_ReadWord(sector_start_addr + i * 4);
        }  
        
        //修改缓冲中的数据
        offset = (WriteAddr - sector_start_addr) / 4;
        for(i=0;i<NumToWrite;i++){
            sector_data[offset + i] = pBuffer[i];
        }
        
        //擦除系统扇区
        if(Erase_sector(sector_start_addr) == HAL_OK){
            //写入数据
            for(i=0;i<sector_size/4;i++){
                if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,sector_start_addr + i*4,sector_data[i])!= HAL_OK){
                    printf("write flash failed\r\n");
                    return HAL_ERROR;
                }
            }
        }
        else{
            return HAL_ERROR;
        }
        
        //上锁
        HAL_FLASH_Lock();          
        
        //释放内存
        myfree(sector_data);
    }else{
        uint16_t *buf_16;
        /*
            1个32位数据 4字节 2 
            2个32为数据 8字节 4
        */
        buf_16 = (uint16_t *)mymalloc(NumToWrite*4);
        memcpy(buf_16,pBuffer,NumToWrite*4);
        
        if(STMFLASH_Write_SysConfig_16(WriteAddr,buf_16, NumToWrite*2) != HAL_OK){
            printf("write flash failed\r\n");
        }
        myfree(buf_16);
    }
    return HAL_OK;
} 

HAL_StatusTypeDef STMFLASH_Write_SysConfig_16(u32 WriteAddr,u16 *pBuffer,u32 NumToWrite){
    //判断是否是写入系统配置扇区的地址
    if(WriteAddr < USER_FLASH_SECTOR_1 || WriteAddr >= USER_FLASH_SECTOR_2){
        return HAL_ERROR;
    }
    
    uint16_t *sector_data;
    uint32_t sector_start_addr = USER_FLASH_SECTOR_1;    //目前所在用户扇区的起始地址
    uint32_t sector_size = USER_SECTOR_1_SIZE;
    uint32_t i;
    uint32_t offset ;
    Erase_sector(sector_start_addr);
    //解锁flash
	HAL_FLASH_Unlock();           
    
    //将原有flash内容读取到缓冲区
    sector_data = (uint16_t *)mymalloc(16*1024);
    for(i=0;i<sector_size/2;i++){
        sector_data[i] = STMFLASH_ReadHalfWord(sector_start_addr + i * 2);
    }  
    
    //修改缓冲中的数据
    offset = (WriteAddr - sector_start_addr)/2; //0x0800c008 - 0x0800c000 = 
    for(i=0;i<NumToWrite;i++){
        sector_data[offset + i] = pBuffer[i];
    }
    
    //擦除系统扇区
    if(Erase_sector(sector_start_addr) == HAL_OK){
        //printf("Erase_sysconfig_sectoe successfully\r\n");
        //写入数据
        for(i=0;i<sector_size/2;i++){
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,sector_start_addr + i*2,sector_data[i])!= HAL_OK){
                printf("write flash failed\r\n");
                return HAL_ERROR;
            }
        }
    }
    else{
        printf("Erase_sysconfig_sectoe failed\r\n");
        return HAL_ERROR;
    }
    
    //上锁
	HAL_FLASH_Lock();          
    
    //释放内存
    myfree(sector_data);
    return HAL_OK;

}

//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToRead:字(32位)数
void STMFLASH_Read_WORD(u32 ReadAddr,u32 *pBuffer,u32 NumToRead)   	
{
	u32 i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadWord(ReadAddr);//读取4个字节.
		ReadAddr+=4;//偏移4个字节.	
	}
}

//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToWrite:半字(16位)数
void STMFLASH_Read_2BYTE(u32 ReadAddr,u16 *pBuffer,u16 NumToRead)   	
{
	u16 i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);//读取2个字节.
		ReadAddr+=2;//偏移2个字节.	
	}
    delay_ms(10);
}

//bootloader 用于更新文件到备份区
#if EN_BOOTLOADER
HAL_StatusTypeDef STMFLASH_Write_BackUp(u32 WriteAddr,u32 *pBuffer,u32 NumToWrite){
    //判断是否是写入系统配置扇区的地址
    if(WriteAddr < USER_FLASH_SECTOR_3 || WriteAddr >= USER_FLASH_SECTOR_4){
        printf("Addr failed\r\n");
        return HAL_ERROR;
    }
    uint32_t sector_size = USER_SECTOR_3_SIZE;
    uint32_t sector_start_addr = USER_FLASH_SECTOR_3,i=0;
    //解锁flash
	HAL_FLASH_Unlock();
    
    //擦除系统扇区
    if(Erase_sector(USER_FLASH_SECTOR_3) == HAL_OK){
        printf("Erase_sysconfig_sectoe successfully\r\n");
        //写入数据
        for(i=0;i<sector_size/4;i++){
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,sector_start_addr + i*4,pBuffer[i])!= HAL_OK){
                printf("write flash failed\r\n");
                return HAL_ERROR;
            }
        }
    }
    else{
        printf("Erase_sysconfig_sectoe failed\r\n");
        return HAL_ERROR;
    }
    
    //上锁
	HAL_FLASH_Lock();          
    
    return HAL_OK;

}



#endif


