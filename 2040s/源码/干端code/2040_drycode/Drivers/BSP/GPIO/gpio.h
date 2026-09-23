   /**
 ****************************************************************************************************
 * @file        gpio.h
 * @author            ()
 * @version     V1.0
 * @date        2026-07-16
 * @brief       GPIO and SW_RESET driver
 * @license     Copyright (c) 2020-2032,
 ****************************************************************************************************
 * @attention
 *
 *             :           F407
 *             :www.yuanzige.com
 *             :www.openedv.com
 *             :www..com
 *             :openedv.taobao.com
 *
 ****************************************************************************************************
 */
#ifndef __GPIO_H
#define __GPIO_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* SW_RESET: PE2 */

#define SW_RESET_GPIO_PORT                  GPIOE
#define SW_RESET_GPIO_PIN                   GPIO_PIN_2
#define SW_RESET_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)
#define SW_RESET_LOW()                      HAL_GPIO_WritePin(SW_RESET_GPIO_PORT, SW_RESET_GPIO_PIN, GPIO_PIN_RESET)
#define SW_RESET_HIGH()                     HAL_GPIO_WritePin(SW_RESET_GPIO_PORT, SW_RESET_GPIO_PIN, GPIO_PIN_SET)

/******************************************************************************************/
/* SONA_POWER_EN: PD14, push-pull output, default low */
#define SONA_POWER_EN_GPIO_PORT                 GPIOD
#define SONA_POWER_EN_GPIO_PIN                  GPIO_PIN_14
#define SONA_POWER_EN_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)
#define SONA_POWER_EN_LOW()                     HAL_GPIO_WritePin(SONA_POWER_EN_GPIO_PORT, SONA_POWER_EN_GPIO_PIN, GPIO_PIN_RESET)
#define SONA_POWER_EN_HIGH()                    HAL_GPIO_WritePin(SONA_POWER_EN_GPIO_PORT, SONA_POWER_EN_GPIO_PIN, GPIO_PIN_SET)

/* Function declarations */
void sw_reset_init(void);
void SW(void);
void sona_power_en_init(void);
void sona_power_en_set(uint8_t en);

#endif
