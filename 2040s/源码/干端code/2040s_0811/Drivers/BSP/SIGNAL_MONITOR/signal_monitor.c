/**
 * @file    signal_monitor.c
 * @brief   HEADING、MOTION、GNSS、SVS、PPS、SYNC 信号有效性监测。
 *
 * 串口接收流程：
 * 1. HAL UART + DMA 以普通 DMA 模式接收一帧数据；
 * 2. UART 空闲中断或 DMA 接满时，HAL 调用 HAL_UARTEx_RxEventCallback()；
 * 3. 回调将 DMA 接收缓冲区复制到业务处理缓冲区，记录长度并置位完成标志；
 * 4. FreeRTOS 任务被通知后解析业务缓冲区，更新 LED，最后重新启动本路 DMA。
 *
 * 中断中只复制数据和通知任务，不解析协议、不操作 I2C LED，避免中断执行时间过长。
 */

#include "signal_monitor.h"
#include "./BSP/LP5012/LP5012.h"
#include "./BSP/SIGNAL_SWITCH/signal_switch.h"

#include "FreeRTOS.h"
#include "task.h"

#include <string.h>

typedef struct
{
    const char *pattern;     /* 需要匹配的报文头。 */
    uint8_t position;        /* 当前已匹配的字符数量。 */
} signal_matcher_t;

typedef void (*signal_led_set_t)(uint8_t on);
typedef void (*signal_parse_byte_t)(uint8_t data, TickType_t now);

/**
 * @brief  单路串口信号监测运行状态。
 *
 * dma_rx_buffer：
 *     HAL DMA 直接写入的接收区，只在 DMA 运行期间使用。
 *
 * frame_buffer：
 *     DMA 完成后由中断回调复制到此处，任务只解析该缓冲区。
 *     DMA 重启后不会覆盖正在处理的数据。
 */
typedef struct
{
    UART_HandleTypeDef *uart;                         /* 本通道 UART HAL 句柄。 */
    uint8_t *dma_rx_buffer;                           /* DMA 接收缓冲区。 */
    uint8_t *frame_buffer;                            /* 供任务解析的完整帧缓冲区。 */
    volatile uint16_t frame_length;                   /* 当前完整帧实际长度。 */
    volatile uint8_t frame_ready;                     /* 1：任务尚未处理当前帧。 */
    volatile uint8_t restart_requested;               /* 1：DMA 需要在任务上下文重新启动。 */
    uint8_t enabled;                                  /* 1：参与协议有效性判断。 */
    uint8_t led_on;                                   /* 1：当前显示绿色有效状态。 */
    TickType_t last_valid_tick;                       /* 最近一次有效协议时间。 */
} signal_uart_channel_t;

/* 四路 DMA 接收区与业务处理区。每路单独保存，互不覆盖。 */
static uint8_t g_heading_dma_rx_buffer[SIGNAL_MONITOR_DMA_RX_SIZE];
static uint8_t g_motion_dma_rx_buffer[SIGNAL_MONITOR_DMA_RX_SIZE];
static uint8_t g_gnss_dma_rx_buffer[SIGNAL_MONITOR_DMA_RX_SIZE];
static uint8_t g_svs_dma_rx_buffer[SIGNAL_MONITOR_DMA_RX_SIZE];

static uint8_t g_heading_frame_buffer[SIGNAL_MONITOR_DMA_RX_SIZE];
static uint8_t g_motion_frame_buffer[SIGNAL_MONITOR_DMA_RX_SIZE];
static uint8_t g_gnss_frame_buffer[SIGNAL_MONITOR_DMA_RX_SIZE];
static uint8_t g_svs_frame_buffer[SIGNAL_MONITOR_DMA_RX_SIZE];

/*
 * 句柄使用业务信号名称命名，避免仅使用 USART1、DMA1_Stream5 等硬件名称。
 *
 * HEADING：USART1 / DMA2 Stream2 / PB7
 * MOTION ：USART2 / DMA1 Stream5 / PA3
 * GNSS   ：USART3 / DMA1 Stream1 / PD9
 * SVS    ：UART5  / DMA1 Stream0 / PD2
 */
static UART_HandleTypeDef g_heading_uart;
static UART_HandleTypeDef g_motion_uart;
static UART_HandleTypeDef g_gnss_uart;
static UART_HandleTypeDef g_svs_uart;

