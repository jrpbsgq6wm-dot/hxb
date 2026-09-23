   /**
 ****************************************************************************************************
 * @file        uart_Set.c
 * @author            ()
 * @version     V1.0
 * @date        2026-07-17
 * @brief       UART                            
 * @license     Copyright (c) 2020-2032,                               
 ****************************************************************************************************
 * @attention
 *
 *             :
 *   PB8  - UART1_232/422_SW                                
 *   PB1  - UART2_232/422_SW                                
 *   PD8  - UART3_232/422_SW                                
 *   PD0  - PPS_SW                                          
 *   PD4  - UART5_232_485_SW                                 
 *   PD3  - UART5_232/485_RE_DE                             
 *   PD1  - UART5_485_SD_RE_DE                              
 *   PD12 - UART5/6_TTL_SW                                  
 *
 ****************************************************************************************************
 */

#include "./BSP/uart_Set/uart_Set.h"

/**
 * @brief                 UART                   
 * @param          
 * @retval         
 * @note        PB8/PB1/PD8                                              
 *              PD0           PPS_SW                        
 *              PD4           UART5_232_485_SW                        
 *              PD3           UART5_232/485_RE_DE                        
 *              PD1           UART5_485_SD_RE_DE                        
 *              PD12           UART5/6_TTL_SW                        
 */
void uart_switch_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    /*        GPIO        */
    UART1_SW_GPIO_CLK_ENABLE();                     /* GPIOB        */
    UART2_SW_GPIO_CLK_ENABLE();                     /* GPIOB        */
    UART3_SW_GPIO_CLK_ENABLE();                     /* GPIOD        */
    PPS_SW_GPIO_CLK_ENABLE();                       /* GPIOD        */
    UART5_232_485_SW_GPIO_CLK_ENABLE();
    UART5_RE_DE_GPIO_CLK_ENABLE();                  /* GPIOD        */
    UART5_485_SD_RE_DE_GPIO_CLK_ENABLE();
    UART5_6_TTL_SW_GPIO_CLK_ENABLE();                /* GPIOD        */

    /*        PB8 - UART1_232/422_SW */
    UART1_SW_SET(GPIO_PIN_RESET);                   /*                 */
    gpio_init_struct.Pin = UART1_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;    /*              */
    gpio_init_struct.Pull = GPIO_PULLDOWN;          /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;   /*        */
    HAL_GPIO_Init(UART1_SW_GPIO_PORT, &gpio_init_struct);

    /*        PB1 - UART2_232/422_SW */
    UART2_SW_SET(GPIO_PIN_RESET);                   /*                 */
    gpio_init_struct.Pin = UART2_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;    /*              */
    gpio_init_struct.Pull = GPIO_PULLDOWN;          /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;   /*        */
    HAL_GPIO_Init(UART2_SW_GPIO_PORT, &gpio_init_struct);

    /*        PD8 - UART3_232/422_SW */
    UART3_SW_SET(GPIO_PIN_RESET);                   /*                 */
    gpio_init_struct.Pin = UART3_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;    /*              */
    gpio_init_struct.Pull = GPIO_PULLDOWN;          /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;   /*        */
    HAL_GPIO_Init(UART3_SW_GPIO_PORT, &gpio_init_struct);

    /*        PD0 - PPS_SW */
    PPS_SW_SET(GPIO_PIN_SET);                       /*                 */
    gpio_init_struct.Pin = PPS_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;    /*              */
    gpio_init_struct.Pull = GPIO_PULLDOWN;          /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;   /*        */
    HAL_GPIO_Init(PPS_SW_GPIO_PORT, &gpio_init_struct);

    /*        PD4 - UART5_232_485_SW */
    UART5_232_485_SW_SET(GPIO_PIN_SET);                   /*                 */
    gpio_init_struct.Pin = UART5_232_485_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;    /*              */
    gpio_init_struct.Pull = GPIO_PULLDOWN;          /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;   /*        */
    HAL_GPIO_Init(UART5_232_485_SW_GPIO_PORT, &gpio_init_struct);

    /*        PD3 - UART5_232/485_RE_DE */
    UART5_RE_DE_SET(GPIO_PIN_SET);                  /*                 */
    gpio_init_struct.Pin = UART5_RE_DE_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;    /*              */
    gpio_init_struct.Pull = GPIO_PULLDOWN;          /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;   /*        */
    HAL_GPIO_Init(UART5_RE_DE_GPIO_PORT, &gpio_init_struct);

    /*        PD1 - UART5_485_SD_RE_DE */
    UART5_485_SD_RE_DE_SET(GPIO_PIN_SET);                  /*                 */
    gpio_init_struct.Pin = UART5_485_SD_RE_DE_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;    /*              */
    gpio_init_struct.Pull = GPIO_PULLDOWN;          /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;   /*        */
    HAL_GPIO_Init(UART5_485_SD_RE_DE_GPIO_PORT, &gpio_init_struct);

    /*        PD12 - UART5/6_TTL_SW */
    UART5_6_TTL_SW_SET(GPIO_PIN_RESET);              /*                 */
    gpio_init_struct.Pin = UART5_6_TTL_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;    /*              */
    gpio_init_struct.Pull = GPIO_PULLDOWN;          /*        */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;   /*        */
    HAL_GPIO_Init(UART5_6_TTL_SW_GPIO_PORT, &gpio_init_struct);
}

