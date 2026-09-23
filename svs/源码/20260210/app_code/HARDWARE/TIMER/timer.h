#ifndef _TIMER_H
#define _TIMER_H
#include "main.h"

extern uint32_t interrupt_count;
extern uint32_t interrupt_comp;

extern uint32_t readly_flag;
extern uint32_t tdc_crc_flag;

extern uint32_t interrupt_tim3_count;

void TIM3_Int_Init(u16 arr,u16 psc);
void TIM3_NVICConfiguration(uint8_t pre, uint8_t sub);
void TIM2_IRQHandler(void);
void TIM2_Int_Init(u16 arr,u16 psc);
void TIM2_NVICConfiguration(uint8_t pre, uint8_t sub);
void TIM4_Int_Init(u16 arr,u16 psc);
void TIM4_NVICConfiguration(uint8_t pre, uint8_t sub);

void PauseTimer(TIM_TypeDef* TIMx);

void ResumeTimer(TIM_TypeDef* TIMx);



#endif