static DMA_HandleTypeDef g_heading_rx_dma;
static DMA_HandleTypeDef g_motion_rx_dma;
static DMA_HandleTypeDef g_gnss_rx_dma;
static DMA_HandleTypeDef g_svs_rx_dma;

static signal_uart_channel_t g_heading_channel;
static signal_uart_channel_t g_motion_channel;
static signal_uart_channel_t g_gnss_channel;
static signal_uart_channel_t g_svs_channel;

static signal_matcher_t g_heading_matcher = {"$HEHDT", 0U};
static signal_matcher_t g_gnss_gga_matcher = {"$GNGGA", 0U};
static signal_matcher_t g_gnss_zda_matcher = {"$GPZDA", 0U};

static TaskHandle_t g_signal_monitor_task_handle = NULL;

static uint8_t g_pps_led_on = 0U;
static uint8_t g_sync_led_on = 0U;
static TickType_t g_sync_last_tick = 0U;
static uint8_t g_sync_monitor_enabled = 0U;
static uint8_t g_svs_wet_input_enabled = 0U;
static signal_monitor_sync_mode_t g_sync_mode = SIGNAL_MONITOR_SYNC_UNCONFIGURED;

/**
 * @brief  向字符串匹配器输入一个字符。
 * @retval 1：完成一次完整匹配；0：尚未匹配完成。
 */
static uint8_t signal_match_feed(signal_matcher_t *matcher, uint8_t data)
{
    if (data == (uint8_t)matcher->pattern[matcher->position])
    {
        matcher->position++;

        if (matcher->pattern[matcher->position] == '\0')
        {
            matcher->position = 0U;
            return 1U;
        }
    }
    else
    {
        /* 当前字符也可能恰好是下一帧报文头的第一个字符。 */
        matcher->position = (data == (uint8_t)matcher->pattern[0]) ? 1U : 0U;
    }

    return 0U;
}

/* 有效信号为绿色；无有效数据或超时为红色。 */
static void signal_heading_led_set(uint8_t on)
{
    if (on != 0U)
    {
        LP5012_U1_Set_D5(0U, SIGNAL_MONITOR_LED_BRIGHT, 0U);
    }
    else
    {
        LP5012_U1_Set_D5(SIGNAL_MONITOR_LED_BRIGHT, 0U, 0U);
    }
}

static void signal_motion_led_set(uint8_t on)
{
    if (on != 0U)
    {
        LP5012_U1_Set_D4(0U, SIGNAL_MONITOR_LED_BRIGHT, 0U);
    }
    else
    {
        LP5012_U1_Set_D4(SIGNAL_MONITOR_LED_BRIGHT, 0U, 0U);
    }
}

static void signal_gnss_led_set(uint8_t on)
{
    if (on != 0U)
    {
        LP5012_U2_Set_D7(0U, SIGNAL_MONITOR_LED_BRIGHT, 0U);
    }
    else
    {
        LP5012_U2_Set_D7(SIGNAL_MONITOR_LED_BRIGHT, 0U, 0U);
    }
}

static void signal_svs_led_set(uint8_t on)
{
    if (on != 0U)
    {
        LP5012_U2_Set_D6(0U, SIGNAL_MONITOR_LED_BRIGHT, 0U);
    }
    else
    {
        LP5012_U2_Set_D6(SIGNAL_MONITOR_LED_BRIGHT, 0U, 0U);
    }
}

static void signal_svs_led_set_blue(void)
{
    LP5012_U2_Set_D6(0U, 0U, SIGNAL_MONITOR_LED_BRIGHT);
}

static void signal_pps_led_set(uint8_t on)
{
    if (on != 0U)
    {
        LP5012_U1_Set_D3(0U, SIGNAL_MONITOR_LED_BRIGHT, 0U);
    }
    else
    {
        LP5012_U1_Set_D3(SIGNAL_MONITOR_LED_BRIGHT, 0U, 0U);
    }
}

static void signal_sync_led_set(uint8_t on)
{
    if (on != 0U)
    {
        LP5012_U2_Set_D8(0U, SIGNAL_MONITOR_LED_BRIGHT, 0U);
    }
    else
    {
        LP5012_U2_Set_D8(SIGNAL_MONITOR_LED_BRIGHT, 0U, 0U);
    }
}

/**
 * @brief  记录一次有效协议，并在首次有效时点亮绿色 LED。
 */
