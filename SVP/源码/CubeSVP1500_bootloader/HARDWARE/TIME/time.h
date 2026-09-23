#ifndef __TIME__H__
#define __TIME__H__

#include "sys.h"
#include "rtc.h"
#include "fat.h"

#define CLOCK_PSC   9600
//TIM2-TIM5
extern TIM_HandleTypeDef TIM_Config_2;  //TDC 
extern TIM_HandleTypeDef TIM_Config_3;  //定时器3 RTC or LTC2943句柄
extern TIM_HandleTypeDef TIM_Config_4;  //压力 温度
extern TIM_HandleTypeDef TIM_Config_5;  //

void TIM3_Init(uint16_t arr,uint16_t psc);
void TIM2_Init(uint16_t arr,uint16_t psc);
void TIM4_Init(uint16_t arr,uint16_t psc);
void TIM5_Init(uint16_t arr,uint16_t psc);

extern void timework_init(void);

#endif

