#ifndef __YMODEM_H__
#define __YMODEM_H__

#include "stm32f10x.h"
#include <string.h>
#include <stdbool.h>
#include "usart1.h"


#define PACKET_SEQNO_INDEX  (1)
#define PACKET_SEQNO_COMP_INDEX (2)
#define PACKET_HEADER   (3)
#define PACKET_TRAILER  (2)

#define PACKET_OVERHEAD (PACKET_HEADER+PACKET_TRAILER)
#define PACKET_SIZE     (128)
#define PACKET_1K_SIZE  (1024)

//YMODEM_CMD
#define     SOH         0X01
#define     STX         0X02
#define     EOT         0X04       
#define     ACK         0X06        
#define     NAK         0X15
#define     CA          0X18
#define     C           0X43
#define     ABORT1      0X41
#define     ABORT2      0X61    

#define YMODEM_CMD_SIZE 1

extern uint8_t ymodem_flag;
extern uint8_t file_name_buf[256];
extern uint8_t file_size_buf[8];


extern uint32_t Send_Byte(uint8_t c);
uint16_t UpdataCRC16(uint16_t crcIn,uint8_t byte);
uint16_t Calculate_CRC16(const uint8_t* data,uint32_t size);
extern int Get_Ymodme_File_Information(uint8_t* data,uint32_t len);
extern int strToint(uint8_t *str,unsigned int len);
int CRC16(const void *_nData, uint16_t wLength);
#endif 