static void signal_uart_mark_valid(signal_uart_channel_t *channel,
                                   TickType_t now,
                                   signal_led_set_t led_set)
{
    channel->last_valid_tick = now;

    if (channel->led_on == 0U)
    {
        channel->led_on = 1U;
        led_set(1U);
    }
}

/**
 * @brief  检查一条通道是否超过有效数据超时时间。
 */
static void signal_uart_timeout_check(signal_uart_channel_t *channel,
                                      TickType_t now,
                                      signal_led_set_t led_set)
{
    if (channel->enabled == 0U)
    {
        if (channel->led_on != 0U)
        {
            channel->led_on = 0U;
            led_set(0U);
        }
        return;
    }

    if ((channel->led_on != 0U) &&
        ((now - channel->last_valid_tick) >= pdMS_TO_TICKS(SIGNAL_MONITOR_TIMEOUT_MS)))
    {
        channel->led_on = 0U;
        led_set(0U);
    }
}

/* 以下四个函数保留原工程的有效报文判断规则。 */
static void signal_heading_parse_byte(uint8_t data, TickType_t now)
{
    if (signal_match_feed(&g_heading_matcher, data) != 0U)
    {
        signal_uart_mark_valid(&g_heading_channel, now, signal_heading_led_set);
    }
}

static void signal_motion_parse_byte(uint8_t data, TickType_t now)
{
    if (data == ':')
    {
        signal_uart_mark_valid(&g_motion_channel, now, signal_motion_led_set);
    }
}

static void signal_gnss_parse_byte(uint8_t data, TickType_t now)
{
    if ((signal_match_feed(&g_gnss_gga_matcher, data) != 0U) ||
        (signal_match_feed(&g_gnss_zda_matcher, data) != 0U))
    {
        signal_uart_mark_valid(&g_gnss_channel, now, signal_gnss_led_set);
    }
}

static void signal_svs_parse_byte(uint8_t data, TickType_t now)
{
    if (data == ' ')
    {
        signal_uart_mark_valid(&g_svs_channel, now, signal_svs_led_set);
    }
}

/**
 * @brief  解析一条已由 DMA 接收完成的完整数据帧。
 *
 * @note frame_buffer 中的数据已在中断回调中复制完成，因此任务解析期间
 *       不会被下一次 DMA 接收覆盖。
 */
static void signal_uart_process_frame(signal_uart_channel_t *channel,
                                      TickType_t now,
                                      signal_parse_byte_t parse_byte)
{
    uint16_t index;
    uint16_t frame_length;

    if (channel->frame_ready == 0U)
    {
        return;
    }
	

    frame_length = channel->frame_length;

    if ((channel->enabled != 0U) && (frame_length <= SIGNAL_MONITOR_DMA_RX_SIZE))
    {
        for (index = 0U; index < frame_length; index++)
        {
            parse_byte(channel->frame_buffer[index], now);
        }
    }

    /*
     * 当前帧已处理完。清除标志后，调用方会重启 DMA，准备接收下一帧。
     * 此时 DMA 尚未运行，因此不会出现任务和 DMA 同时写同一缓冲区的问题。
     */
    taskENTER_CRITICAL();
    channel->frame_length = 0U;
    channel->frame_ready = 0U;
    taskEXIT_CRITICAL();
}

/**
 * @brief  使用 HAL 重新启动一条 UART 的普通 DMA 空闲接收。
 * @note   DMA 不使用循环模式。每收到一帧后必须由任务调用本函数重新开始接收。
 */
static void signal_uart_restart_rx(signal_uart_channel_t *channel)
{
    if ((channel->frame_ready != 0U) || (channel->restart_requested == 0U))
    {
        return;
    }

    if (HAL_UARTEx_ReceiveToIdle_DMA(channel->uart,
                                     channel->dma_rx_buffer,
                                     SIGNAL_MONITOR_DMA_RX_SIZE) == HAL_OK)
    {
        /* 不需要半满中断；一帧结束只依赖 IDLE 或 DMA 满事件。 */
        __HAL_DMA_DISABLE_IT(channel->uart->hdmarx, DMA_IT_HT);
        channel->restart_requested = 0U;
    }
}

/**
 * @brief  配置 UART RX 引脚为复用输入。
 * @note   四路信号监测仅接收数据，不配置 TX 引脚。
 */