/* UART_STAR                   10               AA DD + 7       +       DD   
 *                                    
 *   [0]                   0xAA
 *   [1]                   0xDD
 *   [2]   HEADING         0x00=RS232  0x01=RS422       PB8
 *   [3]   MOTION          0x00=RS232  0x01=RS422       PB1
 *   [4]   GNSS            0x00=RS232  0x01=RS422       PD8
 *   [5]   PPS             0x00=RS422  0x01=TTL         PD0
 *   [6]   SVP/SYNC        0x00=OFF    0x01=SYNC_ON 0x02=SVP_ON     PD12,           [7][8]             
 *   [7]   SVP               0x00=RS422  0x01=RS232       PD4(   /   )          [6]=0x02          
 *   [8]   SYNC            0x00=INPUT  0x01=OUTPUT      PD1, PD3          [6]=0x01          
 *   [9]                   0xDD
 */
uint8_t UART_STAR[10] = {0xAA, 0xDD, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0xDD};

/*                 */
static uint8_t g_pkt_idx = 0;               /*                    */
static uint8_t g_pkt_buf[10];               /*                    */

/**
 * @brief              USART6                
 * @param       str:     null                   
 * @retval         
 */
static void uart_star_send_str(const char *str)
{
    while (*str)
    {
        while ((USART6->SR & USART_SR_TXE) == 0);
        USART6->DR = (uint8_t)(*str++);
    }
}

/**
 * @brief              UART_STAR                                                 
 * @param          
 * @retval         
 * @note                                                              
 */
