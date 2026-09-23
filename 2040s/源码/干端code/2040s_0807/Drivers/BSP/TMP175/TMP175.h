   /**
 ****************************************************************************************************
 * @file        TMP175.h
 * @author            ()
 * @version     V1.0
 * @date        2026-07-17
 * @brief       TMP175AIDGKR                      
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 *           I2C2       
 *   - I2C          : 0x90
 *   - I2C          : 0x91
 *   - 7               : 0x48
 *
 ****************************************************************************************************
 */

#ifndef __TMP175_H
#define __TMP175_H

#include "./SYSTEM/sys/sys.h"

/* TMP175                 */
#define TMP175_PTR_TEMP                 0x00        /*                       */
#define TMP175_PTR_CONFIG               0x01        /*                       */
#define TMP175_PTR_TLOW                 0x02        /*                       */
#define TMP175_PTR_THIGH                0x03        /*                       */

/* I2C          7          0x48       1       */
#define TMP175_ADDR_WRITE               0x90
#define TMP175_ADDR_READ                0x91

/*        I2C2                    I2C.c    */
extern I2C_HandleTypeDef hi2c1;

/*              */
HAL_StatusTypeDef tmp175_init(void);                            /*           TMP175                         */
HAL_StatusTypeDef tmp175_read_temp_raw(int16_t *temp);          /*                               : 0.0625  C    */
float tmp175_convert_to_celsius(int16_t raw);                   /*                       */
HAL_StatusTypeDef tmp175_read_temp_celsius(float *celsius);     /*                             */
HAL_StatusTypeDef tmp175_get_temp_string(char *buf, uint8_t len); /*                             ASCII           */
void tmp175_timer_init(void);                                   /*           TIM6             1                                */
void tmp175_uart6_print_temperature(void);                      /*                      USART6       */

#endif