static void signal_uart_rx_gpio_init(GPIO_TypeDef *port, uint32_t pin, uint32_t alternate)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = alternate;
    HAL_GPIO_Init(port, &gpio);
}

/**
 * @brief  使用 HAL 初始化一条 UART 和它的 RX DMA。
 * @note   DMA 采用普通模式，帧结束后停止，等待任务处理完成再重启。
 */
static HAL_StatusTypeDef signal_uart_dma_init(UART_HandleTypeDef *uart,
                                              DMA_HandleTypeDef *rx_dma,
                                              USART_TypeDef *uart_instance,
                                              DMA_Stream_TypeDef *dma_stream,
                                              uint32_t dma_channel,
                                              uint32_t baud,
                                              uint8_t *rx_buffer)
{
    uart->Instance = uart_instance;
    uart->Init.BaudRate = baud;
    uart->Init.WordLength = UART_WORDLENGTH_8B;
    uart->Init.StopBits = UART_STOPBITS_1;
    uart->Init.Parity = UART_PARITY_NONE;
    uart->Init.Mode = UART_MODE_RX;
    uart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart->Init.OverSampling = UART_OVERSAMPLING_16;

    rx_dma->Instance = dma_stream;
    rx_dma->Init.Channel = dma_channel;
    rx_dma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    rx_dma->Init.PeriphInc = DMA_PINC_DISABLE;
    rx_dma->Init.MemInc = DMA_MINC_ENABLE;
    rx_dma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    rx_dma->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    rx_dma->Init.Mode = DMA_NORMAL;
    rx_dma->Init.Priority = DMA_PRIORITY_HIGH;
    rx_dma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    rx_dma->Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    rx_dma->Init.MemBurst = DMA_MBURST_SINGLE;
    rx_dma->Init.PeriphBurst = DMA_PBURST_SINGLE;

    if (HAL_DMA_Init(rx_dma) != HAL_OK)
    {
        return HAL_ERROR;
    }

    __HAL_LINKDMA(uart, hdmarx, *rx_dma);

    if (HAL_UART_Init(uart) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_UARTEx_ReceiveToIdle_DMA(uart, rx_buffer, SIGNAL_MONITOR_DMA_RX_SIZE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    __HAL_DMA_DISABLE_IT(rx_dma, DMA_IT_HT);
    return HAL_OK;
}

static void signal_channel_state_init(signal_uart_channel_t *channel,
                                      UART_HandleTypeDef *uart,
                                      uint8_t *dma_rx_buffer,
                                      uint8_t *frame_buffer)
{
    memset(channel, 0, sizeof(*channel));
    channel->uart = uart;
    channel->dma_rx_buffer = dma_rx_buffer;
    channel->frame_buffer = frame_buffer;
    channel->enabled = 1U;
    channel->restart_requested = 1U;
}

/**
 * @brief  初始化四路信号监测 UART、DMA、GPIO 和中断优先级。
 */
static HAL_StatusTypeDef signal_monitor_uart_init(uint32_t heading_baud,
                                                   uint32_t motion_baud,
                                                   uint32_t gnss_baud,
                                                   uint32_t svs_baud)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_UART5_CLK_ENABLE();

    signal_uart_rx_gpio_init(GPIOB, GPIO_PIN_7, GPIO_AF7_USART1);
    signal_uart_rx_gpio_init(GPIOA, GPIO_PIN_3, GPIO_AF7_USART2);
    signal_uart_rx_gpio_init(GPIOD, GPIO_PIN_9, GPIO_AF7_USART3);
    signal_uart_rx_gpio_init(GPIOD, GPIO_PIN_2, GPIO_AF8_UART5);

    if (signal_uart_dma_init(&g_heading_uart, &g_heading_rx_dma,
                             USART1, DMA2_Stream2, DMA_CHANNEL_4,
                             heading_baud, g_heading_dma_rx_buffer) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (signal_uart_dma_init(&g_motion_uart, &g_motion_rx_dma,
                             USART2, DMA1_Stream5, DMA_CHANNEL_4,
                             motion_baud, g_motion_dma_rx_buffer) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (signal_uart_dma_init(&g_gnss_uart, &g_gnss_rx_dma,
                             USART3, DMA1_Stream1, DMA_CHANNEL_4,
                             gnss_baud, g_gnss_dma_rx_buffer) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (signal_uart_dma_init(&g_svs_uart, &g_svs_rx_dma,
                             UART5, DMA1_Stream0, DMA_CHANNEL_4,
                             svs_baud, g_svs_dma_rx_buffer) != HAL_OK)
    {
        return HAL_ERROR;
    }

    HAL_NVIC_SetPriority(USART1_IRQn, 6U, 0U);
    HAL_NVIC_SetPriority(USART2_IRQn, 6U, 0U);
    HAL_NVIC_SetPriority(USART3_IRQn, 6U, 0U);
    HAL_NVIC_SetPriority(UART5_IRQn, 6U, 0U);
    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 6U, 0U);
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 6U, 0U);
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 6U, 0U);
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 6U, 0U);

    HAL_NVIC_EnableIRQ(USART1_IRQn);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    HAL_NVIC_EnableIRQ(UART5_IRQn);
    HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

    return HAL_OK;
}

