#include "exti.h"
#include "led.h"
#include "delay.h"


uint8_t INTSign = 0 ;
uint8_t  num=0;  //记录中断次数


//外部中断0服务程序
//初始化PA0为中断输入.
void EXTIX_Init(void)
{
  	GPIO_InitTypeDef GPIO_InitStructure;
 	EXTI_InitTypeDef EXTI_InitStructure;

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

 // 初始化 WK_UP-->GPIOA.0	  下拉输入
  	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 
  	GPIO_Init(GPIOA, &GPIO_InitStructure);

  //GPIOA.0	  中断线以及中断初始化配置
 	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource0);

 	EXTI_InitStructure.EXTI_Line=EXTI_Line0;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);		//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

}

//TDC中断分组配置
void TDC_GP22_NVICConfiguration(uint8_t pre, uint8_t sub)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = pre ;//抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = sub;      //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;         //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure); //根据指定的参数初始化VIC寄存器
}


void EXTI0_IRQHandler(void)
{
//    delay_ms(10);    //消抖
  	if(EXTI_GetITStatus(EXTI_Line0) != RESET)	  //检查指定的EXTI0线路触发请求发生与否
	{	
        INTSign = 1 ;
		num++;        
		LED0=!LED0;
        EXTI_ClearITPendingBit(EXTI_Line0);  //清除EXTI0线路挂起位
	}
	
}
 
