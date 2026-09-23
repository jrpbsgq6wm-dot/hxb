#include "signal_switch.h"
/*
 * 信号与 STM32 引脚对应关系
 *
 * 1. HEADING
 *    模式控制:
 *      PB8  - UART1_232/422/_SW
 *             0 = RS232
 *             1 = RS422
 *    串口输入:
 *      PB7  - UART1_TTL_RXD_MCU
 *      USART1_RX
 *
 * 2. MOTION
 *    模式控制:
 *      PB1  - UART2_232/422/_SW
 *             0 = RS232
 *             1 = RS422
 *    串口输入:
 *      PA3  - UART2_TTL_RXD_MCU
 *      USART2_RX
 *
 * 3. GNSS
 *    模式控制:
 *      PD8  - UART3_232/422/_SW
 *             0 = RS232
 *             1 = RS422
 *    串口输入:
 *      PD9  - UART3_TTL_RXD_MCU
 *      USART3_RX
 *
 * 4. PPS
 *    模式控制:
 *      PD0  - UART4_TTL_SW
 *             0 = RS422
 *             1 = TTL
 *    脉冲输入:
 *      PD6  - UART4_TTL_RXD_MCU
 *      GPIO EXTI6
 *
 * 5. SVS / 表声
 *    模式控制:
 *      PD4  - UART5_232/485_SW
 *             0 = RS232
 *             1 = RS485
 *      PD3  - UART5_232/485_RE_DE
 *             RS232 = 1
 *             RS485_RX = 0
 *      PD12 - UART5/6_TTL_SW
 *             0 = SYNC / UART6
 *             1 = SVS  / UART5
 *      PD1  - UART5_485_SD_RE_DE
 *             1 = TTL -> 485 -> 湿端
 *    串口输入:
 *      PD2  - UART5_TTL_RXD_MCU
 *      UART5_RX
 *
 * 6. SYNC
 *    模式控制:
 *      PD12 - UART5/6_TTL_SW
 *             0 = SYNC / UART6
 *             1 = SVS  / UART5
 *      PD1  - UART5_485_SD_RE_DE
 *             1 = SYNC 输入 -> 湿端
 *             0 = 湿端同步 -> 同步输出口
 *    脉冲检测:
 *      PD13 - UART6_TTL_RXD_MCU
 *             SYNC 输入检测，GPIO EXTI13
 *      PD11 - SYNC_OUT_TTL_TXD_MCU
 *             SYNC 输出链路检测，GPIO EXTI11
 */
/******************************************************************************************/
/* PPS 状态 */

volatile uint8_t g_pps_flag = 0;
volatile uint32_t g_pps_count = 0;

/******************************************************************************************/
/* SYNC 状态 */

static sync_mode_t g_sync_mode = SYNC_MODE_INPUT;
static sync_edge_t g_sync_edge = SYNC_EDGE_RISING;

volatile uint8_t g_sync_event_flag = 0;
volatile uint8_t g_sync_in_flag = 0;
volatile uint8_t g_sync_out_flag = 0;
volatile uint32_t g_sync_in_count = 0;
volatile uint32_t g_sync_out_count = 0;
volatile uint8_t g_sync_last_level = 0;

/******************************************************************************************/

/**
 * @brief  Heading GPIO初始化函数
 * @param  void
 * @note   初始化Heading相关的GPIO
 */
void heading_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*
     * PB8: UART1_232/422/_SW
     * 0 = HEADING RS232 输入
     * 1 = HEADING RS422 输入
     */
    gpio_init_struct.Pin = GPIO_PIN_8;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio_init_struct);
}

/**
 * @brief  设置Heading输入模式
 * @param  mode: 输入模式
 * @note   设置Heading相关的输入模式
 */