/**
 * @brief  初始化信号监测模块。
 */
void signal_monitor_init(uint32_t heading_baud,
                         uint32_t motion_baud,
                         uint32_t gnss_baud,
                         uint32_t svs_baud)
{
    signal_channel_state_init(&g_heading_channel, &g_heading_uart,
                              g_heading_dma_rx_buffer, g_heading_frame_buffer);
    signal_channel_state_init(&g_motion_channel, &g_motion_uart,
                              g_motion_dma_rx_buffer, g_motion_frame_buffer);
    signal_channel_state_init(&g_gnss_channel, &g_gnss_uart,
                              g_gnss_dma_rx_buffer, g_gnss_frame_buffer);
    signal_channel_state_init(&g_svs_channel, &g_svs_uart,
                              g_svs_dma_rx_buffer, g_svs_frame_buffer);

    if (signal_monitor_uart_init(heading_baud, motion_baud, gnss_baud, svs_baud) == HAL_OK)
    {
        g_heading_channel.restart_requested = 0U;
        g_motion_channel.restart_requested = 0U;
        g_gnss_channel.restart_requested = 0U;
        g_svs_channel.restart_requested = 0U;
    }

    signal_heading_led_set(0U);
    signal_motion_led_set(0U);
    signal_gnss_led_set(0U);
    signal_svs_led_set(0U);
    signal_pps_led_set(0U);
    signal_sync_led_set(0U);
}

/**
 * @brief  设置 SVS 是否由湿端输入。
 * @param  enable: 1 为湿端输入，0 为干端 UART5 输入。
 */
void signal_monitor_svs_wet_input_set(uint8_t enable)
{
    g_svs_wet_input_enabled = (enable != 0U) ? 1U : 0U;
    g_svs_channel.enabled = (g_svs_wet_input_enabled == 0U) ? 1U : 0U;
    g_svs_channel.led_on = 0U;
    g_svs_channel.last_valid_tick = 0U;

    if (g_svs_wet_input_enabled != 0U)
    {
        signal_svs_led_set_blue();
    }
    else
    {
        signal_svs_led_set(0U);
        signal_monitor_sync_mode_set(SIGNAL_MONITOR_SYNC_UNCONFIGURED);
    }
}

uint8_t signal_monitor_svs_wet_input_is_enabled(void)
{
    return g_svs_wet_input_enabled;
}

/**
 * @brief  启用或关闭指定 UART 信号的协议有效性监测。
 */
void signal_monitor_enable(signal_monitor_channel_t channel, uint8_t enable)
{
    signal_uart_channel_t *target = NULL;
    signal_led_set_t led_set = NULL;

    switch (channel)
    {
        case SIGNAL_MONITOR_HEADING:
            target = &g_heading_channel;
            led_set = signal_heading_led_set;
            break;
        case SIGNAL_MONITOR_MOTION:
            target = &g_motion_channel;
            led_set = signal_motion_led_set;
            break;
        case SIGNAL_MONITOR_GNSS:
            target = &g_gnss_channel;
            led_set = signal_gnss_led_set;
            break;
        case SIGNAL_MONITOR_SVS:
            target = &g_svs_channel;
            led_set = signal_svs_led_set;
            break;
        default:
            break;
    }

    if (target != NULL)
    {
        target->enabled = (enable != 0U) ? 1U : 0U;
        target->led_on = 0U;
        target->last_valid_tick = 0U;

        if (target->enabled == 0U)
        {
            led_set(0U);
        }
    }
}

