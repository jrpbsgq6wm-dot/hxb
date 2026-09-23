/*
    STM32F4 IWDG CODE
    2024/12/24
    HOU XINBO
*/

#ifndef __IWDG__H__
#define __IWDG__H__

#include "sys.h"

void IWDG_Init(uint8_t prer,uint16_t rlr);
void IWDG_Feed(void);

#endif


