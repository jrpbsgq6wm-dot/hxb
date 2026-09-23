   /**
 ****************************************************************************************************
 * @file        eeprom.h
 * @author            ()
 * @version     V1.0
 * @date        2026-07-17
 * @brief       BL24C128AE0-PARC EEPROM       
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 *           I2C1          SCL = PB6, SDA = PB9   
 *   - I2C          : 0xA0
 *   - I2C          : 0xA1
 *   - 7               : 0x50
 *   -       : 128Kbit = 16KByte                0x0000 ~ 0x3FFF
 *
 ****************************************************************************************************
 */

#ifndef __EEPROM_H
#define __EEPROM_H

#include "./SYSTEM/sys/sys.h"

/* BL24C128AE0-PARC EEPROM       
 * 7         : 0x50         1      :
 *            : 0xA0
 *            : 0xA1
 */
#define BL24C128_ADDR_WRITE             0xA0
#define BL24C128_ADDR_READ              0xA1
#define BL24C128_PAGE_SIZE              64          /*          : 64       */
#define BL24C128_MAX_ADDR               0x3FFF      /*             : 16K-1 */

/*        I2C1                    I2C.c    */
extern I2C_HandleTypeDef hi2c1;

/*              */
HAL_StatusTypeDef bl24c128_write_byte(uint16_t addr, uint8_t data);    /*                 */
HAL_StatusTypeDef bl24c128_read_byte(uint16_t addr, uint8_t *data);    /*                 */
HAL_StatusTypeDef bl24c128_write_page(uint16_t addr, uint8_t *data, uint16_t len);  /*           */
HAL_StatusTypeDef bl24c128_read(uint16_t addr, uint8_t *data, uint16_t len);        /*              */
HAL_StatusTypeDef bl24c128_check(uint16_t addr);    /*        EEPROM              */
void bl24c128_test(void);                           /* EEPROM              */

#endif
