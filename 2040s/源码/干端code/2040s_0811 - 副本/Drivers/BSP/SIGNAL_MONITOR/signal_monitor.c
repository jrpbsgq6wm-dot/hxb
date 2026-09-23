/**
 ****************************************************************************************************
 * @file        signal_monitor.c
 * @brief       HEADING / MOTION / GNSS / SVS / PPS / SYNC 信号有效性监测
 *
 * @note
 *              设计原则：
 *              1. 串口接收使用 DMA 循环模式，避免 5ms/10ms 高频数据丢失。
 *              2. 串口中断只处理 IDLE 标志，不在中断中解析字符串、不点灯。
 *              3. FreeRTOS 任务周期性读取 DMA 新数据，并判断数据头是否有效。
 *              4. 检测到有效数据头后点亮对应 LED。
 *              5. 超过 SIGNAL_MONITOR_TIMEOUT_MS 没收到有效数据，则熄灭对应 LED。
 *
 *              信号与串口 / 引脚对应：
 *              HEADING : USART1_RX / PB7 / 有效头 "$HEHDT" / LED D5
 *              MOTION  : USART2_RX / PA3 / 有效头 ":"      / LED D4
 *              GNSS    : USART3_RX / PD9 / 有效头 "$GNGGA" 或 "$GPZDA" / LED D7
 *              SVS     : UART5_RX  / PD2 / 暂按收到非空字符有效 / LED D6
 *              PPS     : EXTI6     / PD6 / 外部脉冲事件 / LED D3
 *              SYNC    : EXTI11/13 / PD11/PD13 / 外部同步脉冲事件 / LED D8
 ****************************************************************************************************
 */

#include "signal_monitor.h"
#include "./BSP/LP5012/LP5012.h"
#include "./BSP/SIGNAL_SWITCH/signal_switch.h"

#include "FreeRTOS.h"
#include "task.h"

#include <string.h>


/* 串口数据头匹配状态结构体 */
typedef struct
{
    const char *pattern;             /* 需要匹配的数据头字符串 */
    uint8_t pos;                     /* 当前已经匹配到第几个字符 */
} signal_matcher_t;

/* 单路串口监测通道结构体 */
typedef struct
{
    USART_TypeDef *uart;             /* 当前通道使用的 USART/UART 外设 */
    DMA_Stream_TypeDef *dma;         /* 当前通道使用的 DMA Stream */
    uint8_t *dma_buf;                /* 当前通道的 DMA 循环接收缓冲区 */
    uint16_t dma_size;               /* DMA 缓冲区总长度 */
    uint16_t last_pos;               /* 上一次任务处理到的 DMA 写入位置 */
    volatile uint8_t rx_event;       /* 串口 IDLE 中断事件标志 */
    uint8_t enable;                  /* 当前通道是否参与监测 */
    uint8_t led_on;                  /* 当前通道 LED 是否已经点亮 */
    TickType_t last_valid_tick;      /* 最近一次收到有效数据的系统 tick */
} signal_uart_channel_t;

/* HEADING 串口 DMA 接收缓冲区 */
static uint8_t g_heading_dma_buf[SIGNAL_MONITOR_DMA_RX_SIZE];

/* MOTION 串口 DMA 接收缓冲区 */
static uint8_t g_motion_dma_buf[SIGNAL_MONITOR_DMA_RX_SIZE];

/* GNSS 串口 DMA 接收缓冲区 */
static uint8_t g_gnss_dma_buf[SIGNAL_MONITOR_DMA_RX_SIZE];

/* SVS 串口 DMA 接收缓冲区 */
static uint8_t g_svs_dma_buf[SIGNAL_MONITOR_DMA_RX_SIZE];

/* HEADING 通道运行状态 */
static signal_uart_channel_t g_heading_ch;

/* MOTION 通道运行状态 */
static signal_uart_channel_t g_motion_ch;

/* GNSS 通道运行状态 */
static signal_uart_channel_t g_gnss_ch;

/* SVS 通道运行状态 */
static signal_uart_channel_t g_svs_ch;

/* HEADING 数据头匹配器 */
static signal_matcher_t g_heading_match = {"$HEHDT", 0};

/* GNSS GGA 数据头匹配器 */
static signal_matcher_t g_gnss_gga_match = {"$GNGGA", 0};

/* GNSS ZDA 数据头匹配器 */
static signal_matcher_t g_gnss_zda_match = {"$GPZDA", 0};

/* PPS LED 当前状态 */
static uint8_t g_pps_led_on = 0;

/* SYNC LED 当前状态 */
static uint8_t g_sync_led_on = 0;

/* PPS 最近一次有效脉冲 tick */
static TickType_t g_pps_last_tick = 0;

/* SYNC 最近一次有效脉冲 tick */
static TickType_t g_sync_last_tick = 0;

/* 监测任务句柄 */
static TaskHandle_t g_signal_monitor_task_handle = NULL;

static uint8_t g_sync_monitor_enable = 0;

/* SVS 湿端输入标志
 * 1 = SVS 由湿端输入，干端不检测 UART5_RX，SVS 指示灯显示蓝色
 * 0 = SVS 由干端输入，干端检测 UART5_RX，有效绿灯，超时红灯
 */
