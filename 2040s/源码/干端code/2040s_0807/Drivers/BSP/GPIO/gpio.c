   /**
 ****************************************************************************************************
 * @file        gpio.c
 * @author            ()
 * @version     V1.0
 * @date        2026-07-16
 * @brief       GPIO                SW_RESET       
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

#include "./BSP/GPIO/gpio.h"
#include "./SYSTEM/delay/delay.h"

/**
 * @brief                 SW_RESET(PE2)                                  
 * @param          
 * @retval         
 */
void sw_reset_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    SW_RESET_GPIO_CLK_ENABLE();                                     /* PE              */

    gpio_init_struct.Pin = SW_RESET_GPIO_PIN;                       /* PE2 */
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;                    /*              */
    gpio_init_struct.Pull = GPIO_PULLUP;                            /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                  /*        */
    HAL_GPIO_Init(SW_RESET_GPIO_PORT, &gpio_init_struct);           /*           PE2 */

    SW_RESET_LOW();                                                 /*                 */
}

/**
 * @brief       SW_RESET                   
 * @param          
 * @retval         
 * @note            SW_RESET(PE2)        500ms          
 */
void SW(void)
{
    SW_RESET_LOW();             /*        */
    delay_ms(500);              /*       500ms */
    SW_RESET_HIGH();            /*        */
}

/**
 * @brief       Initialize SONA_POWER_EN(PD0), push-pull output, default low
 * @param       none
 * @retval      none
 */
void sona_power_en_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    SONA_POWER_EN_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin = SONA_POWER_EN_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SONA_POWER_EN_GPIO_PORT, &gpio_init_struct);

    SONA_POWER_EN_LOW();
}

/**
 * @brief       Set SONA_POWER_EN output level
 * @param       en: 0=low, 1=high
 * @retval      none
 */
void sona_power_en_set(uint8_t en)
{
    if (en)
        SONA_POWER_EN_HIGH();
    else
        SONA_POWER_EN_LOW();
}
