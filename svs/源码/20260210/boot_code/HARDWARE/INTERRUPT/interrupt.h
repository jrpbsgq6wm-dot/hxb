#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include "stm32f10x.h"
#include "main.h"
#include "stdio.h"
#include "dma.h"
#include "usart1.h"

void USART1_IRQHandler(void);
void DMA1_Channel4_IRQHandler(void);
void USART1_IRQHandler(void);

#endif