static uint8_t g_svs_wet_input = 0;

/**
 * @brief  判断一个匹配器是否匹配到了完整数据头
 * @param  matcher: 数据头匹配器
 * @param  ch: 当前收到的一个字节
 * @retval 1 表示匹配到了完整数据头，0 表示还没有匹配成功
 */
static uint8_t signal_match_feed(signal_matcher_t *matcher, uint8_t ch)
{
    /* 如果当前字节等于期望位置的字符，说明本字节匹配成功 */
    if (ch == (uint8_t)matcher->pattern[matcher->pos])
    {
        /* 匹配位置后移，准备判断下一个字符 */
        matcher->pos++;

        /* 如果下一个字符是字符串结束符，说明整个数据头已经匹配完成 */
        if (matcher->pattern[matcher->pos] == '\0')
        {
            /* 匹配完成后清零，准备下一帧数据头匹配 */
            matcher->pos = 0;

            /* 返回 1，通知上层当前收到了一帧有效数据头 */
            return 1;
        }
    }
    else
    {
        /* 当前字节不匹配时，如果它刚好等于数据头第一个字符，则从第 1 个字符重新开始匹配 */
        matcher->pos = (ch == (uint8_t)matcher->pattern[0]) ? 1U : 0U;
    }

    /* 返回 0，表示当前还没有匹配到完整数据头 */
    return 0;
}


/**
 * @brief  点亮或者熄灭 HEADING LED
 * @param  on: 1 点亮，0 熄灭
 */
static void signal_heading_led_set(uint8_t on)
{
    /* 如果需要点亮，就把 HEADING 对应的 D5 设置为绿色 */
    if (on)
    {
        LP5012_U1_Set_D5(0, SIGNAL_MONITOR_LED_BRIGHT, 0);
    }
    else
    {
        LP5012_U1_Set_D5(SIGNAL_MONITOR_LED_BRIGHT, 0, 0);
    }
}


static void signal_motion_led_set(uint8_t on)
{
    if (on) LP5012_U1_Set_D4(0, SIGNAL_MONITOR_LED_BRIGHT, 0);
    else    LP5012_U1_Set_D4(SIGNAL_MONITOR_LED_BRIGHT, 0, 0);
}

static void signal_gnss_led_set(uint8_t on)
{
    if (on) LP5012_U2_Set_D7(0, SIGNAL_MONITOR_LED_BRIGHT, 0);
    else    LP5012_U2_Set_D7(SIGNAL_MONITOR_LED_BRIGHT, 0, 0);
}

static void signal_svs_led_set(uint8_t on)
{
    if (on) LP5012_U2_Set_D6(0, SIGNAL_MONITOR_LED_BRIGHT, 0);
    else    LP5012_U2_Set_D6(SIGNAL_MONITOR_LED_BRIGHT, 0, 0);
}

static void signal_pps_led_set(uint8_t on)
{
    if (on) LP5012_U1_Set_D3(0, SIGNAL_MONITOR_LED_BRIGHT, 0);
    else    LP5012_U1_Set_D3(SIGNAL_MONITOR_LED_BRIGHT, 0, 0);
}

static void signal_sync_led_set(uint8_t on)
{
    if (on) LP5012_U2_Set_D8(0, SIGNAL_MONITOR_LED_BRIGHT, 0);
    else    LP5012_U2_Set_D8(SIGNAL_MONITOR_LED_BRIGHT, 0, 0);
}

static void signal_svs_led_set_blue(void)
{
    LP5012_U2_Set_D6(0, 0, SIGNAL_MONITOR_LED_BRIGHT);
}

/*
* @brief  使能或关闭某一路串口信号监测
* @param  channel: 串口监测通道
* @param  enable: 1 使能，0 禁用
*/
void signal_monitor_sync_enable(uint8_t enable)
{
    g_sync_monitor_enable = enable;

    if (enable == 0)
    {
        g_sync_led_on = 0;
        g_sync_last_tick = 0;
        g_sync_event_flag = 0;
        signal_sync_led_set(0);
    }
}

/**
 * @brief  标记某路串口通道收到有效数据
 * @param  ch: 串口监测通道
 * @param  now: 当前 FreeRTOS tick
 * @param  led_set: 当前通道对应的 LED 控制函数
 */
static void signal_uart_mark_valid(signal_uart_channel_t *ch,
                                   TickType_t now,
                                   void (*led_set)(uint8_t))
{
    /* 记录最近一次收到有效数据的时间，用于后续 1s 超时判断 */
    ch->last_valid_tick = now;

    /* 如果 LED 当前是灭的，则马上点亮，避免重复 I2C 写入 */
    if (ch->led_on == 0)
    {
        ch->led_on = 1;
        led_set(1);
    }
}


/**
 * @brief  根据通道状态处理 LED 超时
 * @param  ch: 串口监测通道
 * @param  now: 当前 FreeRTOS tick
 * @param  led_set: 当前通道对应的 LED 控制函数
 */
