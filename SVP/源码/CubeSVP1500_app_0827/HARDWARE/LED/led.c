/*
    STM32F4 LED CODE
    2024/12/24
    HOU XINBO
*/

#include "led.h"
uint8_t ledflag = 0;

void LED_Init(void){
    //定义配置GPIO结构体
    GPIO_InitTypeDef GPIO_Initure;
    
    //开启GPIO的RCC时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    //配置GPIO
    GPIO_Initure.Pin = GPIO_PIN_1 | GPIO_PIN_2 ;
    GPIO_Initure.Mode = /*推挽*/ GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;
    
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
    LED1= 1;
    LED2= 1;
	ledflag = init_ledmode; 
}

void led_test(void){
    LED1 = !LED1;
}

static uint8_t return_ledmode(void){
	return ledflag;
}

//判断当前LED状态 
void enter_surveymode_led(void){
	//如果是已经工作状态(绿灯常量)那么就不变化为探测阶段灯
	if(return_ledmode() != work_ledmode){
		ledflag = survey_ledmode;
	}
}

void enter_workmode_led(void){
	//判断当前是否为探测状态，如果为探测状态才可转换为工作状态
	if(return_ledmode() == survey_ledmode){
		ledflag = work_ledmode;
	}
}

void enter_errormode_led(void){
	ledflag = error_lenmode;
}

