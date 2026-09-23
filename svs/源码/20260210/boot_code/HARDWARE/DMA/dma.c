#include "dma.h"

void UART1DMA_Init(void){
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE); //使能dma时钟
    DMA_InitTypeDef DMA_Config;
  
     
    //配置DMA1的通道5作为串口1的接收
    DMA_Config.DMA_MemoryBaseAddr = (u32)UART1_RxBuff;
    DMA_Config.DMA_PeripheralBaseAddr = (u32)&USART1->DR;
    DMA_Config.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_Config.DMA_BufferSize = sizeof(UART1_RxBuff);
    DMA_Config.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_Config.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_Config.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_Config.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_Config.DMA_Mode=DMA_Mode_Normal;
    DMA_Config.DMA_Priority = DMA_Priority_High;
    DMA_Config.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5,&DMA_Config);
    DMA_Cmd(DMA1_Channel5,ENABLE);
    //开启了DMA1通道5，如果有数据到来那么通过DMA来获取数据，获取结束后会处于空闲状态，触发IDLE中断

}




