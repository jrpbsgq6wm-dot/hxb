#ifndef __I2C_H
#define __I2C_H

#include "./SYSTEM/sys/sys.h"

/* I2C1 pins: SCL = PB6, SDA = PB9 */
#define I2C1_SCL_GPIO_PORT GPIOB
#define I2C1_SCL_GPIO_PIN GPIO_PIN_6
#define I2C1_SCL_GPIO_AF GPIO_AF4_I2C1
#define I2C1_SCL_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

#define I2C1_SDA_GPIO_PORT GPIOB
#define I2C1_SDA_GPIO_PIN GPIO_PIN_9
#define I2C1_SDA_GPIO_AF GPIO_AF4_I2C1
#define I2C1_SDA_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

extern I2C_HandleTypeDef hi2c1;

void i2c1_init(void);

#endif