static void signal_uart_led_timeout_check(signal_uart_channel_t *ch,
                                          TickType_t now,
                                          void (*led_set)(uint8_t))
{
    /* 如果当前通道被禁用，则直接保证 LED 熄灭 */
    if (ch->enable == 0)
    {
        if (ch->led_on)
        {
            ch->led_on = 0;
            led_set(0);
        }

        return;
    }

    /* 如果从来没有收到过有效数据，则保持 LED 熄灭 */
    if (ch->last_valid_tick == 0)
    {
        return;
    }

    /* 如果距离上次有效数据已经超过 1s，则熄灭 LED */
    if ((now - ch->last_valid_tick) >= pdMS_TO_TICKS(SIGNAL_MONITOR_TIMEOUT_MS))
    {
        if (ch->led_on)
        {
            ch->led_on = 0;
            led_set(0);
        }
    }
}


/**
 * @brief  解析 HEADING 收到的一个字节
 * @param  data: 当前字节
 * @param  now: 当前 FreeRTOS tick
 */
static void signal_heading_parse_byte(uint8_t data, TickType_t now)
{
    /* HEADING 只判断数据头 "$HEHDT" */
    if (signal_match_feed(&g_heading_match, data))
    {
        signal_uart_mark_valid(&g_heading_ch, now, signal_heading_led_set);
    }
}


/**
 * @brief  解析 MOTION 收到的一个字节
 * @param  data: 当前字节
 * @param  now: 当前 FreeRTOS tick
 */
static void signal_motion_parse_byte(uint8_t data, TickType_t now)
{
    /* MOTION 当前需求只判断数据头 ':' */
    if (data == ':')
    {
        signal_uart_mark_valid(&g_motion_ch, now, signal_motion_led_set);
    }
}


/**
 * @brief  解析 GNSS 收到的一个字节
 * @param  data: 当前字节
 * @param  now: 当前 FreeRTOS tick
 */
static void signal_gnss_parse_byte(uint8_t data, TickType_t now)
{
    /* GNSS 判断 "$GNGGA" 或 "$GPZDA"，任意一个匹配成功都认为 GNSS 数据有效 */
    if (signal_match_feed(&g_gnss_gga_match, data) ||
        signal_match_feed(&g_gnss_zda_match, data))
    {
        signal_uart_mark_valid(&g_gnss_ch, now, signal_gnss_led_set);
    }
}


/**
 * @brief  解析 SVS 收到的一个字节
 * @param  data: 当前字节
 * @param  now: 当前 FreeRTOS tick
 * @note   你还没有给 SVS 的具体帧头，所以这里暂时按收到非空字符认为有效。
 */
static void signal_svs_parse_byte(uint8_t data, TickType_t now)
{
    /* 判断当前字节是否为空格字符 */
    if (data == ' ')
    {
        /* 收到空格帧头，认为 SVS 当前有有效数据输入 */
        signal_uart_mark_valid(&g_svs_ch, now, signal_svs_led_set);
    }
}
/**
 * @brief  处理 DMA 缓冲区中新收到的数据
 * @param  ch: 串口监测通道
 * @param  now: 当前 FreeRTOS tick
 * @param  parse_byte: 当前通道对应的字节解析函数
 */
static void signal_uart_poll_dma(signal_uart_channel_t *ch,
                                 TickType_t now,
                                 void (*parse_byte)(uint8_t, TickType_t))
{
    uint16_t pos;
    uint16_t i;

    /* 如果通道没有使能，就不解析该路数据 */
    if (ch->enable == 0)
    {
        return;
    }

    /* 通过 NDTR 反推出 DMA 当前已经写到缓冲区的哪个位置 */
    pos = ch->dma_size - (uint16_t)ch->dma->NDTR;

    /* 如果当前位置没有变化，说明没有新数据进入 */
    if (pos == ch->last_pos)
    {
        return;
    }

    /* 如果当前位置大于上次位置，说明数据没有发生环形回绕 */
    if (pos > ch->last_pos)
    {
        for (i = ch->last_pos; i < pos; i++)
        {
            parse_byte(ch->dma_buf[i], now);
        }
    }
    else
    {
        /* 如果当前位置小于上次位置，说明 DMA 已经从缓冲区尾部回绕到头部 */
        for (i = ch->last_pos; i < ch->dma_size; i++)
        {
            parse_byte(ch->dma_buf[i], now);
        }

        for (i = 0; i < pos; i++)
        {
            parse_byte(ch->dma_buf[i], now);
        }
    }

    /* 更新已经处理到的位置，下一次从这里继续 */
    ch->last_pos = pos;

    /* 清掉 IDLE 事件标志，表示本轮新数据已经被任务处理 */
    ch->rx_event = 0;
}


/**
 * @brief  初始化 USART RX GPIO
 * @param  gpio: GPIO 端口
 * @param  pin: GPIO 引脚
 * @param  af: GPIO 复用功能
 */
