#include "interrupt.h"
#include "ymodem.h"
#include "main.h"
#include "stmflash.h"
#include "delay.h"
#include "led.h"

void USART1_IRQHandler(void){
    if(USART_GetITStatus(USART1,USART_IT_IDLE) != RESET){
        //清中断
        USART1->SR;
        USART1->DR;
        DMA_Cmd(DMA1_Channel5,DISABLE);
        UART1_RxCounter = UART1_RXBUFF_SIZE - DMA1_Channel5->CNDTR;
        memcpy(temp_value,UART1_RxBuff,sizeof(UART1_RxBuff));
        memset(UART1_RxBuff,0,sizeof(UART1_RxBuff));
        ymodem_flag = 1;    //串口接收到数据 - 影响标志位 - 判断是否为程序更新数据   
        DMA_SetCurrDataCounter(DMA1_Channel5,UART1_RXBUFF_SIZE);
        DMA_Cmd(DMA1_Channel5,ENABLE);
      
    }
}











