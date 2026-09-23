#include "timer.h"
#include "led.h"
#include "main.h"
#include "serial.h"
#include "usart1.h"
#include "tdc_gp22.h"
#include "string.h"
#include "delay.h"
#include "stm32f10x_tim.h"
#include "stmflash.h"
#include "wdg.h"
#include "spi.h"

uint32_t tdc_crc_flag = 0;
uint32_t readly_flag = 0;
uint32_t interrupt_comp = 0; //打印输出的频率
uint32_t zijiancount = 0;
uint32_t svzj = 0,reset_count = 0;

void TIM2_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  ///使能TIM2时钟
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //自动重装载值
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);
    TIM_ClearFlag(TIM2,TIM_FLAG_Update);
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //允许定时器2更新中断
	TIM_Cmd(TIM2,ENABLE); //使能定时器2
}

void TIM2_NVICConfiguration(uint8_t pre, uint8_t sub)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    
    NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn; //定时器2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = pre; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = sub; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
//通用定时器3中断初始化
//arr：自动重装值。
//psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=定时器工作频率,单位:Mhz
//这里使用的是定时器3!
void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///使能TIM3时钟
	
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //自动重装载值
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);
	TIM_ClearFlag(TIM2,TIM_FLAG_Update);
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM3,ENABLE); //使能定时器3
}

void TIM3_NVICConfiguration(uint8_t pre, uint8_t sub)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = pre; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = sub; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void TIM4_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);  
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; 
	TIM_TimeBaseInitStructure.TIM_Period=arr;   
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseInitStructure);
    TIM_ClearFlag(TIM4,TIM_FLAG_Update);
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE); 
	TIM_Cmd(TIM4,ENABLE); 
}

void TIM4_NVICConfiguration(uint8_t pre, uint8_t sub)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    
    NVIC_InitStructure.NVIC_IRQChannel=TIM4_IRQn; //定时器2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = pre; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = sub; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
void TIM4_IRQHandler(void){ //100ms
    char     temp[100] = {0};
    unsigned int value = 0;
    if(TIM_GetITStatus(TIM4,TIM_IT_Update)==SET) //溢出中断
    {
        TIM_ClearITPendingBit(TIM4,TIM_IT_Update);  //清除中断标志位
        if(device_dataformat == 0){
            sprintf(temp," %.3f\r\n", pri_sv);//Aml
            USART1_Send_Data(temp,strlen(temp));
        }else if(device_dataformat == 1){
            value = (unsigned int)(pri_sv*1000);
            sprintf(temp, "%d\r\n",value);//Valeport
            USART1_Send_Data(temp,strlen(temp));
        }else if(device_dataformat == 2){
            sprintf(temp, "$PRSOS,%.3f*78\r\n", pri_sv);//NMEA
            USART1_Send_Data(temp,strlen(temp));
        }else if(device_dataformat == 3){
            sprintf(temp, "S:%.3f,F:%.3f,K:%.3f,P:%.3f,A:%.3f,B:%.3f,I:%.2f%%,FV:%.3f,TV:%.3f,T:%fus -- AVE:%f\r\n",
				True_sv,
				Pretend_sv,
				kalman_sv,
				pri_sv,
				smooth.current_coe_a,
				smooth.current_coe_b,
				PW1STvalue,
				tdc_pse.F_value,
				tdc_pse.T_value,
				echo_times,
                Tdc_Ave_func(&svave,pri_sv)
			);
            //sprintf(temp, "#%.3f\r\n", sound_velocity); //0.00000 1498.00
            USART1_Send_Data(temp,strlen(temp));
            
        }else if(device_dataformat == 4){
            sprintf(temp, "%.3f,%.3f,%.3f\r\n",True_sv,Pretend_sv,pri_sv)                                ;
            USART1_Send_Data(temp,strlen(temp));
        }else{
            u1_printf("%d\n",device_dataformat);
        }
    }
}


void TIM2_IRQHandler(void)
{  
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET) //溢出中断
	{    
        TIM_ClearITPendingBit(TIM2,TIM_IT_Update);  //清除中断标志位
        kalman_sound_velocity_posprocessor();
    }
}


//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
    static int uartLastLen;
    //static int runLedDelay = 0;
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{
        TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
        IWDG_Feed(); 
		
		if(tdc_crc_flag){
			zijiancount++;
			//1s
			if(zijiancount%20 == 0){
				readly_flag  = 1;
			}
		}
		
		char temp[1024];
			svzj++;
			if(svzj % 20 == 0){
				if(pri_sv == 0.0f && echo_times == 0.0f){
					printf(" 换能器异常");
					reset_count++;
					if(reset_count == 10){
						reset_count = 0;
						__disable_irq();    //屏蔽系统中断
			
						NVIC_SystemReset();     //重启系统 -> 进入boot程序
					}
				}else{
					//sprintf(temp, "传感器正常\r\n");
					//USART1_Send_Data(temp,strlen(temp));
				}
			}
		
        if((0 == serial_read_over) && (0 != s_Recvlength) && (uartLastLen == s_Recvlength))
        {
            serial_read_over = 1;
        }
        uartLastLen = s_Recvlength;
	}
	
}

void PauseTimer(TIM_TypeDef* TIMx) {
    TIMx->CR1 &= ~TIM_CR1_CEN; 
}

void ResumeTimer(TIM_TypeDef* TIMx) {
    TIMx->CR1 |= TIM_CR1_CEN; 
}