static void signal_uart_rx_gpio_init(GPIO_TypeDef *gpio, uint32_t pin, uint32_t af)
{
    GPIO_InitTypeDef gpio_init_struct;

    /* 先把结构体清零，避免未赋值字段影响 GPIO 初始化 */
    memset(&gpio_init_struct, 0, sizeof(gpio_init_struct));

    /* 设置当前 RX 引脚 */
    gpio_init_struct.Pin = pin;

    /* 设置为复用功能模式，让引脚连接到 USART/UART 外设 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;

    /* RX 输入一般使用上拉，空闲状态保持高电平 */
    gpio_init_struct.Pull = GPIO_PULLUP;

    /* 串口输入速度设置为高速，保证边沿质量 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;

    /* 设置当前引脚对应的 USART/UART AF 编号 */
    gpio_init_struct.Alternate = af;

    /* 执行 GPIO 初始化 */
    HAL_GPIO_Init(gpio, &gpio_init_struct);
}


/**
 * @brief  清除 DMA Stream 的全部中断标志
 * @param  dma: DMA 控制器
 * @param  stream: DMA Stream
 */
static void signal_dma_clear_flags(DMA_TypeDef *dma, DMA_Stream_TypeDef *stream)
{
    /* DMA1 Stream0 的标志位位于 LIFCR 的低位区域 */
    if ((dma == DMA1) && (stream == DMA1_Stream0))
    {
        DMA1->LIFCR = DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 |
                      DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0;
    }

    /* DMA1 Stream1 的标志位位于 LIFCR */
    if ((dma == DMA1) && (stream == DMA1_Stream1))
    {
        DMA1->LIFCR = DMA_LIFCR_CFEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CTEIF1 |
                      DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTCIF1;
    }

    /* DMA1 Stream5 的标志位位于 HIFCR */
    if ((dma == DMA1) && (stream == DMA1_Stream5))
    {
        DMA1->HIFCR = DMA_HIFCR_CFEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CTEIF5 |
                      DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTCIF5;
    }

    /* DMA2 Stream2 的标志位位于 LIFCR */
    if ((dma == DMA2) && (stream == DMA2_Stream2))
    {
        DMA2->LIFCR = DMA_LIFCR_CFEIF2 | DMA_LIFCR_CDMEIF2 | DMA_LIFCR_CTEIF2 |
                      DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTCIF2;
    }
}


/**
 * @brief  初始化一路 UART RX DMA 循环接收
 * @param  uart: USART/UART 外设
 * @param  dma: DMA 控制器
 * @param  stream: DMA Stream
 * @param  channel: DMA 通道编号，STM32F407 上本设计均使用 Channel 4
 * @param  buf: DMA 接收缓冲区
 * @param  size: DMA 接收缓冲区长度
 */
static void signal_uart_dma_rx_init(USART_TypeDef *uart,
                                    DMA_TypeDef *dma,
                                    DMA_Stream_TypeDef *stream,
                                    uint32_t channel,
                                    uint8_t *buf,
                                    uint16_t size)
{
    /* 关闭 DMA Stream，修改 DMA 配置前必须先关闭 */
    stream->CR &= ~DMA_SxCR_EN;

    /* 等待 DMA Stream 真正关闭 */
    while (stream->CR & DMA_SxCR_EN)
    {
    }

    /* 清除当前 DMA Stream 可能残留的中断标志 */
    signal_dma_clear_flags(dma, stream);

    /* 设置外设地址，UART RX DMA 的外设地址就是 USART_DR */
    stream->PAR = (uint32_t)&uart->DR;

    /* 设置内存地址，DMA 收到的数据会写入这个缓冲区 */
    stream->M0AR = (uint32_t)buf;

    /* 设置 DMA 本轮传输长度，循环模式下 NDTR 会自动重新装载 */
    stream->NDTR = size;

    /* 先清空 CR，避免旧配置残留 */
    stream->CR = 0;

    /* 选择 DMA Channel */
    stream->CR |= channel;

    /* 设置为外设到内存方向 */
    stream->CR |= 0U << DMA_SxCR_DIR_Pos;

    /* 使能内存地址自增，这样每收到 1 字节会写入下一个 buffer 位置 */
    stream->CR |= DMA_SxCR_MINC;

    /* 使能循环模式，写到 buffer 尾部后自动回到 buffer 开头 */
    stream->CR |= DMA_SxCR_CIRC;

    /* 设置 DMA 优先级为高，降低高频数据丢失风险 */
    stream->CR |= DMA_SxCR_PL_1;

    /* FIFO 不使用，保持直接模式即可 */
    stream->FCR = 0;

    /* 打开 DMA Stream，开始搬运 UART RX 数据 */
    stream->CR |= DMA_SxCR_EN;

    /* 使能 USART/UART 的 DMA 接收请求 */
    uart->CR3 |= USART_CR3_DMAR;
}


/**
 * @brief  初始化一路 USART/UART 接收外设
 * @param  uart: USART/UART 外设
 * @param  baud: 波特率
 */
