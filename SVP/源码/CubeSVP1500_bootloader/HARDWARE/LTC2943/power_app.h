#ifndef __POWER_APP__H__
#define __POWER_APP__H__
#include "ltc2943.h"
#include "usart.h"

extern float charge,current,voltage,temperature;

extern HAL_StatusTypeDef read_power_data(void);

extern uint8_t menu_1_automatic_mode(uint8_t mAh_or_Coulombs,uint8_t celcius_or_kelvin,uint16_t prescalar_mode,
                uint16_t prescalar_value,uint16_t alcc_mode);
    

#endif