static uint8_t signal_uart_status_get(const signal_uart_channel_t *channel, TickType_t now)
{
    if ((channel->enabled == 0U) || (channel->led_on == 0U))
    {
        return 0U;
    }

    return ((now - channel->last_valid_tick) <
            pdMS_TO_TICKS(SIGNAL_MONITOR_TIMEOUT_MS)) ? 1U : 0U;
}

void signal_monitor_get_status(signal_monitor_status_t *status)
{
    TickType_t now;

    if (status == NULL)
    {
        return;
    }

    now = xTaskGetTickCount();
    status->heading_status = signal_uart_status_get(&g_heading_channel, now);
    status->motion_status = signal_uart_status_get(&g_motion_channel, now);
    status->gnss_status = signal_uart_status_get(&g_gnss_channel, now);
    status->svs_status = signal_uart_status_get(&g_svs_channel, now);
}

static void signal_monitor_sync_enable(uint8_t enable)
{
    g_sync_monitor_enabled = (enable != 0U) ? 1U : 0U;

    if (g_sync_monitor_enabled == 0U)
    {
        g_sync_led_on = 0U;
        g_sync_last_tick = 0U;
        g_sync_event_flag = 0U;
        signal_sync_led_set(0U);
    }
}

void signal_monitor_sync_mode_set(signal_monitor_sync_mode_t mode)
{
    if (g_svs_wet_input_enabled == 0U)
    {
        mode = SIGNAL_MONITOR_SYNC_UNCONFIGURED;
    }

    if ((mode != SIGNAL_MONITOR_SYNC_INPUT) &&
        (mode != SIGNAL_MONITOR_SYNC_OUTPUT))
    {
        mode = SIGNAL_MONITOR_SYNC_UNCONFIGURED;
    }

    g_sync_mode = mode;
    signal_monitor_sync_enable((mode == SIGNAL_MONITOR_SYNC_INPUT) ? 1U : 0U);
}

signal_monitor_sync_mode_t signal_monitor_sync_mode_get(void)
{
    return g_sync_mode;
}

/**
 * @brief  处理 PPS、SYNC 外部中断置位的事件标志和 LED 超时。
 */
static void signal_exti_event_check(TickType_t now)
{
    if (g_pps_flag != 0U)
    {
        g_pps_flag = 0U;

        if (g_pps_led_on == 0U)
        {
            g_pps_led_on = 1U;
            signal_pps_led_set(1U);
        }
    }

    if (g_sync_monitor_enabled == 0U)
    {
        return;
    }

    if (g_sync_event_flag != 0U)
    {
        g_sync_event_flag = 0U;
        g_sync_last_tick = now;

        if (g_sync_led_on == 0U)
        {
            g_sync_led_on = 1U;
            signal_sync_led_set(1U);
        }
    }

    if ((g_sync_led_on != 0U) &&
        ((now - g_sync_last_tick) >= pdMS_TO_TICKS(SIGNAL_MONITOR_TIMEOUT_MS)))
    {
        g_sync_led_on = 0U;
        signal_sync_led_set(0U);
    }
}

/**
 * @brief  根据 HAL UART 句柄找到对应的业务通道。
 */
static signal_uart_channel_t *signal_channel_from_uart(UART_HandleTypeDef *uart)
{
    if (uart == &g_heading_uart)
    {
        return &g_heading_channel;
    }
    if (uart == &g_motion_uart)
    {
        return &g_motion_channel;
    }
    if (uart == &g_gnss_uart)
    {
        return &g_gnss_channel;
    }
    if (uart == &g_svs_uart)
    {
        return &g_svs_channel;
    }

    return NULL;
}