static void signal_uart_base_init(USART_TypeDef *uart, uint32_t baud)
{
    uint32_t pclk;
    uint32_t keep_te;

    /* 保存原来的 TE 位，避免 USART1 如果已经用于 printf TX，被这里关掉发送功能 */
    keep_te = uart->CR1 & USART_CR1_TE;

    /* USART1 挂在 APB2，总线时钟取 PCLK2 */
    if (uart == USART1)
    {
        pclk = HAL_RCC_GetPCLK2Freq();
    }
    else
    {
        /* USART2 / USART3 / UART5 挂在 APB1，总线时钟取 PCLK1 */
        pclk = HAL_RCC_GetPCLK1Freq();
    }

    /* 配置波特率，OVER8=0 时 BRR 可以按 PCLK / baud 近似四舍五入计算 */
    uart->BRR = (pclk + (baud / 2U)) / baud;

    /* 关闭 LIN / 停止位等特殊配置，保持 8N1 普通串口模式 */
    uart->CR2 = 0;

    /* 保留 DMA 接收位，其他高级流控默认关闭 */
    uart->CR3 &= USART_CR3_DMAR;

    /* 使能接收、IDLE 中断、串口外设，同时保留原来的发送使能位 */
    uart->CR1 = keep_te | USART_CR1_RE | USART_CR1_IDLEIE | USART_CR1_UE;

    /* 读 SR 和 DR，用于清除可能残留的 RXNE / IDLE / ORE 状态 */
    (void)uart->SR;
    (void)uart->DR;
}


/**
 * @brief  初始化 HEADING 串口接收
 * @param  baud: HEADING 串口波特率
 */
static void signal_heading_uart_init(uint32_t baud)
{
    /* 打开 GPIOB 时钟，因为 HEADING RX 使用 PB7 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 打开 USART1 时钟 */
    __HAL_RCC_USART1_CLK_ENABLE();

    /* 打开 DMA2 时钟，因为 USART1_RX 使用 DMA2 */
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* PB7 配置为 USART1_RX，复用功能 AF7 */
    signal_uart_rx_gpio_init(GPIOB, GPIO_PIN_7, GPIO_AF7_USART1);

    /* 初始化 USART1 基础接收参数 */
    signal_uart_base_init(USART1, baud);

    /* USART1_RX 使用 DMA2_Stream2 / Channel4 */
    signal_uart_dma_rx_init(USART1,
                            DMA2,
                            DMA2_Stream2,
                            DMA_CHANNEL_4,
                            g_heading_dma_buf,
                            SIGNAL_MONITOR_DMA_RX_SIZE);

    /* 配置 USART1 中断优先级 */
    HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);

    /* 使能 USART1 中断，用于接收 IDLE 事件 */
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}


/**
 * @brief  初始化 MOTION 串口接收
 * @param  baud: MOTION 串口波特率
 */
static void signal_motion_uart_init(uint32_t baud)
{
    /* 打开 GPIOA 时钟，因为 MOTION RX 使用 PA3 */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 打开 USART2 时钟 */
    __HAL_RCC_USART2_CLK_ENABLE();

    /* 打开 DMA1 时钟，因为 USART2_RX 使用 DMA1 */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* PA3 配置为 USART2_RX，复用功能 AF7 */
    signal_uart_rx_gpio_init(GPIOA, GPIO_PIN_3, GPIO_AF7_USART2);

    /* 初始化 USART2 基础接收参数 */
    signal_uart_base_init(USART2, baud);

    /* USART2_RX 使用 DMA1_Stream5 / Channel4 */
    signal_uart_dma_rx_init(USART2,
                            DMA1,
                            DMA1_Stream5,
                            DMA_CHANNEL_4,
                            g_motion_dma_buf,
                            SIGNAL_MONITOR_DMA_RX_SIZE);

    /* 配置 USART2 中断优先级 */
    HAL_NVIC_SetPriority(USART2_IRQn, 6, 0);

    /* 使能 USART2 中断，用于接收 IDLE 事件 */
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}


/**
 * @brief  初始化 GNSS 串口接收
 * @param  baud: GNSS 串口波特率
 */
static void signal_gnss_uart_init(uint32_t baud)
{
    /* 打开 GPIOD 时钟，因为 GNSS RX 使用 PD9 */
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* 打开 USART3 时钟 */
    __HAL_RCC_USART3_CLK_ENABLE();

    /* 打开 DMA1 时钟，因为 USART3_RX 使用 DMA1 */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* PD9 配置为 USART3_RX，复用功能 AF7 */
    signal_uart_rx_gpio_init(GPIOD, GPIO_PIN_9, GPIO_AF7_USART3);

    /* 初始化 USART3 基础接收参数 */
    signal_uart_base_init(USART3, baud);

    /* USART3_RX 使用 DMA1_Stream1 / Channel4 */
    signal_uart_dma_rx_init(USART3,
                            DMA1,
                            DMA1_Stream1,
                            DMA_CHANNEL_4,
                            g_gnss_dma_buf,
                            SIGNAL_MONITOR_DMA_RX_SIZE);

    /* 配置 USART3 中断优先级 */
    HAL_NVIC_SetPriority(USART3_IRQn, 6, 0);

    /* 使能 USART3 中断，用于接收 IDLE 事件 */
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}


/**
 * @brief  初始化 SVS 串口接收
 * @param  baud: SVS 串口波特率
 */
