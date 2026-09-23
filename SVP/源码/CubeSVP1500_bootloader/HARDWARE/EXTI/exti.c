/*
    STM32F4 EXTI CODE
    2024/12/24
    HOU XINBO
*/
#include "exti.h"
#include "delay.h"
#include "led.h"
#include "usart.h"


//外部中断初始化
void EXTI_Init(void){
    GPIO_InitTypeDef GPIO_Config;
    
    //使能IO口时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    //C4 
    GPIO_Config.Pin = GPIO_PIN_4;
    GPIO_Config.Mode = GPIO_MODE_IT_FALLING; 
    GPIO_Config.Pull = GPIO_PULLDOWN;   //拉低
    GPIO_Config.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_Config);
   
    //a0
    GPIO_Config.Pin = GPIO_PIN_0;
    GPIO_Config.Mode = GPIO_MODE_IT_RISING; 
    GPIO_Config.Pull = GPIO_PULLUP;   //拉低
    GPIO_Config.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_Config);
    //配置外部中断优先级 使能中断
    HAL_NVIC_SetPriority(EXTI4_IRQn,0,0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);  
}

//中断处理函数回调函数 HAL库中所有的外部中断函数都会调用该回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if(GPIO_Pin == GPIO_PIN_4){
        INTSign = 1;   
        num++;       
    }

}

//中断处理函数处理过程
//中断处理函数EXTI4_IRQHandler -> 中断处理公用函数HAL_GPIO_EXTI_IRQHandler -> 中断处理函数回调函数 HAL_GPIO_EXTI_Callback 处理用户逻辑
void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4); //调用中断处理公用函数
}
