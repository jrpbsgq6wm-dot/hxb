#ifndef __DELAY_H
#define __DELAY_H 			   
#include "stm32f10x.h"



extern void delay_init(u8 SYSCLK);
extern void delay_ms(u16 nms);
extern void delay_us(u32 nus);

#endif





























