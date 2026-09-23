#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"

/******************************************************************************************************/
/* printf 重定向
 *
 * 说明：
 * 1. 工程使用 MicroLIB 时，通过 fputc() 将 printf 输出到 USART_UX。
 * 2. USART_UX 由 usart.h 中的宏决定，当前为 USART6。
 * 3. 这里关闭半主机模式，避免 printf 依赖调试器。
 */
/******************************************************************************************************/

#if 1

#if (__ARMCC_VERSION >= 6010050)

__asm(".global __use_no_semihosting\n\t");
__asm(".global __ARM_use_no_argv \n\t");

#else

#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
};

#endif

int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

FILE __stdout;

/**
 * @brief  printf 字符输出接口
 * @note   printf 每输出 1 个字符，都会进入此函数。
 */
int fputc(int ch, FILE *f)
{
    while ((USART6->SR & 0X40) == 0);

    USART6->DR = (uint8_t)ch;
    return ch;
}

#endif

/******************************************************************************************************/
/* 调试串口接收相关变量
 *
 * g_usart_rx_buf:
 *      接收数据缓存。
 *
 * g_usart_rx_sta:
 *      bit15    = 1 表示一帧接收完成
 *      bit14    = 1 表示已收到 0x0D
 *      bit13~0  = 当前已接收字节数
 *
 * g_rx_buffer:
 *      HAL 单字节接收缓存。
 */
/******************************************************************************************************/

#if USART_EN_RX

uint8_t g_usart_rx_buf[USART_REC_LEN];

uint16_t g_usart_rx_sta = 0;

uint8_t g_rx_buffer[RXBUFFERSIZE];

UART_HandleTypeDef g_uart1_handle;

/**
 * @brief  初始化调试串口
 * @param  baudrate: 波特率
 * @note   当前串口实例由 USART_UX 宏指定。
 */
void usart_init(uint32_t baudrate)
{
    g_uart1_handle.Instance = USART_UX;
    g_uart1_handle.Init.BaudRate = baudrate;
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart1_handle.Init.StopBits = UART_STOPBITS_1;
    g_uart1_handle.Init.Parity = UART_PARITY_NONE;
    g_uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_uart1_handle.Init.Mode = UART_MODE_TX_RX;

    HAL_UART_Init(&g_uart1_handle);

    HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
}

/**
 * @brief  UART MSP 初始化
 * @param  huart: UART 句柄
 * @note   HAL_UART_Init() 内部会调用此函数。
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (huart->Instance == USART_UX)
    {
        USART_UX_CLK_ENABLE();
        USART_TX_GPIO_CLK_ENABLE();
        USART_RX_GPIO_CLK_ENABLE();

        /* TX 引脚配置 */
        gpio_init_struct.Pin = USART_TX_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = USART_TX_GPIO_AF;
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);

        /* RX 引脚配置 */
        gpio_init_struct.Pin = USART_RX_GPIO_PIN;
        gpio_init_struct.Alternate = USART_RX_GPIO_AF;
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);

#if USART_EN_RX
        HAL_NVIC_EnableIRQ(USART_UX_IRQn);
        HAL_NVIC_SetPriority(USART_UX_IRQn, 3, 3);
#endif
    }
}

/**
 * @brief  UART 接收完成回调
 * @param  huart: UART 句柄
 * @note   当前按 “0x0D 0x0A” 作为一帧结束标志。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART_UX)
    {
        if ((g_usart_rx_sta & 0x8000) == 0)
        {
            if (g_usart_rx_sta & 0x4000)
            {
                if (g_rx_buffer[0] != 0x0A)
                {
                    g_usart_rx_sta = 0;
                }
                else
                {
                    g_usart_rx_sta |= 0x8000;
                }
            }
            else
            {
                if (g_rx_buffer[0] == 0x0D)
                {
                    g_usart_rx_sta |= 0x4000;
                }
                else
                {
                    g_usart_rx_buf[g_usart_rx_sta & 0X3FFF] = g_rx_buffer[0];
                    g_usart_rx_sta++;

                    if (g_usart_rx_sta > (USART_REC_LEN - 1))
                    {
                        g_usart_rx_sta = 0;
                    }
                }
            }
        }

        HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
    }
}

/**
 * @brief  USART_UX 中断服务函数
 */
void USART_UX_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart1_handle);
}

#endif

