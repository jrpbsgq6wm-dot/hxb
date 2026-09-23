   /**
 ****************************************************************************************************
 * @file        I2C.c
 * @author            ()
 * @version     V1.0
 * @date        2026-07-17
 * @brief       I2C                             STM32F4        I2C1          
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 * I2C1       : SCL = PB6 (AF4), SDA = PB9 (AF4)
 *              I2C              GPIO       
 *
 *             :
 *   1. TMP175AIDGKR -                 (0x90/0x91)
 *
 ****************************************************************************************************
 */

#include "./BSP/I2C/I2C.h"


/* I2C1              */
I2C_HandleTypeDef hi2c1;


/**
 * @brief                 I2C1             
 * @param          
 * @retval         
 * @note        I2C1       : SCL=PB6, SDA=PB9
 *                           I2C       GPIO       
 *                       : 100kHz                  
 */
void i2c1_init(void)
{
    hi2c1.Instance = I2C1;                          /* I2C1 */
    hi2c1.Init.ClockSpeed = 100000;                 /* 100kHz              */
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;         /* 50%           */
    hi2c1.Init.OwnAddress1 = 0;                     /*                                      */
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;  /* 7                */
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; /*                       */
    hi2c1.Init.OwnAddress2 = 0;                     /*                      2 */
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; /*                    */
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;    /*                    */
    HAL_I2C_Init(&hi2c1);                           /*           I2C1 */
}


/**
 * @brief       I2C                             HAL             
 * @param       hi2c: I2C             
 * @retval         
 * @note                        HAL_I2C_Init()       
 *                                    GPIO       
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (hi2c->Instance == I2C1)
    {
        /*        I2C1        */
        __HAL_RCC_I2C1_CLK_ENABLE();

        /*        GPIO        */
        I2C1_SCL_GPIO_CLK_ENABLE();                 /* GPIOB        */
        I2C1_SDA_GPIO_CLK_ENABLE();                 /* GPIOB        */

        /*        SCL (PB6)                       */
        gpio_init_struct.Pin = I2C1_SCL_GPIO_PIN;   /* PB6 */
        gpio_init_struct.Mode = GPIO_MODE_AF_OD;    /*                    */
        gpio_init_struct.Pull = GPIO_PULLUP;        /*        */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;  /*        */
        gpio_init_struct.Alternate = I2C1_SCL_GPIO_AF;  /* AF4: I2C1 */
        HAL_GPIO_Init(I2C1_SCL_GPIO_PORT, &gpio_init_struct);

        /*        SDA (PB9)                       */
        gpio_init_struct.Pin = I2C1_SDA_GPIO_PIN;   /* PB9 */
        gpio_init_struct.Alternate = I2C1_SDA_GPIO_AF;  /* AF4: I2C1 */
        HAL_GPIO_Init(I2C1_SDA_GPIO_PORT, &gpio_init_struct);
    }
}