void heading_set_input_mode(heading_input_mode_t mode)
{
    if (mode == HEADING_INPUT_RS232)
    {
        printf("HEADING:RS232\r\n");
        /*
         * HEADING RS232 输入：
         * PB8 = 0
         */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    }
    else
    {
        printf("HEADING:RS422\r\n");
        /*
         * HEADING RS422 输入：
         * PB8 = 1
         */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    }
}


/**
 * @brief  Motion GPIO初始化函数
 * @param  void
 * @note   初始化Motion相关的GPIO
 */
void motion_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*
     * PB1: UART2_232/422/_SW
     * 0 = MOTION RS232 输入
     * 1 = MOTION RS422 输入
     */
    gpio_init_struct.Pin = GPIO_PIN_1;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio_init_struct);
}

/**
 * @brief  设置Motion输入模式
 * @param  mode: 输入模式
 * @note   设置Motion相关的输入模式
 */
void motion_set_input_mode(motion_input_mode_t mode)
{
    if (mode == MOTION_INPUT_RS232)
    {
        printf("MOTION:RS232\r\n");
        /*
         * MOTION RS232 输入：
         * PB1 = 0
         */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    }
    else
    {
        printf("MOTION:RS422\r\n");
        /*
         * MOTION RS422 输入：
         * PB1 = 1
         */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
    }
}


/**
 * @brief  GNSS GPIO初始化函数
 * @param  void
 * @note   初始化GNSS相关的GPIO
 */
void gnss_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*
     * PD8: UART3_232/422/_SW
     * 0 = GNSS RS232 输入
     * 1 = GNSS RS422 输入
     */
    gpio_init_struct.Pin = GPIO_PIN_8;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);
}

/**
 * @brief  设置GNSS输入模式
 * @param  mode: 输入模式
 * @note   设置GNSS相关的输入模式
 */
void gnss_set_input_mode(gnss_input_mode_t mode)
{
    if (mode == GNSS_INPUT_RS232)
    {
        printf("GNSS:RS232\r\n");
        /*
         * GNSS RS232 输入：
         * PD8 = 0
         */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_RESET);
    }
    else
    {
        printf("GNSS:RS422\r\n");
        /*
         * GNSS RS422 输入：
         * PD8 = 1
         */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_SET);
    }
}


/**
 * @brief  PPS GPIO初始化函数
 * @param  void
 * @note   初始化PPS相关的GPIO
 */
void pps_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*
     * PD0: UART4_TTL_SW
     * 0 = PPS RS422 输入
     * 1 = PPS TTL 输入
     */
    gpio_init_struct.Pin = GPIO_PIN_0;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);

    /*
     * PD6: UART4_TTL_RXD_MCU
     * PPS 脉冲输入检测脚，默认上升沿触发。
     */
    gpio_init_struct.Pin = GPIO_PIN_6;
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/**
 * @brief  设置PPS输入模式
 * @param  mode: 输入模式
 * @note   设置PPS相关的输入模式
 */
void pps_set_input_mode(pps_input_mode_t mode)
{
    if (mode == PPS_INPUT_TTL)
    {
        printf("PPS:TTL\r\n");
        /*
         * PPS TTL 输入：
         * PD0 = 1
         */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
    }
    else
    {
        printf("PPS:RS422\r\n");
        /*
         * PPS RS422 输入：
         * PD0 = 0
         */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
    }
}

/**
 * @brief  PPS中断处理函数
 * @param  void
 * @note   处理PPS相关的中断
 */
void EXTI9_5_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);
        g_pps_count++;
        g_pps_flag = 1;
    }
}

/*
 * @brief  SVS GPIO初始化函数
 * @param  void
 * @note   初始化SVS相关的GPIO
 */
void SVS_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;

    /* PD4: 表声 RS232 / RS485 选择 */
    gpio_init_struct.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);

    /* PD3: 表声芯片方向控制 */
    gpio_init_struct.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);

    /* PD12: SVS / SYNC 通道选择 */
    gpio_init_struct.Pin = GPIO_PIN_12;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);

    /* PD1: 到湿端 485 方向控制 */
    gpio_init_struct.Pin = GPIO_PIN_1;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);
}

