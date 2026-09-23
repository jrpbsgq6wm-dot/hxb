/*
    STM32F4 LED CODE
    2024/12/24
    HOU XINBO
*/
#ifndef __LED__H__
#define __LED__H__

#include "sys.h"


//#define LED_R   PAout(1)
//#define LED_G   PAout(2)


#define LED0 PAout(0)       //LED0
#define LED1 PAout(1)   	//LED1
#define LED2 PAout(2)   	//LED2
#define LED3 PAout(3)   	//LED3
#define LED4 PAout(4)   	//LED4
#define LED5 PAout(5)   	//LED5
#define LED6 PAout(6)   	//LED6
#define LED7 PAout(7)   	//LED7

void LED_Init(void);
void led_test(void);

#endif

