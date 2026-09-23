#include "spi.h"

//单片机的硬件连接接入了SPI2
SPI_HandleTypeDef SPI1_Config;  //SPI1 TDC句柄
SPI_HandleTypeDef SPI2_Config;  //SPI2 ADC句柄
SPI_HandleTypeDef SPI3_Config;  //SPI3 flash句柄

void SPI2_Init(void){

    SPI2_Config.Instance = SPI2;    //SPI1
    SPI2_Config.Init.Mode = SPI_MODE_MASTER;    //设置SPI工作模式 主从模式
    SPI2_Config.Init.Direction = SPI_DIRECTION_2LINES;  //设置SPI双向模式
    SPI2_Config.Init.DataSize = SPI_DATASIZE_8BIT;  //设置SPI的发送接收8位帧结构
    SPI2_Config.Init.CLKPolarity = SPI_POLARITY_HIGH;   //串行同步时钟的空闲状态为高电平
    SPI2_Config.Init.CLKPhase = SPI_PHASE_2EDGE;    //串行同步时钟的第二个跳变沿数据被采样
    SPI2_Config.Init.NSS = SPI_NSS_SOFT;    //
    SPI2_Config.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256; //定义波特率预分频256
    SPI2_Config.Init.FirstBit = SPI_FIRSTBIT_MSB;   //MSB:高位数据在前 LSB低位数据在前
    SPI2_Config.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;   //关闭硬件CRC校验
    SPI2_Config.Init.CRCPolynomial = 0; //crc值计算的多项式

    HAL_SPI_Init(&SPI2_Config); //spi初始化
    
    __HAL_SPI_ENABLE(&SPI2_Config); //使能SPI
}

void SPI1_Init(void){
    SPI1_Config.Instance = SPI1;    //SPI1
    SPI1_Config.Init.Mode = SPI_MODE_MASTER;    //设置SPI工作模式 主从模式
    SPI1_Config.Init.Direction = SPI_DIRECTION_2LINES;  //设置SPI双向模式
    SPI1_Config.Init.DataSize = SPI_DATASIZE_8BIT;  //设置SPI的发送接收8位帧结构
    SPI1_Config.Init.CLKPolarity = SPI_POLARITY_LOW;   //串行同步时钟的空闲状态为高电平
    SPI1_Config.Init.CLKPhase = SPI_PHASE_2EDGE;    //串行同步时钟的第二个跳变沿数据被采样
    SPI1_Config.Init.NSS = SPI_NSS_SOFT;    //
    SPI1_Config.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32; //定义波特率预分频256
    SPI1_Config.Init.FirstBit = SPI_FIRSTBIT_MSB;   //MSB:高位数据在前 LSB低位数据在前
    SPI1_Config.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;   //关闭硬件CRC校验
    SPI1_Config.Init.CRCPolynomial = 7; //crc值计算的多项式

    HAL_SPI_Init(&SPI1_Config); //spi初始化
    
    __HAL_SPI_ENABLE(&SPI1_Config); //使能SPI
}



void SPI3_Init(void){
    SPI3_Config.Instance = SPI3;    //SPI3
    SPI3_Config.Init.Mode = SPI_MODE_MASTER;    //设置SPI工作模式 主从模式
    SPI3_Config.Init.Direction = SPI_DIRECTION_2LINES;  //设置SPI双向模式
    SPI3_Config.Init.DataSize = SPI_DATASIZE_8BIT;  //设置SPI的发送接收8位帧结构
    
    SPI3_Config.Init.CLKPolarity = SPI_POLARITY_LOW;   //串行同步时钟的空闲状态为高电平
    SPI3_Config.Init.CLKPhase = SPI_PHASE_1EDGE;    //串行同步时钟的第二个跳变沿数据被采样
    
    SPI3_Config.Init.NSS = SPI_NSS_SOFT;    //硬件片选
    SPI3_Config.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2; //定义波特率预分频256
    SPI3_Config.Init.FirstBit = SPI_FIRSTBIT_MSB;   //MSB:高位数据在前 LSB低位数据在前
    SPI3_Config.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;   //关闭硬件CRC校验
    SPI3_Config.Init.CRCPolynomial = 7; //crc值计算的多项式

    if(HAL_SPI_Init(&SPI3_Config) != HAL_OK){ //spi初始化
          
    }
    
    __HAL_SPI_ENABLE(&SPI3_Config); //使能SPI
    
}

