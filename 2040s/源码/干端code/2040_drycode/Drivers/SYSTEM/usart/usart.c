/**
 ****************************************************************************************************
 * @file        usart.c
 * @author                  ()
 * @version     V1.1
 * @date        2023-06-05
 * @brief                     (          1)      printf
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
 * V1.0 20211014
 *           
 * V1.1 20230605
 *     USART_UX_IRQHandler()                    HAL_UART_RxCpltCallback()
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"


/*        os,                     */
//#if SYS_SUPPORT_OS
//#include "os.h"                               /* os      */
//#endif

/******************************************************************************************/
/*             ,     printf    ,             use MicroLIB */

#if 1
#if (__ARMCC_VERSION >= 6010050)                    /*     AC6         */
__asm(".global __use_no_semihosting\n\t");          /*                      */
__asm(".global __ARM_use_no_argv \n\t");            /* AC6          main                                                 */

#else
/*     AC5        ,             __FILE                     */
#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
    /* Whatever you require here. If the only file you are using is */
    /* standard output using printf() for debugging, no file handling */
    /* is required. */
};

#endif

/*                                 _ttywrch\_sys_exit\_sys_command_string    ,          AC6  AC5     */
int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

/*     _sys_exit()                     */
void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

/* FILE    stdio.h        . */
FILE __stdout;

/*       fputc    , printf                  fputc                */
int fputc(int ch, FILE *f)
{
    while ((USART6->SR & 0X40) == 0);               /*                       */

    USART6->DR = (uint8_t)ch;                       /*                ch       DR       */
    return ch;
}
#endif
/***********************************************END*******************************************/
    
#if USART_EN_RX                                     /*               */

/*         ,    USART_REC_LEN      . */
uint8_t g_usart_rx_buf[USART_REC_LEN];

/*          
 *  bit15                   
 *  bit14              0x0d
 *  bit13~0                          
*/
uint16_t g_usart_rx_sta = 0;

uint8_t g_rx_buffer[RXBUFFERSIZE];                  /* HAL                     */

UART_HandleTypeDef g_uart1_handle;                  /* UART    */


/**
 * @brief           X          
 * @param       baudrate:       ,                         
 * @note            :                     ,                           .
 *                   USART          sys_stm32_clock_init()                  .
 * @retval        
 */
void usart_init(uint32_t baudrate)
{
    g_uart1_handle.Instance = USART_UX;                         /* USART1 */
    g_uart1_handle.Init.BaudRate = baudrate;                    /*        */
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;        /*       8           */
    g_uart1_handle.Init.StopBits = UART_STOPBITS_1;             /*            */
    g_uart1_handle.Init.Parity = UART_PARITY_NONE;              /*              */
    g_uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;        /*            */
    g_uart1_handle.Init.Mode = UART_MODE_TX_RX;                 /*          */
    HAL_UART_Init(&g_uart1_handle);                             /* HAL_UART_Init()      UART1 */
    
    /*                             UART_IT_RXNE                                           */
    HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
}

/**
 * @brief       UART             
 * @param       huart: UART           
 * @note                  HAL_UART_Init()    
 *                                             
 * @retval        
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;
    if(huart->Instance == USART_UX)                             /*          1          1 MSP       */
    {
        USART_UX_CLK_ENABLE();                                  /* USART1          */
        USART_TX_GPIO_CLK_ENABLE();                             /*                  */
        USART_RX_GPIO_CLK_ENABLE();                             /*                  */

        gpio_init_struct.Pin = USART_TX_GPIO_PIN;               /* TX     */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /*             */
        gpio_init_struct.Pull = GPIO_PULLUP;                    /*      */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /*      */
        gpio_init_struct.Alternate = USART_TX_GPIO_AF;          /*       USART1 */
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);   /*                */

        gpio_init_struct.Pin = USART_RX_GPIO_PIN;               /* RX     */
        gpio_init_struct.Alternate = USART_RX_GPIO_AF;          /*       USART1 */
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);   /*                */

#if USART_EN_RX
        HAL_NVIC_EnableIRQ(USART_UX_IRQn);                      /*     USART1         */
        HAL_NVIC_SetPriority(USART_UX_IRQn, 3, 3);              /*           3          3 */
#endif
    }
}

/**
 * @brief       Rx           
 * @param       huart: UART           
 * @retval        
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART_UX)             /*          1 */
    {
        if((g_usart_rx_sta & 0x8000) == 0)      /*           */
        {
            if(g_usart_rx_sta & 0x4000)         /*         0x0d */
            {
                if(g_rx_buffer[0] != 0x0a) 
                {
                    g_usart_rx_sta = 0;         /*         ,         */
                }
                else 
                {
                    g_usart_rx_sta |= 0x8000;   /*           */
                }
            }
            else                                /*         0X0D */
            {
                if(g_rx_buffer[0] == 0x0d)
                {
                    g_usart_rx_sta |= 0x4000;
                }
                else
                {
                    g_usart_rx_buf[g_usart_rx_sta & 0X3FFF] = g_rx_buffer[0] ;
                    g_usart_rx_sta++;
                    if(g_usart_rx_sta > (USART_REC_LEN - 1))
                    {
                        g_usart_rx_sta = 0;     /*             ,             */
                    }
                }
            }
        }
        
        HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
    }
}

/**
 * @brief           1          
 * @param         
 * @retval        
 */
void USART_UX_IRQHandler(void)
{ 
//#if SYS_SUPPORT_OS                              /*     OS */
//    OSIntEnter();    
//#endif

    HAL_UART_IRQHandler(&g_uart1_handle);       /*     HAL                   */

//#if SYS_SUPPORT_OS                              /*     OS */
//    OSIntExit();
//#endif
}

#endif


 

 




