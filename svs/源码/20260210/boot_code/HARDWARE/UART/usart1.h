#ifndef __USART1_H
#define __USART1_H

#include "stm32f10x.h"
#include "main.h"
#include "stdio.h"
#include "dma.h"
#include "string.h"

#define RS485_TX_EN PBout(0)

#define UART1_RXBUFF_SIZE   1029 //定义串口1接收缓冲区大小

extern volatile u32 UART1_RxCounter;
extern uint8_t UART1_RxBuff[UART1_RXBUFF_SIZE]; //指定串口1接收缓冲区

extern void UART1_Init(u32 bound);
extern void USART1_PutChar(uint8_t c);
extern void USART1_PutStr(uint8_t* s);
extern uint8_t USART1_RecvData(uint8_t* data);
extern void USART1_Send_Data(void *sendBuf, u16 len);
void u1_printf(char *fmt, ...);

#endif