static void signal_svs_uart_init(uint32_t baud)
{
    /* 打开 GPIOD 时钟，因为 SVS RX 使用 PD2 */
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* 打开 UART5 时钟 */
    __HAL_RCC_UART5_CLK_ENABLE();

    /* 打开 DMA1 时钟，因为 UART5_RX 使用 DMA1 */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* PD2 配置为 UART5_RX，复用功能 AF8 */
    signal_uart_rx_gpio_init(GPIOD, GPIO_PIN_2, GPIO_AF8_UART5);

    /* 初始化 UART5 基础接收参数 */
    signal_uart_base_init(UART5, baud);

    /* UART5_RX 使用 DMA1_Stream0 / Channel4 */
    signal_uart_dma_rx_init(UART5,
                            DMA1,
                            DMA1_Stream0,
                            DMA_CHANNEL_4,
                            g_svs_dma_buf,
                            SIGNAL_MONITOR_DMA_RX_SIZE);

    /* 配置 UART5 中断优先级 */
    HAL_NVIC_SetPriority(UART5_IRQn, 6, 0);

    /* 使能 UART5 中断，用于接收 IDLE 事件 */
    HAL_NVIC_EnableIRQ(UART5_IRQn);
}


/**
 * @brief  初始化单个监测通道的软件状态
 * @param  ch: 监测通道
 * @param  uart: USART/UART 外设
 * @param  dma: DMA Stream
 * @param  buf: DMA 缓冲区
 */
static void signal_channel_state_init(signal_uart_channel_t *ch,
                                      USART_TypeDef *uart,
                                      DMA_Stream_TypeDef *dma,
                                      uint8_t *buf)
{
    /* 保存当前通道使用的串口外设 */
    ch->uart = uart;

    /* 保存当前通道使用的 DMA Stream */
    ch->dma = dma;

    /* 保存当前通道使用的 DMA 接收缓冲区 */
    ch->dma_buf = buf;

    /* 保存 DMA 接收缓冲区长度 */
    ch->dma_size = SIGNAL_MONITOR_DMA_RX_SIZE;

    /* 初始处理位置为 0 */
    ch->last_pos = 0;

    /* 初始时没有 IDLE 接收事件 */
    ch->rx_event = 0;

    /* 默认使能该通道监测，后续可通过 signal_monitor_enable 单独关闭 */
    ch->enable = 1;

    /* 初始 LED 状态为灭 */
    ch->led_on = 0;

    /* 初始认为还没有收到有效数据 */
    ch->last_valid_tick = 0;
}


/**
 * @brief  信号监测模块初始化
 * @param  heading_baud: HEADING 波特率
 * @param  motion_baud: MOTION 波特率
 * @param  gnss_baud: GNSS 波特率
 * @param  svs_baud: SVS 波特率
 */
void signal_monitor_init(uint32_t heading_baud,
                         uint32_t motion_baud,
                         uint32_t gnss_baud,
                         uint32_t svs_baud)
{
    /* 初始化 HEADING 软件状态 */
    signal_channel_state_init(&g_heading_ch, USART1, DMA2_Stream2, g_heading_dma_buf);

    /* 初始化 MOTION 软件状态 */
    signal_channel_state_init(&g_motion_ch, USART2, DMA1_Stream5, g_motion_dma_buf);

    /* 初始化 GNSS 软件状态 */
    signal_channel_state_init(&g_gnss_ch, USART3, DMA1_Stream1, g_gnss_dma_buf);

    /* 初始化 SVS 软件状态 */
    signal_channel_state_init(&g_svs_ch, UART5, DMA1_Stream0, g_svs_dma_buf);

    /* 初始化 HEADING USART1_RX + DMA */
    signal_heading_uart_init(heading_baud);

    /* 初始化 MOTION USART2_RX + DMA */
    signal_motion_uart_init(motion_baud);

    /* 初始化 GNSS USART3_RX + DMA */
    signal_gnss_uart_init(gnss_baud);

    /* 初始化 SVS UART5_RX + DMA */
    signal_svs_uart_init(svs_baud);

    /* 初始化时先熄灭 HEADING LED */
    signal_heading_led_set(0);

    /* 初始化时先熄灭 MOTION LED */
    signal_motion_led_set(0);

    /* 初始化时先熄灭 GNSS LED */
    signal_gnss_led_set(0);

    /* 初始化时先熄灭 SVS LED */
    signal_svs_led_set(0);

    /* 初始化时先熄灭 PPS LED */
    signal_pps_led_set(0);

    /* 初始化时先熄灭 SYNC LED */
    signal_sync_led_set(0);
}

/*
 * @brief  设置 SVS 湿端输入模式
 * @param  enable: 1 使能湿端输入，0 关闭湿端输入
 */
