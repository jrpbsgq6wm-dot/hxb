#include "./BSP/I2C/I2C.h"

/* I2C1 handle */
I2C_HandleTypeDef hi2c1;

/* Initialize I2C1 on PB6/PB9 at 100 kHz. */
void i2c1_init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

/* HAL MSP hook for I2C1 GPIO clock and pin setup. */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_I2C1_CLK_ENABLE();

        I2C1_SCL_GPIO_CLK_ENABLE();
        I2C1_SDA_GPIO_CLK_ENABLE();

        gpio_init_struct.Pin = I2C1_SCL_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_OD;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = I2C1_SCL_GPIO_AF;
        HAL_GPIO_Init(I2C1_SCL_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = I2C1_SDA_GPIO_PIN;
        gpio_init_struct.Alternate = I2C1_SDA_GPIO_AF;
        HAL_GPIO_Init(I2C1_SDA_GPIO_PORT, &gpio_init_struct);
    }
}
