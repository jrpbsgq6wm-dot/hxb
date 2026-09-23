/*
    STM32F4 LED CODE
    2024/12/24
    HOU XINBO
*/

#include "led.h"

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
   
    LED1 = 0;
    LED2 = 0;
}

void led_test(void){
    LED1 = !LED1;
}

