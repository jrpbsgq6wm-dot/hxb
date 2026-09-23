#include "usart1.h"
#include "stdarg.h"
#include "delay.h"

//定义串口1相关的变量和内存
volatile u32 UART1_RxCounter = 0;
uint8_t UART1_RxBuff[UART1_RXBUFF_SIZE]={0}; 
    
    
#if 1
#pragma import(__use_no_semihosting)
//标准库需要的支持函数
struct __FILE
{
    int handle;
};

FILE __stdout;
//定义_sys_exit()以避免使用半主机模式
void _sys_exit(int x)
{
    x = x;
}

//__use_no_semihosting was requested, but _ttywrch was
void _ttywrch(int ch)
{
    ch = ch;
}

//重定义fputc函数
int fputc(int ch, FILE *f)
{
    RS485_TX_EN = 1;   //设置为发送模式
    while ((USART1->SR & 0X40) == 0); //循环发送,直到发送完毕
    USART1->DR = (u8) ch;
    RS485_TX_EN = 0;        //设置为接收模式
    return ch;
}
#endif    
    
void UART1_Init(u32 bound){
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    USART_DeInit(USART1);  //复位串口1

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);   //使能GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);

    //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);  
    
    //使能脚，推挽输出 = 1
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;				 //PB.0 端口配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB,GPIO_Pin_0);	

    USART_InitStructure.USART_BaudRate = bound;             //波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; //字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;  //一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;     //无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; //收发模式
    USART_Init(USART1, &USART_InitStructure);               //初始化串口1
    USART_Cmd(USART1, ENABLE);  //使能串口
    
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);          //开启空闲中断
    
    //配置nvic
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1 ;//抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;      //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;         //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);
    
    //开启串口1的DMA接收和发送功能
    //USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE); //使能串口1的DMA发送功能
    USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE); //使能串口1的DMA发送功能
    
    RS485_TX_EN = 0;        //设置为接收模式
    
    //初始化串口1的DMA
    UART1DMA_Init();
}


void USART1_PutChar(uint8_t c){
    RS485_TX_EN = 1; 
    USART_SendData(USART1,c);
    while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET){
        
    }
    RS485_TX_EN = 0; 
}

void USART1_PutStr(uint8_t* s){
    while(*s != '\0'){
        USART1_PutChar(*s);
        s++;
    }
}


uint8_t USART1_RecvData(uint8_t* data){
    if(USART_GetFlagStatus(USART1,USART_FLAG_RXNE)!=RESET){
        *data = (uint8_t)USART1->DR;
        return 1;
    }else{
        return 0;
    }
}

//串口1,printf 函数
//确保一次发送数据不超过USART1_MAX_SEND_LEN字节
void u1_printf(char *fmt, ...)
{
    u16 i, j;
    char *buf;
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    i = strlen((const char *)buf); //此次发送数据的长度
    RS485_TX_EN = 1;   //设置为发送模式
    for (j = 0; j < i; j++) //循环发送数据
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); //循环发送,直到发送完毕
        USART_SendData(USART1, (uint8_t)buf[j]);
    }
    RS485_TX_EN = 0;        //设置为接收模式
}

void USART1_Send_Data(void *sendBuf, u16 len)
{
    u16 i = 0;
    u8 *buf;
    buf = (u8 *)sendBuf;
    RS485_TX_EN = 1;   //设置为发送模式
    delay_us(50);
    for (i = 0; i < len; i++)   //循环发送数据
    {
        USART1->DR = buf[i];
        while (RESET == USART_GetFlagStatus(USART1, USART_FLAG_TC));//等待发送结束
        //while ((UART4->SR & 0X40) == 0);    
    }
    RS485_TX_EN = 0;        //设置为接收模式
}
