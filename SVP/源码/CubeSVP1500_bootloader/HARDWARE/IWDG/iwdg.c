/*
    STM32F4 IWDG CODE
    2024/12/24
    HOU XINBO
*/
#include "iwdg.h"


HAL_StatusTypeDef HAL_IWDG_Start(IWDG_HandleTypeDef *hiwdg)
{
	__HAL_IWDG_START(hiwdg);//开启 IWDG外设
	
	__HAL_IWDG_RELOAD_COUNTER(hiwdg);//用RLR寄存器中定义的值重新加载IWDG计数器
	
	 return HAL_OK;
}

IWDG_HandleTypeDef IWDG_Config;   //独立看门狗句柄

//初始化独立看门狗
//prer:分频数:IWDG_PRESCALER_4~IWDG_PRESCALER_256
//rlr:自动重装载值,0~0XFFF.
//时间计算(大概):Tout=((4*2^prer)*rlr)/32 (ms).
void IWDG_Init(uint8_t prer,uint16_t rlr){
    IWDG_Config.Instance = IWDG;
    IWDG_Config.Init.Prescaler = prer;  //设置分频系数
    IWDG_Config.Init.Reload = rlr;      //设置重装载值
    HAL_IWDG_Init(&IWDG_Config);        //初始化IWDG
    
    HAL_IWDG_Start(&IWDG_Config);       //启动看门狗   
}

void IWDG_Feed(void){
    
    HAL_IWDG_Refresh(&IWDG_Config);
}
