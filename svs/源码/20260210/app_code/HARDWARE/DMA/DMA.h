#ifndef __DMA_H
#define __DMA_H

#include "stm32f10x.h"



#define UART1DMA_TXBUF_SIZE (1024*4)
#define UART1DMA_RXBUF_SIZE (1024*4)
extern char UART1DMA_TxBUF[UART1DMA_TXBUF_SIZE];
extern unsigned int UART1DMA_TxCOUNT;
extern unsigned char DMA_TcFlags;
extern char UART1DMA_RxBUF[UART1DMA_RXBUF_SIZE];
extern unsigned int UART1DMA_RxCOUNT;

extern void UART1DMA_Init(void);
extern void UART1DMA_Tx_TEST(void);
extern void print(void);
extern void DMA1_Channel4_IRQHandler(void);
extern void UART1_DMA_SEND_DATA(char* data,uint16_t len);

#endif



