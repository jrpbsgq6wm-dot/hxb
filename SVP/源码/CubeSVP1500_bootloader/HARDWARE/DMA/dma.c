#include "dma.h"

DMA_HandleTypeDef USART1TxDMA_Handler;


void USART1_DMA_Init(void){
    __HAL_RCC_DMA2_CLK_ENABLE();    //enable dma
    //配置dma
    //Tx DMA 配置
    //关联dma到usart1
    __HAL_LINKDMA(&UART1_Handler,hdmarx,USART1TxDMA_Handler);
    
     USART1TxDMA_Handler.Instance=DMA2_Stream2; //数据流选择
     USART1TxDMA_Handler.Init.Channel=DMA_CHANNEL_4; //通道选择
     USART1TxDMA_Handler.Init.Direction=DMA_PERIPH_TO_MEMORY; //存储器到外设
     USART1TxDMA_Handler.Init.PeriphInc=DMA_PINC_DISABLE; //外设非增量模式
     USART1TxDMA_Handler.Init.MemInc=DMA_MINC_ENABLE; //存储器增量模式
     USART1TxDMA_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_BYTE; 
    //外设数据长度:8 位
     USART1TxDMA_Handler.Init.MemDataAlignment=DMA_MDATAALIGN_BYTE; 
     //存储器数据长度:8 位
     USART1TxDMA_Handler.Init.Mode=DMA_NORMAL; //外设普通模式
     USART1TxDMA_Handler.Init.Priority=DMA_PRIORITY_HIGH; //中等优先级
     USART1TxDMA_Handler.Init.FIFOMode=DMA_FIFOMODE_DISABLE; 
     USART1TxDMA_Handler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL; 
     USART1TxDMA_Handler.Init.MemBurst=DMA_MBURST_SINGLE;//存储器突发单次传输
     USART1TxDMA_Handler.Init.PeriphBurst=DMA_PBURST_SINGLE; //外设突发单次传输
     
     HAL_DMA_Init(&USART1TxDMA_Handler);
        //dma NVIC
        HAL_NVIC_SetPriority(DMA2_Stream2_IRQn,0,0);	
        HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);		
}


void DMA2_Stream2_IRQHandler(void){
    HAL_DMA_IRQHandler(&USART1TxDMA_Handler);
}