void uart_switch_apply_config(void)
{
    uint8_t *p = UART_STAR;

    /* [2] HEADING     PB8 */
    if (p[2] == 0x00)
    {
        UART1_SW_SET(GPIO_PIN_RESET);
        uart_star_send_str("HEADING:RS232\r\n");
    }
    else if (p[2] == 0x01)
    {
        UART1_SW_SET(GPIO_PIN_SET);
        uart_star_send_str("HEADING:RS422\r\n");
    }

    /* [3] MOTION     PB1 */
    if (p[3] == 0x00)
    {
        UART2_SW_SET(GPIO_PIN_RESET);
        uart_star_send_str("MOTION:RS232\r\n");
    }
    else if (p[3] == 0x01)
    {
        UART2_SW_SET(GPIO_PIN_SET);
        uart_star_send_str("MOTION:RS422\r\n");
    }

    /* [4] GNSS     PD8 */
    if (p[4] == 0x00)
    {
        UART3_SW_SET(GPIO_PIN_RESET);
        uart_star_send_str("GNSS:RS232\r\n");
    }
    else if (p[4] == 0x01)
    {
        UART3_SW_SET(GPIO_PIN_SET);
        uart_star_send_str("GNSS:RS422\r\n");
    }

    /* [5] PPS     PD0 */
    if (p[5] == 0x00)
    {
        PPS_SW_SET(GPIO_PIN_RESET);
        uart_star_send_str("PPS:RS422\r\n");
    }
    else if (p[5] == 0x01)
    {
        PPS_SW_SET(GPIO_PIN_SET);
        uart_star_send_str("PPS:TTL\r\n");
    }

    /* [6] SVP/SYNC     PD12 */
    if (p[6] == 0x00)
    {
        UART5_6_TTL_SW_SET(GPIO_PIN_SET);
        uart_star_send_str("SVP and SYNC:OFF\r\n");
    }
    else if (p[6] == 0x01)
    {
        UART5_6_TTL_SW_SET(GPIO_PIN_RESET);
        uart_star_send_str("SVP:OFF SYNC:ON\r\n");
    }
    else if (p[6] == 0x02)
    {
        UART5_6_TTL_SW_SET(GPIO_PIN_SET);
        uart_star_send_str("SVP:ON SYNC:OFF\r\n");
    }

    /* [7] SVP            PD4          [6](SVP/SYNC      )=0x02              */
    if (p[6] == 0x02)
    {
        if (p[7] == 0x00)
        {
            UART5_232_485_SW_SET(GPIO_PIN_SET);     /* PD4     */
            uart_star_send_str("SVP:RS422\r\n");
        }
        else if (p[7] == 0x01)
        {
            UART5_232_485_SW_SET(GPIO_PIN_RESET);   /* PD4     */
            uart_star_send_str("SVP:RS232\r\n");
        }
    }

    /* [8] SYNC            PD1, PD3
     *        [6](SVP/SYNC      )=0x01(SYNC_ON)           */
    if (p[6] == 0x01)
    {
        if (p[8] == 0x01)
        {
            UART5_485_SD_RE_DE_SET(GPIO_PIN_RESET);   /* PD1           SD    */
            UART5_RE_DE_SET(GPIO_PIN_SET);            /* PD3       DE                 */
            uart_star_send_str("SYNC:OUTPUT\r\n");
        }
        else
        {
            UART5_485_SD_RE_DE_SET(GPIO_PIN_SET);     /* PD1           SD    */
            UART5_RE_DE_SET(GPIO_PIN_RESET);          /* PD3       RE                 */
            uart_star_send_str("SYNC:INPUT\r\n");
        }
    }
}

/**
 * @brief       USART6                      10                      
 * @param       byte:                
 * @retval         
 * @note                           AA     DD         8                                  DD
 *                                       UART_STAR           uart_switch_apply_config()
 */
void uart_switch_rx_process(uint8_t byte)
{
    /*        0                   0xAA */
    if (g_pkt_idx == 0)
    {
        if (byte == 0xAA)
        {
            g_pkt_buf[0] = byte;
            g_pkt_idx = 1;
        }
        return;
    }

    /*        1                      0xDD */
    if (g_pkt_idx == 1)
    {
        if (byte == 0xDD)
        {
            g_pkt_buf[1] = byte;
            g_pkt_idx = 2;
        }
        else
        {
            g_pkt_idx = 0;      /*                          */
        }
        return;
    }

    /*        2~9                      */
    g_pkt_buf[g_pkt_idx] = byte;
    g_pkt_idx++;

    /*        10                       */
    if (g_pkt_idx >= 10)
    {
        if (g_pkt_buf[9] == 0xDD)
        {
            /*                       UART_STAR           */
            uint8_t i;
            for (i = 0; i < 10; i++)
            {
                UART_STAR[i] = g_pkt_buf[i];
            }
            uart_switch_apply_config();
        }
        g_pkt_idx = 0;  /*                             */
    }
}
