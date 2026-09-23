   /**
 ****************************************************************************************************
 * @file        I2C.h
 * @author            ()
 * @version     V1.0
 * @date        2026-07-15
 * @brief       I2C                             STM32F4        I2C2          
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 * I2C2       : SCL = PA8 (AF4), SDA = PC9 (AF4)
 *              I2C              GPIO       
 *
 *             :
 *   1. TMP175AIDGKR                
 *      -          : 0x90,          : 0x91
 *
 ****************************************************************************************************
 */

#ifndef __I2C_H
#define __I2C_H

#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/* I2C1              */

#define I2C1_SCL_GPIO_PORT              GPIOB
#define I2C1_SCL_GPIO_PIN               GPIO_PIN_6
#define I2C1_SCL_GPIO_AF                GPIO_AF4_I2C1
#define I2C1_SCL_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define I2C1_SDA_GPIO_PORT              GPIOB
#define I2C1_SDA_GPIO_PIN               GPIO_PIN_9
#define I2C1_SDA_GPIO_AF                GPIO_AF4_I2C1
#define I2C1_SDA_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

/******************************************************************************************/

/*              */
extern I2C_HandleTypeDef hi2c1;                     /* I2C1        */

/*                    */
void i2c1_init(void);                               /*           I2C1              */

#endif
