   /**
 ****************************************************************************************************
 * @file        ethernet.h
 * @author                  ()
 * @version     V1.0
 * @date        2021-12-02
 * @brief       ETHERNET         
 * @license     Copyright (c) 2020-2032,                           
 ****************************************************************************************************
 * @attention
 *
 *         :                F407      
 *         :www.yuanzige.com
 *         :www.openedv.com
 *         :www..com
 *        :openedv.taobao.com
 *
 *         
 * V1.0 20211202
 *           
 *
 ****************************************************************************************************
 */
 
#ifndef __ETHERNET_H
#define __ETHERNET_H
#include "./SYSTEM/sys/sys.h"
#include "stm32f4xx_hal_conf.h"


/******************************************************************************************/
/*           */

#define ETH_CLK_GPIO_PORT               GPIOA
#define ETH_CLK_GPIO_PIN                GPIO_PIN_1
#define ETH_CLK_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOA_CLK_ENABLE();}while(0)                  /*     IO           */

#define ETH_MDIO_GPIO_PORT              GPIOA
#define ETH_MDIO_GPIO_PIN               GPIO_PIN_2
#define ETH_MDIO_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOA_CLK_ENABLE();}while(0)                 /*     IO           */

#define ETH_CRS_GPIO_PORT               GPIOA
#define ETH_CRS_GPIO_PIN                GPIO_PIN_7
#define ETH_CRS_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOA_CLK_ENABLE();}while(0)                  /*     IO           */

#define ETH_MDC_GPIO_PORT               GPIOC
#define ETH_MDC_GPIO_PIN                GPIO_PIN_1
#define ETH_MDC_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOC_CLK_ENABLE();}while(0)                  /*     IO           */

#define ETH_RXD0_GPIO_PORT              GPIOC
#define ETH_RXD0_GPIO_PIN               GPIO_PIN_4
#define ETH_RXD0_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOC_CLK_ENABLE();}while(0)                  /*     IO           */

#define ETH_RXD1_GPIO_PORT              GPIOC
#define ETH_RXD1_GPIO_PIN               GPIO_PIN_5
#define ETH_RXD1_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOC_CLK_ENABLE();}while(0)                  /*     IO           */

#define ETH_TX_EN_GPIO_PORT             GPIOB
#define ETH_TX_EN_GPIO_PIN              GPIO_PIN_11
#define ETH_TX_EN_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOB_CLK_ENABLE();}while(0)                  /*     IO           */

#define ETH_TXD0_GPIO_PORT              GPIOB
#define ETH_TXD0_GPIO_PIN               GPIO_PIN_12
#define ETH_TXD0_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOB_CLK_ENABLE();}while(0)                  /*     IO           */

#define ETH_TXD1_GPIO_PORT              GPIOB
#define ETH_TXD1_GPIO_PIN               GPIO_PIN_13
#define ETH_TXD1_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOB_CLK_ENABLE();}while(0)                  /*     IO           */

#define ETH_RESET_GPIO_PORT             GPIOB
#define ETH_RESET_GPIO_PIN              GPIO_PIN_0
#define ETH_RESET_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOB_CLK_ENABLE();}while(0)                  /*     IO           */


/******************************************************************************************/

/* ETH         */
#define ETHERNET_RST(x)  do{ x ? \
                            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET) : \
                            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); \
                        }while(0)



extern ETH_HandleTypeDef g_eth_handler;                                 /*           */
extern ETH_DMADescTypeDef *g_eth_dma_rx_dscr_tab;                       /*       DMA                         */
extern ETH_DMADescTypeDef *g_eth_dma_tx_dscr_tab;                       /*       DMA                         */
extern uint8_t *g_eth_rx_buf;                                           /*                   buffers     */
extern uint8_t *g_eth_tx_buf;                                           /*                   buffers     */

uint8_t ethernet_init(void);                                            /*                  */
uint32_t ethernet_read_phy(uint16_t reg);                               /*                        */
void ethernet_write_phy(uint16_t reg, uint16_t value);                  /*                                 */
uint8_t ethernet_chip_get_speed(void);                                  /*                         */
uint32_t  ethernet_get_eth_rx_size(ETH_DMADescTypeDef *dma_rx_desc);    /*                    */

uint8_t ethernet_mem_malloc(void);                                           /*   ETH                 */
void ethernet_mem_free(void);                                                /*     ETH                   */
#endif

