   /**
 ****************************************************************************************************
 * @file        eeprom.c
 * @author            ()
 * @version     V1.0
 * @date        2026-07-17
 * @brief       BL24C128AE0-PARC EEPROM       
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 *           I2C1          SCL = PB6, SDA = PB9   
 *        STM32F4        I2C1       
 *
 ****************************************************************************************************
 */

#include "./BSP/EEPROM/eeprom.h"
#include "./SYSTEM/delay/delay.h"
#include "stdio.h"

/**
 * @brief           BL24C128                                  
 * @param       addr:                0x0000 ~ 0x3FFF   
 * @param       data:                   
 * @retval      HAL_OK:       ,       :       
 * @note                                                         5ms   
 */
HAL_StatusTypeDef bl24c128_write_byte(uint16_t addr, uint8_t data)
{
    HAL_StatusTypeDef ret;

    /*        HAL Mem_Write            16    */
    ret = HAL_I2C_Mem_Write(&hi2c1, BL24C128_ADDR_WRITE, addr,
                            I2C_MEMADD_SIZE_16BIT, &data, 1, HAL_MAX_DELAY);

    /*        EEPROM                       */
    delay_ms(10);

    return ret;
}

/**
 * @brief           BL24C128                                  
 * @param       addr:             0x0000 ~ 0x3FFF   
 * @param       data:                         
 * @retval      HAL_OK:       ,       :       
 */
HAL_StatusTypeDef bl24c128_read_byte(uint16_t addr, uint8_t *data)
{
    return HAL_I2C_Mem_Read(&hi2c1, BL24C128_ADDR_READ, addr,
                            I2C_MEMADD_SIZE_16BIT, data, 1, HAL_MAX_DELAY);
}

/**
 * @brief           BL24C128                                64          
 * @param       addr:                                               addr % 64 == 0   
 * @param       data:                            
 * @param       len:                           64          
 * @retval      HAL_OK:       ,       :       
 * @note                                                  
 */
HAL_StatusTypeDef bl24c128_write_page(uint16_t addr, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret;

    /*              */
    if (len > BL24C128_PAGE_SIZE)
    {
        len = BL24C128_PAGE_SIZE;
    }

    /*           */
    ret = HAL_I2C_Mem_Write(&hi2c1, BL24C128_ADDR_WRITE, addr,
                            I2C_MEMADD_SIZE_16BIT, data, len, HAL_MAX_DELAY);

    /*        EEPROM                       */
    delay_ms(10);

    return ret;
}

/**
 * @brief           BL24C128                   
 * @param       addr:             
 * @param       data:                      
 * @param       len:                       
 * @retval      HAL_OK:       ,       :       
 */
HAL_StatusTypeDef bl24c128_read(uint16_t addr, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c1, BL24C128_ADDR_READ, addr,
                            I2C_MEMADD_SIZE_16BIT, data, len, HAL_MAX_DELAY);
}

/**
 * @brief              BL24C128 EEPROM             
 * @param       test_addr:                             0x0000                               
 * @retval      HAL_OK:       ,       :             
 * @note                           0x55                                          
 */
HAL_StatusTypeDef bl24c128_check(uint16_t test_addr)
{
    uint8_t temp;
    uint8_t original;
    HAL_StatusTypeDef ret;

    /*                    */
    ret = bl24c128_read_byte(test_addr, &original);
    if (ret != HAL_OK) return ret;

    /*                 0x55 */
    ret = bl24c128_write_byte(test_addr, 0x55);
    if (ret != HAL_OK) return ret;

    /*                 */
    ret = bl24c128_read_byte(test_addr, &temp);
    if (ret != HAL_OK) return ret;

    /*                 */
    bl24c128_write_byte(test_addr, original);

    if (temp == 0x55)
    {
        return HAL_OK;      /*              */
    }

    return HAL_ERROR;       /*              */
}

/**
 * @brief       EEPROM                                                        printf                
 * @param          
 * @retval         
 * @note                        0x0000        "startesteerpom"                        
 */
void bl24c128_test(void)
{
    uint8_t wr_buf[] = "startesteerpom";
    uint8_t rd_buf[32] = {0};
    uint16_t eeprom_addr = 0x0000;
    uint8_t i;

    /*                       */
    printf("eeprom write:");
    for (i = 0; i < sizeof(wr_buf) - 1; i++)
    {
        if (bl24c128_write_byte(eeprom_addr + i, wr_buf[i]) != HAL_OK)
        {
            break;
        }
    }
    if (i == sizeof(wr_buf) - 1)
    {
        printf("ok\r\n");
    }
    else
    {
        printf("fail(addr=0x%04X err=%lu)\r\n", eeprom_addr + i, HAL_I2C_GetError(&hi2c1));
    }

    delay_ms(10);

    printf("eeprom read:");
    if (bl24c128_read(eeprom_addr, rd_buf, sizeof(wr_buf) - 1) != HAL_OK)
    {
        printf("fail(err=%lu)\r\n", HAL_I2C_GetError(&hi2c1));
    }
    else
    {
        rd_buf[sizeof(wr_buf) - 1] = '\0';
        printf("'%s'\r\n", (char *)rd_buf);
    }
}
