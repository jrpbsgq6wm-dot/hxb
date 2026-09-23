#include "./BSP/uart_Set/uart_Set.h"

/**
 * @brief  UART切换GPIO初始化
 * @note
 * PB8  -> UART1 232/422切换
 * PB1  -> UART2 232/422切换
 * PD8  -> UART3 232/422切换
 * PD0  -> PPS切换
 * PD4  -> UART5 232/485切换
 * PD3  -> UART5 232/485 RE/DE控制
 * PD1  -> UART5 485 SD/RE/DE控制
 * PD12 -> UART5/6 TTL切换
 */
void uart_switch_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    /* 先打开各个GPIO时钟 */
    UART1_SW_GPIO_CLK_ENABLE();
    UART2_SW_GPIO_CLK_ENABLE();
    UART3_SW_GPIO_CLK_ENABLE();
    PPS_SW_GPIO_CLK_ENABLE();
    UART5_232_485_SW_GPIO_CLK_ENABLE();
    UART5_RE_DE_GPIO_CLK_ENABLE();
    UART5_485_SD_RE_DE_GPIO_CLK_ENABLE();
    UART5_6_TTL_SW_GPIO_CLK_ENABLE();

    /* PB8：UART1模式选择脚，默认拉低 */
    UART1_SW_SET(GPIO_PIN_RESET);
    gpio_init_struct.Pin = UART1_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(UART1_SW_GPIO_PORT, &gpio_init_struct);

    /* PB1：UART2模式选择脚，默认拉低 */
    UART2_SW_SET(GPIO_PIN_RESET);
    gpio_init_struct.Pin = UART2_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(UART2_SW_GPIO_PORT, &gpio_init_struct);

    /* PD8：UART3模式选择脚，默认拉低 */
    UART3_SW_SET(GPIO_PIN_RESET);
    gpio_init_struct.Pin = UART3_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(UART3_SW_GPIO_PORT, &gpio_init_struct);

    /* PD0：PPS模式选择脚，默认拉高 */
    PPS_SW_SET(GPIO_PIN_SET);
    gpio_init_struct.Pin = PPS_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PPS_SW_GPIO_PORT, &gpio_init_struct);

    /* PD4：UART5 232/485切换脚，默认拉高 */
    UART5_232_485_SW_SET(GPIO_PIN_SET);
    gpio_init_struct.Pin = UART5_232_485_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(UART5_232_485_SW_GPIO_PORT, &gpio_init_struct);

    /* PD3：UART5 RE/DE控制脚，默认拉高 */
    UART5_RE_DE_SET(GPIO_PIN_SET);
    gpio_init_struct.Pin = UART5_RE_DE_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(UART5_RE_DE_GPIO_PORT, &gpio_init_struct);

    /* PD1：UART5 485 SD/RE/DE控制脚，默认拉高 */
    UART5_485_SD_RE_DE_SET(GPIO_PIN_SET);
    gpio_init_struct.Pin = UART5_485_SD_RE_DE_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(UART5_485_SD_RE_DE_GPIO_PORT, &gpio_init_struct);

    /* PD12：UART5/6 TTL切换脚，默认拉低 */
    UART5_6_TTL_SW_SET(GPIO_PIN_RESET);
    gpio_init_struct.Pin = UART5_6_TTL_SW_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(UART5_6_TTL_SW_GPIO_PORT, &gpio_init_struct);
}
/*
 * UART_STAR配置帧：
 * [0] 0xAA
 * [1] 0xDD
 * [2] HEADING  0x00=RS232 0x01=RS422
 * [3] MOTION   0x00=RS232 0x01=RS422
 * [4] GNSS     0x00=RS232 0x01=RS422
 * [5] PPS      0x00=RS422 0x01=TTL
 * [6] SVP/SYNC 0x00=OFF    0x01=SYNC_ON  0x02=SVP_ON
 * [7] SVP      仅当[6]=0x02时有效，0x00=RS422 0x01=RS232
 * [8] SYNC     仅当[6]=0x01时有效，0x00=INPUT  0x01=OUTPUT
 * [9] 0xDD
 */
uint8_t UART_STAR[10] = {0xAA, 0xDD, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0xDD};

/* 接收缓冲和当前字节位置 */
static uint8_t g_pkt_idx = 0;
static uint8_t g_pkt_buf[10];

/**
 * @brief  用USART6发送字符串
 * @param  str: 待发送字符串，必须以'\0'结尾
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
 * @brief  根据UART_STAR数组内容，真正设置GPIO状态
 */
