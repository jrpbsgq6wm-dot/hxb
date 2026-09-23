   /**
 ****************************************************************************************************
 * @file        ethernet.c
 * @author                  ()
 * @version     V1.0
 * @date        2021-10-14
 * @brief       ETHERNET         
 * @license     Copyright (c) 2020-2032,                           
 ****************************************************************************************************
 * @attention
 *
 *         :                F407      
 *         :www.yuanzige.com
 *         :www.openedv.com
 *         :www..com
 *       ?:openedv.taobao.com
 *
 *         
 * V1.0 20211014
 *           
 *
 ****************************************************************************************************
 */

#include "./BSP/ETHERNET/ethernet.h"
#include "lwip_comm.h"
#include "./SYSTEM/delay/delay.h"
#include "./MALLOC/malloc.h"


ETH_HandleTypeDef g_eth_handler;            /*          ? */
ETH_DMADescTypeDef *g_eth_dma_rx_dscr_tab;  /*       DMA                         */
ETH_DMADescTypeDef *g_eth_dma_tx_dscr_tab;  /*       DMA                         */
uint8_t *g_eth_rx_buf;                      /*                   buffers     */
uint8_t *g_eth_tx_buf;                      /*                   buffers     */


/**
 * @brief                       
 * @param         
 * @retval      0,    
 *              1,    
 */
