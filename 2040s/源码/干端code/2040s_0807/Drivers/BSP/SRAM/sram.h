   /**
 ****************************************************************************************************
 * @file        sram.c
 * @author                  ()
 * @version     V1.0
 * @date        2021-11-03
 * @brief           SRAM         
 * @license     Copyright (c) 2020-2032,                           
 ****************************************************************************************************
 * @attention
 *
 *         :         STM32F407      
 *         :www.yuanzige.com
 *         :www.openedv.com
 *         :www..com
 *         :openedv.taobao.com
 *
 *         
 * V1.0 20211103
 *           
 *
 ****************************************************************************************************
 */

#ifndef __SRAM_H
#define __SRAM_H

#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/* SRAM WR/RD/CS           
 * SRAM_D0~D15          ,            ,                ,      SRAM_init        .                ,
 *         3  IO  ,       SRAM_init                              IO  .
 */

#define SRAM_WR_GPIO_PORT               GPIOD
#define SRAM_WR_GPIO_PIN                GPIO_PIN_5
#define SRAM_WR_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)   /*     IO           */

#define SRAM_RD_GPIO_PORT               GPIOD
#define SRAM_RD_GPIO_PIN                GPIO_PIN_4
#define SRAM_RD_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)   /*     IO           */

/* SRAM_CS(        SRAM_FSMC_NEX          IO  )           */
#define SRAM_CS_GPIO_PORT                GPIOG
#define SRAM_CS_GPIO_PIN                 GPIO_PIN_10
#define SRAM_CS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)   /*     IO           */

/* FSMC              
 *     :               FSMC  3      SRAM,   1  4      : FSMC_NE1~4
 *
 *     SRAM_FSMC_NEX,       SRAM_CS_GPIO              
 */
#define SRAM_FSMC_NEX           3         /*     FSMC_NE3  SRAM_CS,              : 1~4 */

#define SRAM_FSMC_BCRX          FSMC_Bank1->BTCR[(SRAM_FSMC_NEX - 1) * 2]       /* BCR      ,    SRAM_FSMC_NEX         */
#define SRAM_FSMC_BTRX          FSMC_Bank1->BTCR[(SRAM_FSMC_NEX - 1) * 2 + 1]   /* BTR      ,    SRAM_FSMC_NEX         */
#define SRAM_FSMC_BWTRX         FSMC_Bank1E->BWTR[(SRAM_FSMC_NEX - 1) * 2]      /* BWTR      ,    SRAM_FSMC_NEX         */

/******************************************************************************************/

/* SRAM      ,      SRAM_FSMC_NEX                     
 *             FSMC    1(BANK1)      SRAM,   1                256MB,      4  :
 *       1(FSMC_NE1)        : 0X6000 0000 ~ 0X63FF FFFF
 *       2(FSMC_NE2)        : 0X6400 0000 ~ 0X67FF FFFF
 *       3(FSMC_NE3)        : 0X6800 0000 ~ 0X6BFF FFFF
 *       4(FSMC_NE4)        : 0X6C00 0000 ~ 0X6FFF FFFF
 */
#define SRAM_BASE_ADDR         (0X60000000 + (0X4000000 * (SRAM_FSMC_NEX - 1)))

extern SRAM_HandleTypeDef g_sram_handler;    /* SRAM     */


void sram_init(void);
void sram_write(uint8_t *pbuf, uint32_t addr, uint32_t datalen);
void sram_read(uint8_t *pbuf, uint32_t addr, uint32_t datalen);

uint8_t sram_test_read(uint32_t addr);
void sram_test_write(uint32_t addr, uint8_t data);

#endif