void uart_switch_apply_config(void)
{
    uint8_t *p = UART_STAR;

    /* [2] HEADING */
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

    /* [3] MOTION */
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

    /* [4] GNSS */
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

    /* [5] PPS */
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

    /* [6] SVP/SYNC总开关 */
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

    /* [7] SVP模式，只有[6]=0x02时才看这个值 */
    if (p[6] == 0x02)
    {
        if (p[7] == 0x00)
        {
            UART5_232_485_SW_SET(GPIO_PIN_SET);
            uart_star_send_str("SVP:RS422\r\n");
        }
        else if (p[7] == 0x01)
        {
            UART5_232_485_SW_SET(GPIO_PIN_RESET);
            uart_star_send_str("SVP:RS232\r\n");
        }
    }

    /* [8] SYNC方向，只有[6]=0x01时才看这个值 */
    if (p[6] == 0x01)
    {
        if (p[8] == 0x01)
        {
            UART5_485_SD_RE_DE_SET(GPIO_PIN_RESET);
            UART5_RE_DE_SET(GPIO_PIN_SET);
            uart_star_send_str("SYNC:OUTPUT\r\n");
        }
        else
        {
            UART5_485_SD_RE_DE_SET(GPIO_PIN_SET);
            UART5_RE_DE_SET(GPIO_PIN_RESET);
            uart_star_send_str("SYNC:INPUT\r\n");
        }
    }
}

/**
 * @brief  串口接收处理函数
 * @param  byte: 当前收到的1个字节
 * @note   按 0xAA 0xDD ... 0xDD 的10字节协议收包
 */
void uart_switch_rx_process(uint8_t byte)
{
    /* 第1字节：0xAA */
    if (g_pkt_idx == 0)
    {
        if (byte == 0xAA)
        {
            g_pkt_buf[0] = byte;
            g_pkt_idx = 1;
        }
        return;
    }

    /* 第2字节：0xDD */
    if (g_pkt_idx == 1)
    {
        if (byte == 0xDD)
        {
            g_pkt_buf[1] = byte;
            g_pkt_idx = 2;
        }
        else
        {
            g_pkt_idx = 0;
        }
        return;
    }

    /* 第3~第10字节 */
    g_pkt_buf[g_pkt_idx] = byte;
    g_pkt_idx++;

    /* 收满10字节 */
    if (g_pkt_idx >= 10)
    {
        if (g_pkt_buf[9] == 0xDD)
        {
            /* 收到完整有效帧，更新配置 */
            for (uint8_t i = 0; i < 10; i++)
            {
                UART_STAR[i] = g_pkt_buf[i];
            }
            uart_switch_apply_config();
        }

        /* 重新等待下一帧 */
        g_pkt_idx = 0;
    }
}

/**
 * @brief  Heading输入模式枚举
 * @note   定义Heading的输入模式
 */
typedef enum
{
    HEADING_INPUT_RS232 = 0,
    HEADING_INPUT_RS422
} heading_input_mode_t;

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
        /*
         * HEADING RS232 输入：
         * PB8 = 0
         */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    }
    else
    {
        /*
         * HEADING RS422 输入：
         * PB8 = 1
         */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    }
}

/**
 * @brief  Motion输入模式枚举
 * @note   定义Motion的输入模式
 */
typedef enum
{
    MOTION_INPUT_RS232 = 0,
    MOTION_INPUT_RS422
} motion_input_mode_t;

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
        /*
         * MOTION RS232 输入：
         * PB1 = 0
         */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    }
    else
    {
        /*
         * MOTION RS422 输入：
         * PB1 = 1
         */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
    }
}

/**
 * @brief  GNSS输入模式枚举
 * @note   定义GNSS的输入模式
 */
typedef enum
{
    GNSS_INPUT_RS232 = 0,
    GNSS_INPUT_RS422
} gnss_input_mode_t;

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
        /*
         * GNSS RS232 输入：
         * PD8 = 0
         */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_RESET);
    }
    else
    {
        /*
         * GNSS RS422 输入：
         * PD8 = 1
         */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_SET);
    }
}

/**
 * @brief  PPS输入模式枚举
 * @note   定义PPS的输入模式
 */
typedef enum
{
    PPS_INPUT_RS422 = 0,
    PPS_INPUT_TTL
} pps_input_mode_t;

/**
 * @brief  PPS全局变量
 * @note   定义PPS相关的全局变量
 */
volatile uint8_t g_pps_flag = 0;
volatile uint32_t g_pps_count = 0;

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
        /*
         * PPS TTL 输入：
         * PD0 = 1
         */
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
    }
    else
    {
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