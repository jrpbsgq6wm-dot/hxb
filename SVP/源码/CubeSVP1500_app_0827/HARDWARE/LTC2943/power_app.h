

#ifndef __POWER_APP__H__
#define __POWER_APP__H__
#include "ltc2943.h"
#include "usart.h"

typedef struct{
	int NOW_SOC;
	int LAST_SOC;
}Battry_t;
extern Battry_t battry_t;
extern uint16_t Full_Battry_V;
extern float charge,current,voltage,temperature,SOC;
extern void ltc2943_init(void);
extern HAL_StatusTypeDef read_power_data(void);
extern void LTC2943_writeFullChargeI2C(uint16_t fullacr);
extern uint8_t menu_1_automatic_mode(uint8_t mAh_or_Coulombs,uint8_t celcius_or_kelvin,uint16_t prescalar_mode,
uint16_t prescalar_value,uint16_t alcc_mode);
    

#endif
