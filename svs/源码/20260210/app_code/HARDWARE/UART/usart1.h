#ifndef __USART1_H
#define __USART1_H

#include "main.h"
#include "stdio.h"


#define RS485_TX_EN PBout(0)

void usart1_init(u32 bound);
void USART1_NVICConfiguration(uint8_t pre, uint8_t sub);
void USART1_Send_Data(void *sendBuf, u16 len);
void u1_printf(char *fmt, ...);


void up(char* str);
#endif