/**
 * @brief  HAL UART 空闲接收完成回调。
 * @param  uart: 产生 IDLE 或 DMA 满事件的 UART 句柄。
 * @param  size: 本帧 DMA 实际收到的字节数。
 *
 * @note
 * 此函数在中断上下文运行。DMA 为普通模式，进入本回调时当前接收已停止，
 * 所以可以将 DMA 缓冲区复制到任务处理缓冲区。任务随后负责解析和重启 DMA。
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *uart, uint16_t size)
{
    signal_uart_channel_t *channel;
    BaseType_t higher_priority_task_woken = pdFALSE;

    channel = signal_channel_from_uart(uart);
    if (channel == NULL)
    {
        return;
    }

    if ((size > 0U) && (size <= SIGNAL_MONITOR_DMA_RX_SIZE))
    {
        memcpy(channel->frame_buffer, channel->dma_rx_buffer, size);
        channel->frame_length = size;
        channel->frame_ready = 1U;
    }

    /* 即使长度为 0，也请求由任务重新启动 DMA。 */
    channel->restart_requested = 1U;

    if (g_signal_monitor_task_handle != NULL)
    {
        vTaskNotifyGiveFromISR(g_signal_monitor_task_handle, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

/**
 * @brief  HAL UART 错误回调。
 * @note   不在中断中阻塞等待 DMA 终止；只通知任务尝试重新启动接收。
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    signal_uart_channel_t *channel;
    BaseType_t higher_priority_task_woken = pdFALSE;

    channel = signal_channel_from_uart(uart);
    if (channel == NULL)
    {
        return;
    }

    channel->restart_requested = 1U;

    if (g_signal_monitor_task_handle != NULL)
    {
        vTaskNotifyGiveFromISR(g_signal_monitor_task_handle, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

/* UART、DMA 中断入口仅转交给 HAL，由 HAL 处理 IDLE、DMA 完成和错误状态。 */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_heading_uart);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_motion_uart);
}

void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_gnss_uart);
}

void UART5_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_svs_uart);
}

void DMA2_Stream2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_heading_rx_dma);
}

void DMA1_Stream5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_motion_rx_dma);
}

void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_gnss_rx_dma);
}

void DMA1_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_svs_rx_dma);
}

/**
 * @brief  信号监测 FreeRTOS 任务。
 *
 * 工作顺序：
 * 1. 等待 UART DMA 完成回调的任务通知，最长 50ms 醒来一次；
 * 2. 检查各通道完成标志，解析业务缓冲区中是否包含有效协议；
 * 3. 根据有效性更新 LED；
 * 4. 清除完成标志并重新启动下一帧 DMA 接收；
 * 5. 检查各路信号超时和 PPS/SYNC 外部中断事件。
 */
void signal_monitor_task(void *pvParameters)
{
    TickType_t now;

    (void)pvParameters;

    for (;;)
    {
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SIGNAL_MONITOR_TIMEOUT_CHECK_MS));
        now = xTaskGetTickCount();

        signal_uart_process_frame(&g_heading_channel, now, signal_heading_parse_byte);
        signal_uart_process_frame(&g_motion_channel, now, signal_motion_parse_byte);
        signal_uart_process_frame(&g_gnss_channel, now, signal_gnss_parse_byte);
        signal_uart_process_frame(&g_svs_channel, now, signal_svs_parse_byte);

        signal_uart_restart_rx(&g_heading_channel);
        signal_uart_restart_rx(&g_motion_channel);
        signal_uart_restart_rx(&g_gnss_channel);
        signal_uart_restart_rx(&g_svs_channel);

        signal_uart_timeout_check(&g_heading_channel, now, signal_heading_led_set);
        signal_uart_timeout_check(&g_motion_channel, now, signal_motion_led_set);
        signal_uart_timeout_check(&g_gnss_channel, now, signal_gnss_led_set);
        signal_uart_timeout_check(&g_svs_channel, now, signal_svs_led_set);
        signal_exti_event_check(now);
    }
}

/**
 * @brief  创建信号监测任务。
 * @retval pdPASS：创建成功或任务已存在；其他值：创建失败。
 */
BaseType_t signal_monitor_start(void)
{
    if (g_signal_monitor_task_handle != NULL)
    {
        return pdPASS;
    }

    return xTaskCreate(signal_monitor_task,
                       "signal_monitor",
                       512U,
                       NULL,
                       9U,
                       &g_signal_monitor_task_handle);
}

/**
 * @brief  配置项目默认信号输入模式。
 */
void def_work_mode_init(void)
{
    heading_set_input_mode(HEADING_INPUT_RS232);
    motion_set_input_mode(MOTION_INPUT_RS232);
    gnss_set_input_mode(GNSS_INPUT_RS232);
    pps_set_input_mode(PPS_INPUT_TTL);

    /* 默认 SVS 由湿端输入，干端 SVS LED 显示蓝色。 */
    signal_monitor_svs_wet_input_set(1U);

    /* 默认监测外部 SYNC 上升沿。 */
    sync_config(SYNC_MODE_INPUT, SYNC_EDGE_RISING);
    signal_monitor_sync_mode_set(SIGNAL_MONITOR_SYNC_INPUT);
}
