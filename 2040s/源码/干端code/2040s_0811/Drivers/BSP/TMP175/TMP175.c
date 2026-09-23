   /**
 ****************************************************************************************************
 * @file        TMP175.c
 * @author            ()
 * @version     V1.0
 * @date        2026-07-18
 * @brief       TMP175AIDGKR                      
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 *           I2C2          SCL = PA8, SDA = PC9   
 *        STM32F4        I2C2       
 *
 *        TIM7                       1                                USART6       
 *
 ****************************************************************************************************
 */

#include "./BSP/TMP175/TMP175.h"
#include "./SYSTEM/usart/usart.h"
#include "stdio.h"

/* TMP175 周期采样使用的 TIM7 HAL 句柄。 */
static TIM_HandleTypeDef g_tmp175_timer;

/**
 * @brief              USART6                                                 
 * @param       str:     null                   
 * @retval         
 */
static void tmp175_uart6_send_str(const char *str)
{
    if (str == NULL)
    {
        return;
    }

    (void)usart_send((const uint8_t *)str, (uint16_t)strlen(str), HAL_MAX_DELAY);
}

/**
 * @brief                 TMP175                
 * @param          
 * @retval      HAL_OK:       ,       :       
 * @note                                             12             
 */
HAL_StatusTypeDef tmp175_init(void)
{
    uint8_t config = 0x60;  /*                      12                               */
    return HAL_I2C_Mem_Write(&hi2c1, TMP175_ADDR_WRITE, TMP175_PTR_CONFIG,
                             I2C_MEMADD_SIZE_8BIT, &config, 1, HAL_MAX_DELAY);
}

/**
 * @brief              TMP175                
 * @param       temp:                         12                     : 0.0625  C   
 * @retval      HAL_OK:       ,       :       
 * @note                                    0x00          2             
 *                          :       (  C) = raw_temp * 0.0625
 *                     : raw_temp = 400        25.0  C
 */
HAL_StatusTypeDef tmp175_read_temp_raw(int16_t *temp)
{
    uint8_t buf[2];
    HAL_StatusTypeDef ret;

    /*     TMP175                          0x00          2        */
    ret = HAL_I2C_Mem_Read(&hi2c1, TMP175_ADDR_READ, TMP175_PTR_TEMP,
                           I2C_MEMADD_SIZE_8BIT, buf, 2, HAL_MAX_DELAY);
    if (ret == HAL_OK)
    {
        /* TMP175 12                                    MSB[15:0]    12          */
        *temp = (int16_t)((buf[0] << 8) | buf[1]) >> 4;
    }

    return ret;
}

/**
 * @brief           TMP175                                  
 * @param       raw:                          tmp175_read_temp_raw   
 * @retval                  
 */
float tmp175_convert_to_celsius(int16_t raw)
{
    return raw * 0.0625f;
}

/**
 * @brief              TMP175                            
 * @param       celsius:                   
 * @retval      HAL_OK:       ,       :       
 */
HAL_StatusTypeDef tmp175_read_temp_celsius(float *celsius)
{
    int16_t raw;
    HAL_StatusTypeDef ret;

    ret = tmp175_read_temp_raw(&raw);
    if (ret == HAL_OK)
    {
        *celsius = tmp175_convert_to_celsius(raw);
    }

    return ret;
}

/**
 * @brief              TMP175                       ASCII          
 * @param       buf:                          16          
 * @param       len:                
 * @retval      HAL_OK:       ,       :                    buf                      
 * @note                        "temperature:+25.00C\r\n"       20          
 *                              '+'                   '-'
 */
HAL_StatusTypeDef tmp175_get_temp_string(char *buf, uint8_t len)
{
    int16_t raw;
    int32_t integer;
    uint16_t decimal;
    char sign = '+';

    if (len < 24)
    {
        if (len > 0) buf[0] = '\0';
        return HAL_ERROR;
    }

    if (tmp175_read_temp_raw(&raw) != HAL_OK)
    {
        /*                       HAL           */
        uint32_t err = HAL_I2C_GetError(&hi2c1);
        sprintf(buf, "temperature:error(0x%04lX)\r\n", err);
        return HAL_ERROR;
    }

    /*           =        / 0.0625 =        * 16                = raw / 16.0 */
    if (raw < 0)
    {
        sign = '-';
        raw = -raw;
    }

    integer = raw / 16;                                     /*              */
    decimal = (uint16_t)(((raw % 16) * 100 + 8) / 16);      /*                                         */

    if (decimal >= 100)
    {
        integer++;
        decimal = 0;
    }

    sprintf(buf, "temperature:%c%ld.%02uC\r\n", sign, integer, decimal);

    return HAL_OK;
}

/**
 * @brief                 TIM7             1                   
 * @param          
 * @retval         
 * @note        TIM7        = 84MHz   Prescaler = 8399     10kHz   Period = 9999     1s
 */
void tmp175_timer_init(void)
{
    __HAL_RCC_TIM7_CLK_ENABLE();                                    /* TIM7              */

    g_tmp175_timer.Instance = TIM7;
    g_tmp175_timer.Init.Prescaler = 8399;                       /* 84MHz / 8400 = 10kHz */
    g_tmp175_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_tmp175_timer.Init.Period = 9999;                          /* 10000 ticks = 1s */
    g_tmp175_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_tmp175_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&g_tmp175_timer);

    /*        TIM7_DAC                          */
    HAL_NVIC_SetPriority(TIM7_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);

    /*        TIM7                 */
    HAL_TIM_Base_Start_IT(&g_tmp175_timer);
}

/**
 * @brief       TIM7                   
 * @param          
 * @retval         
 */
void TIM7_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_tmp175_timer);
}

/**
 * @brief       TIM                          HAL     TIM7                   
 * @param       htim:                               
 * @retval         
 * @note               TMP175                 USART6       
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM7)
    {
        char temp_str[24];

        if (tmp175_get_temp_string(temp_str, sizeof(temp_str)) == HAL_OK)
        {
            tmp175_uart6_send_str(temp_str);
        }
    }
}

/**
 * @brief                            USART6                                             
 */
void tmp175_uart6_print_temperature(void)
{
    char temp_str[24];
    if (tmp175_get_temp_string(temp_str, sizeof(temp_str)) == HAL_OK)
    {
        tmp175_uart6_send_str(temp_str);
    }
}
