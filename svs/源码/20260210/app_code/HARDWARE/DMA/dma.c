#include "dma.h"
#include "stdio.h"
#include "delay.h"
#include "led.h"


char UART1DMA_TxBUF[UART1DMA_TXBUF_SIZE];//定义发送缓冲区
unsigned int UART1DMA_TxCOUNT = 0;//发送数据计数
unsigned char DMA_TcFlags = 0;//数据发送未完成

char UART1DMA_RxBUF[UART1DMA_RXBUF_SIZE];//定义发送缓冲区
unsigned int UART1DMA_RxCOUNT = 0;//发送数据计数

void UART1DMA_Init(void){
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE); //使能dma时钟
    //配置DMA1通道4作为串口1的发送功能
    DMA_InitTypeDef DMA_Config;
    DMA_Config.DMA_PeripheralBaseAddr = (u32)&USART1->DR;
    DMA_Config.DMA_MemoryBaseAddr = (u32)UART1DMA_TxBUF;
    DMA_Config.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_Config.DMA_BufferSize = UART1DMA_TXBUF_SIZE;
    DMA_Config.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_Config.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_Config.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_Config.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_Config.DMA_Mode=DMA_Mode_Normal;
    DMA_Config.DMA_Priority = DMA_Priority_Medium;
    DMA_Config.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4,&DMA_Config);
    
    //配置dma使能发送完成中断
    DMA_ITConfig(DMA1_Channel4,DMA_IT_TC,ENABLE);
    
    //配置nvic的dma1中通道4的功能
    NVIC_InitTypeDef NVIC_Config;
    //配置中断源为dma1通道4
    NVIC_Config.NVIC_IRQChannel = DMA1_Channel4_IRQn;
    NVIC_Config.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_Config.NVIC_IRQChannelSubPriority = 2;
    NVIC_Config.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_Config);
    
    
    
    //配置DMA1的通道5作为串口1的接收
    DMA_Config.DMA_MemoryBaseAddr = (u32)UART1DMA_RxBUF;
    DMA_Config.DMA_PeripheralBaseAddr = (u32)&USART1->DR;
    DMA_Config.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_Config.DMA_BufferSize = UART1DMA_RXBUF_SIZE;
    DMA_Config.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_Config.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_Config.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_Config.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_Config.DMA_Mode=DMA_Mode_Normal;
    DMA_Config.DMA_Priority = DMA_Priority_High;
    DMA_Config.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5,&DMA_Config);
    //开启了DMA1通道5，如果有数据到来那么通过DMA来获取数据，获取结束后会处于空闲状态，触发IDLE中断
}

void UART1DMA_Tx_TEST(void){
    int i=0;
    for(i=0 ; i<UART1DMA_TXBUF_SIZE;i++)
        UART1DMA_TxBUF[i] = 'h';
    //先关闭DMA
    DMA_Cmd(DMA1_Channel4,DISABLE);
    //使能串口1的DMA发送
    USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);
    //指定要搬移的长度
    DMA_SetCurrDataCounter(DMA1_Channel4,UART1DMA_TXBUF_SIZE);
    //准备发送打开DMA
    DMA_Cmd(DMA1_Channel4,ENABLE);
    DMA_TcFlags = 0;
    while(1){
        if(DMA_TcFlags){
            printf("ok\n");
            break;
        }
        //LED0=~LED0;
        delay_ms(200);
    }
}

//串口1发送完成中断   
void DMA1_Channel4_IRQHandler(void){
    if(DMA_GetFlagStatus(DMA1_FLAG_TC4) != RESET){
        // 清除中断到来标志位
        DMA_ClearITPendingBit(DMA1_IT_TC4);//通道 4 传输完成中断
        //关闭dma1通道4
        DMA_Cmd(DMA1_Channel4,DISABLE);
        DMA_TcFlags = 1; //表示数据发送完成
    }
}

void UART1_DMA_SEND_DATA(char* data,uint16_t len){
    while(DMA_GetFlagStatus(DMA1_FLAG_TC4) == RESET);
    DMA_ClearFlag(DMA1_FLAG_TC4);
    DMA_Cmd(DMA1_Channel4,DISABLE);
    DMA1_Channel4->CMAR = (uint32_t)data;
    DMA1_Channel4->CNDTR = len;
    DMA_Cmd(DMA1_Channel4,ENABLE);
    USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);
}