void signal_monitor_svs_wet_input_set(uint8_t enable)
{
    g_svs_wet_input = enable;

    if (enable)
    {
        /* SVS 湿端输入时，干端不再检测 UART5_RX */
        g_svs_ch.enable = 0;

        /* 清除 SVS 旧状态，避免之前的有效状态影响当前显示 */
        g_svs_ch.led_on = 0;
        g_svs_ch.last_valid_tick = 0;

        /* SVS 湿端输入固定显示蓝色 */
        signal_svs_led_set_blue();
    }
    else
    {
        /* 退出湿端输入后，恢复 SVS 干端输入监测 */
        g_svs_ch.enable = 1;

        /* 恢复后先显示红色，等收到有效 SVS 数据头再变绿 */
        g_svs_ch.led_on = 0;
        g_svs_ch.last_valid_tick = 0;
        signal_svs_led_set(0);
    }
}

/**
 * @brief  使能或关闭某一路监测
 * @param  channel: 需要配置的监测通道
 * @param  enable: 1 使能，0 关闭
 */
void signal_monitor_enable(signal_monitor_channel_t channel, uint8_t enable)
{
    /* 根据通道枚举选择对应的通道状态 */
    switch (channel)
    {
        case SIGNAL_MONITOR_HEADING:
            g_heading_ch.enable = enable;
            if (enable == 0)
            {
                g_heading_ch.led_on = 0;
                signal_heading_led_set(0);
            }
            break;

        case SIGNAL_MONITOR_MOTION:
            g_motion_ch.enable = enable;
            if (enable == 0)
            {
                g_motion_ch.led_on = 0;
                signal_motion_led_set(0);
            }
            break;

        case SIGNAL_MONITOR_GNSS:
            g_gnss_ch.enable = enable;
            if (enable == 0)
            {
                g_gnss_ch.led_on = 0;
                signal_gnss_led_set(0);
            }
            break;

        case SIGNAL_MONITOR_SVS:
            g_svs_ch.enable = enable;
            if (enable == 0)
            {
                g_svs_ch.led_on = 0;
                signal_svs_led_set(0);
            }
            break;

        default:
            break;
    }
}


/**
 * @brief  串口 IDLE 中断处理入口
 * @param  uart: 产生中断的 USART/UART 外设
 * @note   这个函数只在中断里清标志和置位，不做字符串解析。
 */
void signal_monitor_uart_idle_irq(USART_TypeDef *uart)
{
    /* 判断当前串口是否产生了 IDLE 中断 */
    if ((uart->SR & USART_SR_IDLE) != 0)
    {
        /* 读 SR 再读 DR，是 STM32F4 清除 IDLE 标志的要求 */
        (void)uart->SR;
        (void)uart->DR;

        /* 如果是 USART1，则标记 HEADING 有接收事件 */
        if (uart == USART1)
        {
            g_heading_ch.rx_event = 1;
        }

        /* 如果是 USART2，则标记 MOTION 有接收事件 */
        if (uart == USART2)
        {
            g_motion_ch.rx_event = 1;
        }

        /* 如果是 USART3，则标记 GNSS 有接收事件 */
        if (uart == USART3)
        {
            g_gnss_ch.rx_event = 1;
        }

        /* 如果是 UART5，则标记 SVS 有接收事件 */
        if (uart == UART5)
        {
            g_svs_ch.rx_event = 1;
        }
    }

    /* 如果发生溢出错误，读 DR 清除 ORE，避免后续接收卡死 */
    if ((uart->SR & USART_SR_ORE) != 0)
    {
        (void)uart->DR;
    }
}


/**
 * @brief  USART1 中断服务函数
 * @note   如果你工程里已经有 USART1_IRQHandler，需要把这一句合并进去，不能重复定义。
 */
void USART1_IRQHandler(void)
{
    signal_monitor_uart_idle_irq(USART1);
}


/**
 * @brief  USART2 中断服务函数
 * @note   如果你工程里已经有 USART2_IRQHandler，需要把这一句合并进去，不能重复定义。
 */
void USART2_IRQHandler(void)
{
    signal_monitor_uart_idle_irq(USART2);
}


/**
 * @brief  USART3 中断服务函数
 * @note   如果你工程里已经有 USART3_IRQHandler，需要把这一句合并进去，不能重复定义。
 */
void USART3_IRQHandler(void)
{
    signal_monitor_uart_idle_irq(USART3);
}


/**
 * @brief  UART5 中断服务函数
 * @note   如果你工程里已经有 UART5_IRQHandler，需要把这一句合并进去，不能重复定义。
 */
void UART5_IRQHandler(void)
{
    signal_monitor_uart_idle_irq(UART5);
}


/**
 * @brief  处理 PPS 和 SYNC 的外部中断事件标志
 * @param  now: 当前 FreeRTOS tick
 */
