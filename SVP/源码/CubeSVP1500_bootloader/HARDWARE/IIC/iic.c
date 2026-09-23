#include "iic.h"
#include "usart.h"

I2C_HandleTypeDef IIC_Config;    //iic句柄

/*iic初始化*/
void IIC_Init(void){
    
    IIC_Config.Instance = I2C1;
    IIC_Config.Init.ClockSpeed = 100000;    //iic时钟速度
    IIC_Config.Init.DutyCycle = I2C_DUTYCYCLE_2;   //设置占空比
    IIC_Config.Init.OwnAddress1 = 0X00; //主机地址
    IIC_Config.Init.OwnAddress2 = 0x00; //双地址模式禁用时无效
    IIC_Config.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;   //七位地址模式
    IIC_Config.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; //双地址模式禁用
    IIC_Config.Init.GeneralCallMode = I2C_DUALADDRESS_DISABLE; //禁用通道调用模式
    IIC_Config.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;  //禁用时钟延迟
        if (HAL_I2C_Init(&IIC_Config) != HAL_OK){
    Error_Handler();
    }
}

/*
    iic初始化函数HAL_I2C_Init 回调函数
    GPIO NVIC COLCK
*/
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c){
    GPIO_InitTypeDef GPIO_Config;
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    //IIC1 SDA  PB7   SCL  PB6
    GPIO_Config.Pin = GPIO_PIN_7 |GPIO_PIN_6;
    GPIO_Config.Mode = GPIO_MODE_AF_OD; 
    GPIO_Config.Pull = GPIO_PULLUP;
    GPIO_Config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Config.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB,&GPIO_Config);
}


void Error_Handler(void){
    printf("eeprom init error\r\n");
}