/**
 * @brief  设置SVS输入模式
 * @param  mode: 输入模式
 * @note   设置SVS相关的输入模式
 */
void SVS_set_input_mode(SVS_input_mode_t mode)
{
    /* PD12 = 1，选择 SVS/UART5 */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);

    /* PD1 = 1，SVS TTL -> 485 -> 湿端 */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, GPIO_PIN_SET);

    if (mode == SVS_INPUT_RS232)
    {
        printf("SVS:RS232\r\n");
        /* PD4 = 0，PD3 = 1 */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);
    }
    else
    {
        printf("SVS:RS485\r\n");
        /* PD4 = 1，PD3 = 0 */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
    }
}

/**
 * @brief  配置 SYNC 检测引脚的中断边沿
 * @param  edge: 上升沿、下降沿或双边沿
 */
static void sync_gpio_exti_config(sync_edge_t edge)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_11 | GPIO_PIN_13);

    gpio_init_struct.Pin = GPIO_PIN_11 | GPIO_PIN_13;

    if (edge == SYNC_EDGE_RISING)
    {
        gpio_init_struct.Mode = GPIO_MODE_IT_RISING;
    }
    else if (edge == SYNC_EDGE_FALLING)
    {
        gpio_init_struct.Mode = GPIO_MODE_IT_FALLING;
    }
    else
    {
        gpio_init_struct.Mode = GPIO_MODE_IT_RISING_FALLING;
    }

    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
}


/**
 * @brief  SYNC 硬件初始化
 * @note   初始化 PD1、PD12、PD11、PD13，并默认进入 SYNC 输入 / 上升沿模式。
 */
void sync_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* PD12: 选择 SVP/SYNC 通道 */
    gpio_init_struct.Pin = GPIO_PIN_12;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);

    /* PD1: 控制 U31 方向 */
    gpio_init_struct.Pin = GPIO_PIN_1;
    HAL_GPIO_Init(GPIOD, &gpio_init_struct);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
 * @brief  配置 SYNC 工作模式和触发边沿
 * @param  mode: SYNC_MODE_INPUT 或 SYNC_MODE_OUTPUT
 * @param  edge: SYNC_EDGE_RISING / SYNC_EDGE_FALLING / SYNC_EDGE_BOTH
 */
void sync_config(sync_mode_t mode, sync_edge_t edge)
{
    g_sync_mode = mode;
    g_sync_edge = edge;

    /* PD12 = 0，选择 SYNC/UART6 */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

    if (mode == SYNC_MODE_INPUT)
    {
        printf("SYNC:INPUT\r\n");
        /* PD1 = 1，外部同步输入 -> 湿端 */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, GPIO_PIN_SET);
    }
    else
    {
        printf("SYNC:OUTPUT\r\n");
        /* PD1 = 0，湿端同步 -> 同步输出口 */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, GPIO_PIN_RESET);
    }

    sync_gpio_exti_config(edge);
}

/**
 * @brief  PD11 / PD13 外部中断处理
 * @note   中断里只置标志和计数，不做 printf、网络发送、I2C 等耗时操作。
 */
void EXTI15_10_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);

        if (g_sync_mode == SYNC_MODE_INPUT)
        {
            g_sync_last_level = (uint8_t)HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_13);
            g_sync_in_count++;
            g_sync_in_flag = 1;
            g_sync_event_flag = 1;
        }
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);

        if (g_sync_mode == SYNC_MODE_OUTPUT)
        {
            g_sync_last_level = (uint8_t)HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_11);
            g_sync_out_count++;
            g_sync_out_flag = 1;
            g_sync_event_flag = 1;
        }
    }
}

sync_edge_t sync_get_edge(void)
{
    return g_sync_edge;
}