uint8_t ethernet_init(void)
{
    uint8_t macaddress[6];

    macaddress[0] = g_lwipdev.mac[0];
    macaddress[1] = g_lwipdev.mac[1];
    macaddress[2] = g_lwipdev.mac[2];
    macaddress[3] = g_lwipdev.mac[3];
    macaddress[4] = g_lwipdev.mac[4];
    macaddress[5] = g_lwipdev.mac[5];

    g_eth_handler.Instance = ETH;
    g_eth_handler.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
    g_eth_handler.Init.Speed = ETH_SPEED_100M;
    g_eth_handler.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
    g_eth_handler.Init.PhyAddress = ETHERNET_PHY_ADDRESS;
    g_eth_handler.Init.MACAddr = macaddress;
    g_eth_handler.Init.RxMode = ETH_RXINTERRUPT_MODE;
    g_eth_handler.Init.ChecksumMode = ETH_CHECKSUM_BY_SOFTWARE;
    g_eth_handler.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;

    if (HAL_ETH_Init(&g_eth_handler) == HAL_OK)
    {
        /*     MACCR  100M            HAL  YT8512C             */
        g_eth_handler.Instance->MACCR |= ETH_SPEED_100M | ETH_MODE_FULLDUPLEX;
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * @brief       ETH                            
 *    @note               HAL_ETH_Init()    
 * @param       heth:         ?
 * @retval        
 */
void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef gpio_init_struct;

    ETH_CLK_GPIO_CLK_ENABLE();          /*     ETH_CLK     */
    ETH_MDIO_GPIO_CLK_ENABLE();         /*     ETH_MDIO     */
    ETH_CRS_GPIO_CLK_ENABLE();          /*     ETH_CRS     */
    ETH_MDC_GPIO_CLK_ENABLE();          /*     ETH_MDC     */
    ETH_RXD0_GPIO_CLK_ENABLE();         /*     ETH_RXD0     */
    ETH_RXD1_GPIO_CLK_ENABLE();         /*     ETH_RXD1     */
    ETH_TX_EN_GPIO_CLK_ENABLE();        /*     ETH_TX_EN     */
    ETH_TXD0_GPIO_CLK_ENABLE();         /*     ETH_TXD0     */
    ETH_TXD1_GPIO_CLK_ENABLE();         /*     ETH_TXD1     */
    ETH_RESET_GPIO_CLK_ENABLE();        /*     ETH_RESET     */
    __HAL_RCC_ETH_CLK_ENABLE();         /*     ETH     */


    /*              RMII    
     * ETH_MDIO -------------------------> PA2
     * ETH_MDC --------------------------> PC1
     * ETH_RMII_REF_CLK------------------> PA1
     * ETH_RMII_CRS_DV ------------------> PA7
     * ETH_RMII_RXD0 --------------------> PC4
     * ETH_RMII_RXD1 --------------------> PC5
     * ETH_RMII_TX_EN -------------------> PB11
     * ETH_RMII_TXD0 --------------------> PB12
     * ETH_RMII_TXD1 --------------------> PB13
     * ETH_RESET-------------------------> PB0
     */

    /* PA1,2,7 */
    gpio_init_struct.Pin = ETH_CLK_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /*          */
    gpio_init_struct.Pull = GPIO_NOPULL;                    /*            */
    gpio_init_struct.Speed = GPIO_SPEED_HIGH;               /*      */
    gpio_init_struct.Alternate = GPIO_AF11_ETH;             /*       ETH     */
    HAL_GPIO_Init(ETH_CLK_GPIO_PORT, &gpio_init_struct);    /* ETH_CLK             */
    
    gpio_init_struct.Pin = ETH_MDIO_GPIO_PIN;
    HAL_GPIO_Init(ETH_MDIO_GPIO_PORT, &gpio_init_struct);   /* ETH_MDIO             */
    
    gpio_init_struct.Pin = ETH_CRS_GPIO_PIN;    
    HAL_GPIO_Init(ETH_CRS_GPIO_PORT, &gpio_init_struct);    /* ETH_CRS             */

    /* PC1 */
    gpio_init_struct.Pin = ETH_MDC_GPIO_PIN;
    HAL_GPIO_Init(ETH_MDC_GPIO_PORT, &gpio_init_struct);    /* ETH_MDC       */

    /* PC4 */
    gpio_init_struct.Pin = ETH_RXD0_GPIO_PIN;
    HAL_GPIO_Init(ETH_RXD0_GPIO_PORT, &gpio_init_struct);   /* ETH_RXD0       */
    
    /* PC5 */
    gpio_init_struct.Pin = ETH_RXD1_GPIO_PIN;
    HAL_GPIO_Init(ETH_RXD1_GPIO_PORT, &gpio_init_struct);   /* ETH_RXD1       */
    
    
    /* PB11 */
    gpio_init_struct.Pin = ETH_TX_EN_GPIO_PIN;
    HAL_GPIO_Init(ETH_TX_EN_GPIO_PORT, &gpio_init_struct);  /* ETH_TX_EN       */

     /* PB12,13 */
    gpio_init_struct.Pin = ETH_TXD0_GPIO_PIN; 
    HAL_GPIO_Init(ETH_TXD0_GPIO_PORT, &gpio_init_struct);   /* ETH_TXD0       */
    
    gpio_init_struct.Pin = ETH_TXD1_GPIO_PIN; 
    HAL_GPIO_Init(ETH_TXD1_GPIO_PORT, &gpio_init_struct);   /* ETH_TXD1       */
    
    
    /*          */
    gpio_init_struct.Pin = ETH_RESET_GPIO_PIN;      /* ETH_RESET       */
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;    /*        ? */
    gpio_init_struct.Pull = GPIO_NOPULL;            /*          */
    gpio_init_struct.Speed = GPIO_SPEED_HIGH;       /*      */
    HAL_GPIO_Init(ETH_RESET_GPIO_PORT, &gpio_init_struct);

    ETHERNET_RST(0);     /*          */
    delay_ms(50);
    ETHERNET_RST(1);     /*          */

    HAL_NVIC_SetPriority(ETH_IRQn, 6, 0);           /*                          */
    HAL_NVIC_EnableIRQ(ETH_IRQn);
}

/**
 * @breif                             
 * @param       reg                  
 * @retval        
 */
uint32_t ethernet_read_phy(uint16_t reg)
{
    uint32_t regval;

    HAL_ETH_ReadPHYRegister(&g_eth_handler, reg, &regval);
    return regval;
}

/**
 * @breif                                     ?
 * @param       reg   :              ?
 * @param       value :              ?
 * @retval        
 */
void ethernet_write_phy(uint16_t reg, uint16_t value)
{
    uint32_t temp = value;
    
    HAL_ETH_WritePHYRegister(&g_eth_handler, reg, temp);
}

/**
 * @breif                 ?          
 * @param         
 * @retval      1:    100M    
                0:    
 */
uint8_t ethernet_chip_get_speed(void)
{
    uint8_t speed;
    #if(PHY_TYPE == LAN8720) 
    speed = ~((ethernet_read_phy(PHY_SR) & PHY_SPEED_STATUS));         /*   LAN8720  31                                 */
    #elif(PHY_TYPE == SR8201F)
    speed = ((ethernet_read_phy(PHY_SR) & PHY_SPEED_STATUS) >> 13);    /*   SR8201F  0                                 */
    #elif(PHY_TYPE == YT8512C)
    speed = ((ethernet_read_phy(PHY_SR) & PHY_SPEED_STATUS) >> 14);    /*   YT8512C  17                                 */
    #elif(PHY_TYPE == RTL8201)
    speed = ((ethernet_read_phy(PHY_SR) & PHY_SPEED_STATUS) >> 1);     /*   RTL8201  16                                 */
    #endif

    return speed;
}

extern void lwip_pkt_handle(void);                  /*   lwip_comm.c         */

/**
 * @breif                 
 * @param         
 * @retval        
 */
void ETH_IRQHandler(void)
{
    if (ethernet_get_eth_rx_size(g_eth_handler.RxDesc))
    {
        lwip_pkt_handle();      /*                               LWIP */
    }

    __HAL_ETH_DMA_CLEAR_IT(&g_eth_handler, ETH_DMA_IT_NIS);   /*    DMA           */
    __HAL_ETH_DMA_CLEAR_IT(&g_eth_handler, ETH_DMA_IT_R);     /*    DMA               */
}

/**
 * @breif                         
 * @param       dma_rx_desc :     DMA      
 * @retval      frameLength :               
 */
uint32_t  ethernet_get_eth_rx_size(ETH_DMADescTypeDef *dma_rx_desc)
{
    uint32_t frameLength = 0;

    if (((dma_rx_desc->Status & ETH_DMARXDESC_OWN) == (uint32_t)RESET) &&
        ((dma_rx_desc->Status & ETH_DMARXDESC_ES)  == (uint32_t)RESET) &&
        ((dma_rx_desc->Status & ETH_DMARXDESC_LS)  != (uint32_t)RESET))
    {
        frameLength = ((dma_rx_desc->Status & ETH_DMARXDESC_FL) >> ETH_DMARXDESC_FRAME_LENGTHSHIFT);
    }

    return frameLength;
}

/**
 * @breif         ETH                
 * @param         
 * @retval      0,    
 *              1,    
 */
uint8_t ethernet_mem_malloc(void)
{
    if ((g_eth_dma_rx_dscr_tab || g_eth_dma_tx_dscr_tab || g_eth_rx_buf || g_eth_tx_buf) == NULL)
    {
        g_eth_dma_rx_dscr_tab = mymalloc(SRAMIN, ETH_RXBUFNB * sizeof(ETH_DMADescTypeDef));         /*          */
        g_eth_dma_tx_dscr_tab = mymalloc(SRAMIN, ETH_TXBUFNB * sizeof(ETH_DMADescTypeDef));         /*          */
        g_eth_rx_buf = mymalloc(SRAMIN, ETH_RX_BUF_SIZE * ETH_RXBUFNB);                             /*          */
        g_eth_tx_buf = mymalloc(SRAMIN, ETH_TX_BUF_SIZE * ETH_TXBUFNB);                             /*          */
        
        if (!(uint32_t)&g_eth_dma_rx_dscr_tab || !(uint32_t)&g_eth_dma_tx_dscr_tab || !(uint32_t)&g_eth_rx_buf || !(uint32_t)&g_eth_tx_buf)
        {
            ethernet_mem_free();
            return 1;                                                                               /*          */
        }
    }

    return 0;       /*        ? */
}

/**
 * @breif           ETH                  ?
 * @param         
 * @retval        
 */
void ethernet_mem_free(void)
{
    myfree(SRAMIN, g_eth_dma_rx_dscr_tab);  /*          */
    myfree(SRAMIN, g_eth_dma_tx_dscr_tab);  /*          */
    myfree(SRAMIN, g_eth_rx_buf);           /*          */
    myfree(SRAMIN, g_eth_tx_buf);           /*          */
}