//HAL_SPI_Init函数的回调函数 - 配置引脚 时钟
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi){
    GPIO_InitTypeDef GPIO_Config;
    
    if(hspi== &SPI2_Config){    //SPI2
        __HAL_RCC_GPIOB_CLK_ENABLE();   //使能GPIOB时钟
        __HAL_RCC_SPI2_CLK_ENABLE();    //使能SPI2时钟
       /*
            NSS PB12
        
            SCK PB13
            MISO PB14
            MOSI PB15
        */
        GPIO_Config.Pin = GPIO_PIN_12;
        GPIO_Config.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_Config.Pull = GPIO_NOPULL;
        GPIO_Config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_Config);
        
        GPIO_Config.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
        GPIO_Config.Mode = GPIO_MODE_AF_PP;
        GPIO_Config.Pull = GPIO_NOPULL;
        GPIO_Config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_Config.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &GPIO_Config);
        
    }else if(hspi == &SPI1_Config){ //SPI1
        __HAL_RCC_GPIOA_CLK_ENABLE();   //使能GPIOA时钟
        __HAL_RCC_SPI1_CLK_ENABLE();    //使能SPI1时钟
        /*
            SCK PA5
            MISO PA6
            MOSI PA7
        */
        GPIO_Config.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
        GPIO_Config.Mode=GPIO_MODE_AF_PP;              //复用推挽输出
        GPIO_Config.Pull=GPIO_PULLUP;                  //上拉
        GPIO_Config.Speed=GPIO_SPEED_HIGH;             //快速   
        GPIO_Config.Alternate = GPIO_AF5_SPI1;	  	    //复用为SPI1
        HAL_GPIO_Init(GPIOA,&GPIO_Config);
        
    }else if(hspi == &SPI3_Config){ //SPI3
        __HAL_RCC_GPIOB_CLK_ENABLE();   //使能GPIOB时钟
        __HAL_RCC_SPI3_CLK_ENABLE();    //使能SPI1时钟
        /*
            NSS PA15    初始化中定义 
            SCK PB3
            MISO PB4
            MOSI PB5
        */
        
        //其他
        GPIO_Config.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
        GPIO_Config.Mode=GPIO_MODE_AF_PP;              //复用推挽输出
        GPIO_Config.Pull=GPIO_PULLUP;                  //上拉
        GPIO_Config.Speed=GPIO_SPEED_HIGH;             //快速   
        GPIO_Config.Alternate = GPIO_AF6_SPI3;	  	    //复用为SPI1
        HAL_GPIO_Init(GPIOB,&GPIO_Config);
    }
}


//SPI速度设置函数
//SPI速度=fAPB1/分频系数
//@ref SPI_BaudRate_Prescaler:SPI_BAUDRATEPRESCALER_2~SPI_BAUDRATEPRESCALER_2 256
//fAPB1时钟一般为36Mhz：
/*
    参数：SPI句柄，SPI_BaudRate_Prescaler SPI BaudRate Prescaler
*/
void SPI_SetSpeed(SPI_HandleTypeDef *hspi,u8 SPI_BaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));//判断有效性
    __HAL_SPI_DISABLE(hspi);            //关闭SPI
    hspi->Instance->CR1 &= 0XFFC7;          //位3-5清零，用来设置波特率
    hspi->Instance->CR1 |= SPI_BaudRatePrescaler;//设置SPI速度
    __HAL_SPI_ENABLE(hspi);             //使能SPI
}

//SPI2 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
/*
    参数：SPI句柄，发送的字节
*/
 u8 SPI_ReadWriteByte(SPI_HandleTypeDef *hspi,u8 TxData)
{
    u8 Rxdata;
    HAL_SPI_TransmitReceive(hspi,&TxData,&Rxdata,1, 1000);       
 	return Rxdata;          		    //返回收到的数据		
}