static void signal_exti_event_check(TickType_t now)
{
    /* 如果 PPS 外部中断置位了 g_pps_flag，说明已经收到 PPS 脉冲 */
    if (g_pps_flag)
    {
        /* 清除 PPS 事件标志，避免同一个 PPS 脉冲被重复处理 */
        g_pps_flag = 0;

        /* PPS 只要收到一次就持续点亮，不做 1s 超时熄灭 */
        if (g_pps_led_on == 0)
        {
            /* 记录 PPS LED 已经点亮，避免任务每 5ms 重复写 LP5012 */
            g_pps_led_on = 1;

            /* 点亮 PPS 对应的 D3 指示灯 */
            signal_pps_led_set(1);
        }
    }

    /* 如果 SYNC 监测被关闭，说明当前处于 SVS 干端输入模式，SYNC 不参与业务监测 */
    if (g_sync_monitor_enable == 0){
        /* 清除可能残留的 SYNC 事件，避免重新使能后误点灯 */
        g_sync_event_flag = 0;

        /* 如果 SYNC LED 当前亮着，则立即熄灭 */
        if (g_sync_led_on)
        {
            g_sync_led_on = 0;
            signal_sync_led_set(0);
        }

        /* 清除 SYNC 最近有效时间 */
        g_sync_last_tick = 0;
    }else{
        /* 如果 SYNC 外部中断置位了 g_sync_event_flag，说明收到一次符合边沿设置的 SYNC 脉冲 */
        if (g_sync_event_flag)
        {
            /* 清除 SYNC 事件标志，避免同一个事件被重复处理 */
            g_sync_event_flag = 0;

            /* 记录 SYNC 最近一次有效脉冲时间 */
            g_sync_last_tick = now;

            /* 如果 SYNC LED 当前是灭的，则点亮 */
            if (g_sync_led_on == 0)
            {
                g_sync_led_on = 1;
                signal_sync_led_set(1);
            }
        }

        /* SYNC 超过 1s 没有新脉冲，则熄灭 SYNC LED */
        if ((g_sync_last_tick != 0) &&
            ((now - g_sync_last_tick) >= pdMS_TO_TICKS(SIGNAL_MONITOR_TIMEOUT_MS)))
        {
            if (g_sync_led_on)
            {
                g_sync_led_on = 0;
                signal_sync_led_set(0);
            }
        }
    }
}


/**
 * @brief  信号监测 FreeRTOS 任务
 * @param  pvParameters: FreeRTOS 任务参数，当前未使用
 */
void signal_monitor_task(void *pvParameters)
{
    TickType_t now;

    /* 当前任务不需要外部参数 */
    (void)pvParameters;

    while (1)
    {
        /* 获取当前系统 tick，用于有效数据时间戳和超时判断 */
        now = xTaskGetTickCount();

        /* 处理 HEADING DMA 新收到的数据 */
        signal_uart_poll_dma(&g_heading_ch, now, signal_heading_parse_byte);

        /* 处理 MOTION DMA 新收到的数据 */
        signal_uart_poll_dma(&g_motion_ch, now, signal_motion_parse_byte);

        /* 处理 GNSS DMA 新收到的数据 */
        signal_uart_poll_dma(&g_gnss_ch, now, signal_gnss_parse_byte);

        /* 处理 SVS DMA 新收到的数据 */
        signal_uart_poll_dma(&g_svs_ch, now, signal_svs_parse_byte);

        /* 检查 HEADING 是否超过 1s 没有有效数据 */
        signal_uart_led_timeout_check(&g_heading_ch, now, signal_heading_led_set);

        /* 检查 MOTION 是否超过 1s 没有有效数据 */
        signal_uart_led_timeout_check(&g_motion_ch, now, signal_motion_led_set);

        /* 检查 GNSS 是否超过 1s 没有有效数据 */
        signal_uart_led_timeout_check(&g_gnss_ch, now, signal_gnss_led_set);

        /* 检查 SVS 是否超过 1s 没有有效数据 */
        signal_uart_led_timeout_check(&g_svs_ch, now, signal_svs_led_set);

        /* 处理 PPS / SYNC 外部中断事件和 LED 超时 */
        signal_exti_event_check(now);

        /* 任务延时 5ms，兼顾实时性和 CPU 占用 */
        vTaskDelay(pdMS_TO_TICKS(SIGNAL_MONITOR_TASK_PERIOD_MS));
    }
}


/**
 * @brief  创建信号监测任务
 * @retval pdPASS 创建成功，其它值表示创建失败
 */
BaseType_t signal_monitor_start(void)
{
    /* 如果任务已经创建过，则直接返回成功，避免重复创建 */
    if (g_signal_monitor_task_handle != NULL)
    {
        return pdPASS;
    }

    /* 创建信号监测任务 */
    return xTaskCreate((TaskFunction_t)signal_monitor_task,
                       (const char *)"signal_monitor",
                       (uint16_t)512,
                       (void *)NULL,
                       (UBaseType_t)9,
                       (TaskHandle_t *)&g_signal_monitor_task_handle);
}

void def_work_mode_init(void)
{
    heading_set_input_mode(HEADING_INPUT_RS232);
    motion_set_input_mode(MOTION_INPUT_RS232);
    gnss_set_input_mode(GNSS_INPUT_RS232);
    pps_set_input_mode(PPS_INPUT_TTL);

    /* 默认 SVS 湿端输入：SVS 干端不检测，SVS 灯蓝色 */
    signal_monitor_svs_wet_input_set(1);

    /* 默认 SYNC 可用：输入模式，上升沿 */
    sync_config(SYNC_MODE_INPUT, SYNC_EDGE_RISING);
    signal_monitor_sync_enable(1);
}
