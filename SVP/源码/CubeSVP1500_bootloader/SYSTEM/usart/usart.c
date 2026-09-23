/*
    STM32F4 USART CODE
    2024/12/24
    HOU XINBO
*/

#include "usart.h"
#include "delay.h"

//如果使用os,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os 使用	  
#endif
 
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
//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
	USART1->DR = (u8) ch;      
	return ch;
}
#endif 

#if EN_USART1_RX   //如果使能了接收
uint8_t tx_buffer[1029];   //数据处理发送缓冲区
uint8_t rx_buffer[1029];      //接收缓冲区
volatile uint32_t rx_length;            //接收数据长度
volatile uint8_t frame_receied;                  //接收完成标志位
UART_HandleTypeDef UART1_Handler;
/*DMA*/
uint8_t dma_buffer[1029];  //接收缓冲区
volatile uint16_t dma_rx_len;
/*mode*/
uint8_t mode_flag;


//初始化IO 串口1 
//bound:波特率
void uart_init(u32 bound)
{
    
	//UART 初始化设置
	UART1_Handler.Instance=USART1;					    //USART1
	UART1_Handler.Init.BaudRate=bound;				    //波特率
	UART1_Handler.Init.WordLength=UART_WORDLENGTH_8B;   //字长为8位数据格式
	UART1_Handler.Init.StopBits=UART_STOPBITS_1;	    //一个停止位
	UART1_Handler.Init.Parity=UART_PARITY_NONE;		    //无奇偶校验位
	UART1_Handler.Init.HwFlowCtl=UART_HWCONTROL_NONE;   //无硬件流控
	UART1_Handler.Init.Mode=UART_MODE_TX_RX;		    //收发模式
	HAL_UART_Init(&UART1_Handler);					    //HAL_UART_Init()会使能UART1
	
    //__HAL_UART_ENABLE(handler)；//使能句柄 handler 指定的串口
    //__HAL_UART_DISABLE(handler)；//失能句柄 hander 指定的串口
//    
//    //该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量
//	HAL_UART_Receive_IT(&UART1_Handler, &rx_buffer[0], 1);  
    //dma初始化
    USART1_DMA_Init();
    //使能usart1空闲中断
    __HAL_UART_ENABLE_IT(&UART1_Handler,UART_IT_IDLE);
    //启动DMA
    HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
}




//UART底层初始化，时钟使能，引脚配置，中断配置
//此函数会被HAL_UART_Init()调用
//huart:串口句柄
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    //GPIO端口设置
	GPIO_InitTypeDef GPIO_Initure;
	
	if(huart->Instance==USART1)//如果是串口1，进行串口1 MSP初始化
	{
		__HAL_RCC_GPIOA_CLK_ENABLE();			//使能GPIOA时钟
		__HAL_RCC_USART1_CLK_ENABLE();			//使能USART1时钟
	
		GPIO_Initure.Pin=GPIO_PIN_9;			//PA9
		GPIO_Initure.Mode=GPIO_MODE_AF_PP;		//复用推挽输出
		GPIO_Initure.Pull=GPIO_PULLUP;			//上拉
		GPIO_Initure.Speed=GPIO_SPEED_FAST;		//高速
		GPIO_Initure.Alternate=GPIO_AF7_USART1;	//复用为USART1
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);	   	//初始化PA9

		GPIO_Initure.Pin=GPIO_PIN_10;			//PA10
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);	   	//初始化PA10

		HAL_NVIC_SetPriority(USART1_IRQn,0,0);	//
        HAL_NVIC_EnableIRQ(USART1_IRQn);		//使能USART1中断通道
	}
}


#endif

void USART1_IRQHandler(void){
    if(__HAL_UART_GET_FLAG(&UART1_Handler,UART_FLAG_IDLE) != RESET){
        __HAL_UART_CLEAR_IDLEFLAG(&UART1_Handler);
        //停止DMA传输
        HAL_UART_DMAStop(&UART1_Handler);
        
        //计算接收到的数据长度
        dma_rx_len = RX_BUFFER_SIZE - USART1TxDMA_Handler.Instance->NDTR;
        //将接收到的数据拷贝到CMD数组下
        memcpy(rx_buffer,dma_buffer,1029);
        rx_length = dma_rx_len;
        frame_receied  = 1;
        //开启DMA
        memset(dma_buffer,0,sizeof(dma_buffer));
        HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
    }
}

HAL_StatusTypeDef Send_data(uint8_t *pdata,uint16_t size){
    return HAL_UART_Transmit(&UART1_Handler,pdata,size,HAL_MAX_DELAY);
}



void usart_bound(void){
    uart_init(115200);
}


void set_type(void){
    
}

void stm32_reset(void){
    HAL_NVIC_SystemReset();
}


/*CRC*/
uint16_t CRC_16;
uint8_t CRC_LOW;
uint8_t CRC_HIG;



/*------------------------------------------------------------------
** 函数原型: int CRC16 (const char *nData, int wLength)
** 功能描述: CRC16校验
** 输入参数: const char *nData, int wLength
** 调用模块: None
** 返 回 值: wCRCWord
** 说    明：
-------------------------------------------------------------------*/
static const int wCRCTable[] =
{
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

int CRC16(const void *_nData, uint16_t wLength)
{
    const char *nData;
    unsigned char nTemp;
    unsigned char CRCHI = 0XFF;
    unsigned char CRCLO = 0XFF;
    nData = (const char *)_nData;
    while (wLength--) 
    {
        nTemp = CRCHI ^ (*(nData++)); 	
        CRCHI = CRCLO ^ wCRCTable[nTemp]; 		
        CRCLO = wCRCTable[nTemp]>>8; 
    }
    return ((unsigned short)CRCLO << 8 | CRCHI);
} // End: CRC16

