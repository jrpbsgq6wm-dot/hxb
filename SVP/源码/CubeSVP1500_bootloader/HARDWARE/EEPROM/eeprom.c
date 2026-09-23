#include "eeprom.h"

#include "usart.h"
#include "delay.h"
#include "string.h"




HAL_StatusTypeDef AT24C02_Write(uint16_t memAddress,uint8_t *pData,uint16_t size){
    if(memAddress + size > 256){
        return HAL_ERROR;
    }
    HAL_StatusTypeDef status;
    uint16_t byteWritten = 0;
    uint16_t currentAddress = memAddress;   //使用临时变量 避免修改原参数
    while(byteWritten < size){
        //计算当前页偏移和可写入字节数
        uint16_t pageOffset = currentAddress % EEPROM_PAGE_BYTE_SIZE;
        uint16_t bytesRemainingInPage = EEPROM_PAGE_BYTE_SIZE - pageOffset;
        uint16_t bytesToWrite = (size - byteWritten) < bytesRemainingInPage ? (size-byteWritten) : bytesRemainingInPage;
        //执行写入操作
        status = HAL_I2C_Mem_Write(&IIC_Config,EEPROM_WRITE_ADDR,currentAddress,I2C_MEMADD_SIZE_8BIT,&pData[byteWritten],bytesToWrite,HAL_MAX_DELAY);
        if(status != HAL_OK )
            return status;
        //等待写入完成
        HAL_Delay(5);
        //更新计数器和地址
        byteWritten += bytesToWrite;
        currentAddress += bytesToWrite;
    }
    return status;
}


HAL_StatusTypeDef AT24C02_Read(uint16_t memAddress,uint8_t *pData,uint16_t size){
    //页大小8byte 读取时可以跨页
    //检查地址是否超出范围
    if(memAddress + size > 256){
        return HAL_ERROR;
    }
    //发送内存地址
    return HAL_I2C_Mem_Read(&IIC_Config,EEPROM_WRITE_ADDR|0x01,memAddress,I2C_MEMADD_SIZE_8BIT,pData,size,HAL_MAX_DELAY);
}

void AT24C02_EraseALL(void){
    uint8_t blank[256] = {0};
    AT24C02_Write(0,blank,256);
    delay_ms(5);
}




void SVP_CMD_INIT(svp_cmd_t *svp_cmd){
    AT24C02_Read(0,(uint8_t*)svp_cmd,sizeof(svp_cmd_t));      //读取eeprom数据到buf
}
