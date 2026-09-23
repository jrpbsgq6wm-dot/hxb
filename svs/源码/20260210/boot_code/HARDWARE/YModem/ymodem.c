#include "ymodem.h"
#include "delay.h"
#include "stmflash.h"
#include "usart1.h"
#include "stm32f10x.h"
#include "stdint.h"
#include <string.h>


uint8_t file_name_buf[256] = {0};    //0x2000082a
uint8_t file_size_buf[8] = {0};
volatile int file_size;
uint8_t ymodem_flag = 0;

/**
 * @brief 通过串口1发送一个字节
 * @param 发送的字节数据
 * @retval 
 */
uint32_t Send_Byte(uint8_t c){
    USART1_PutChar(c);
    return 0;
}

/**
 * @brief 文件大小的ASSIC码的十六进制转换为int类型
 * @param 字符串数组
 * @retval 
 */
int strToint(uint8_t *str,unsigned int len){
    int res = 0,i=0;
    for(i=0;i<len;i++){
        if(str[i] < '0' || str[i] > '9'){
            return -1;
        }
        
        res = res * 10 + (str[i] - '0');
    }
    return res;
}

/**
 * @brief 获取起始帧中的文件名和文件大小 
 * @param 起始帧文件名的地址
 * @retval 
 */
int Get_Ymodme_File_Information(uint8_t* data,uint32_t len){
    int i = 0,count = 0,file_size_len;
    for(i=0;i<len;i++){
        if(data[i] == 0x00){
            file_name_buf[count] = '\0';
            break;
        }else{
            file_name_buf[count] = data[i];
            count++;
        }
    } 
    memcpy(file_name,file_name_buf,count);
    count = 0;
    for(i+=1;i<len;i++){
        if(data[i] == 0x20){
            file_size_buf[count] = '\0';
            break;
        }else{
            file_size_buf[count] = data[i];
            count++;
        }
    }
    file_size_len = count;
    file_size = strToint(file_size_buf,file_size_len);
    return file_size;
}
/**
 * @brief 更新输入字节的CRC16
 * @param CRC输入值
 * @param 输入字节
 * @retval 
 */
uint16_t UpdataCRC16(uint16_t crcIn,uint8_t byte){
    uint32_t crc_data = crcIn;
    uint32_t in = byte | 0x100;
    do{
        crc_data <<= 1;
        in <<= 1;
        if(in & 0x100)
            ++crc_data;
        if(crc_data & 0x10000)
            crc_data ^= 0x1021;
    }while(!(in &0x10000));
    return crc_data & 0xffffu;
}
/**
 * @brief 用于Ymodem包的CRC16
 * @param 数据起始地址
 * @param 长度
 * @retval
 */
uint16_t Calculate_CRC16(const uint8_t* data,uint32_t size){
    uint32_t crc_data = 0;
    const uint8_t* dataEnd = data+size;
    while(data < dataEnd){
        crc_data = UpdataCRC16(crc_data,*data++);
    }
    crc_data = UpdataCRC16(crc_data,0);
    crc_data = UpdataCRC16(crc_data,0);
    return crc_data & 0xffffu;
}

/*------------------------------------------------------------------
** 函数原型: int CRC16 (const char *nData, int wLength)
** 功能描述: CRC16校验
** 输入参数: const char *nData, int wLength
** 调用模块: None
** 返 回 值: wCRCWord
** 说    明：
-------------------------------------------------------------------*/
static const int wCRCTable[] =
{
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

int CRC16(const void *_nData, uint16_t wLength)
{
    const char *nData;
    unsigned char nTemp;
    unsigned char CRCHI = 0XFF;
    unsigned char CRCLO = 0XFF;
    nData = (const char *)_nData;
    while (wLength--) 
    {
        nTemp = CRCHI ^ (*(nData++)); 	
        CRCHI = CRCLO ^ wCRCTable[nTemp]; 		
        CRCLO = wCRCTable[nTemp]>>8; 
    }
    return ((unsigned short)CRCLO << 8 | CRCHI);
    //return (unsigned short)(CRCHI << 8 | CRCLO);
} // End: CRC16




