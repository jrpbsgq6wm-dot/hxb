#include "./BSP/LP5012/LP5012.h"
#include "./BSP/I2C/I2C.h"
#include "./SYSTEM/delay/delay.h"
#include <stdio.h>

/* 软件 I2C 引脚 */
#define LP5012_SCL_PORT  GPIOB
#define LP5012_SCL_PIN   GPIO_PIN_6
#define LP5012_SDA_PORT  GPIOB
#define LP5012_SDA_PIN   GPIO_PIN_9

/* 拉高/拉低/读 SDA */
#define SCL_H()   BOARD_LP5012_SCL_SET(1)
#define SCL_L()   BOARD_LP5012_SCL_SET(0)
#define SDA_H()   BOARD_LP5012_SDA_SET(1)
#define SDA_L()   BOARD_LP5012_SDA_SET(0)
#define SDA_R()   BOARD_LP5012_SDA_GET()

/* 很短的空转延时，用来凑软件 I2C 时序 */
static void dly(void)
{
    volatile uint16_t i = 420;
    while (i--) ;
}

/* 初始化软件 I2C 引脚 */
static void io_init(void)
{
    GPIO_InitTypeDef g = {0};

    /* 复位/释放 I2C1，确保相关外设状态干净 */
    __HAL_RCC_I2C1_FORCE_RESET();
    dly();
    dly();
    __HAL_RCC_I2C1_RELEASE_RESET();
    dly();
    dly();

    /* 打开 GPIOB 时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB6/PB9 配成开漏输出，上拉，模拟 I2C 总线 */
    g.Pin = LP5012_SCL_PIN | LP5012_SDA_PIN;
    g.Mode = GPIO_MODE_OUTPUT_OD;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LP5012_SCL_PORT, &g);

    /* 总线空闲状态：SCL/SDA 都拉高 */
    SCL_H();
    SDA_H();
    dly();
}

/* I2C 起始信号 START */
static void st(void)
{
    SDA_H();
    dly();
    SCL_H();
    dly();
    SDA_L();
    dly();
    SCL_L();
}

/* I2C 停止信号 STOP */
static void sp(void)
{
    SDA_L();
    dly();
    SCL_H();
    dly();
    SDA_H();
    dly();
}

/* 等待从机应答 ACK
 * 返回 0：收到 ACK
 * 返回 1：超时/未应答
 */
static uint8_t wa(void)
{
    uint8_t t = 0;

    dly();
    SCL_H();
    dly();

    while (SDA_R())
    {
        t++;
        if (t > 200)
        {
            SCL_L();
            return 1;
        }
        dly();
    }

    SCL_L();
    return 0;
}

/* 发送 1 个字节，MSB 先发 */
static void sb(uint8_t d)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (d & 0x80)
            SDA_H();
        else
            SDA_L();

        d <<= 1;
        dly();
        SCL_H();
        dly();
        SCL_L();
    }
}

/* 往某个寄存器写 1 字节数据
 * 流程：START -> addr -> reg -> data -> STOP
 */
static void wreg(uint8_t a, uint8_t r, uint8_t d)
{
    st();
    sb(a); wa();
    sb(r); wa();
    sb(d); wa();
    sp();
}

/* 初始化一颗 LP5012 芯片 */
static void dev_init(uint8_t addr)
{
    uint8_t i;
    /* 这些寄存器看起来是在做芯片上电配置 */
    wreg(addr, 0x00, 0x40);
    delay_ms(2);
    wreg(addr, 0x00, 0x40);
    delay_ms(2);
    wreg(addr, 0x03, 0xFF);
    wreg(addr, 0x02, 0x00);

    /* 打开若干通道 */
    for (i = 0; i < 4; i++)
        wreg(addr, 0x07 + i, 0xFF);
}

/* 把某颗芯片的 12 个通道都写成同一个值 */
static void out_all(uint8_t addr, uint8_t v)
{
    uint8_t i;
    for (i = 0; i < 12; i++)
        wreg(addr, 0x0B + i, v);
}

/* LP5012 总初始化入口 */
void LP5012_Init_All(void)
{
    io_init();
    delay_ms(10);

    /* 两颗灯驱芯片 */
    dev_init(0x28);
    dev_init(0x2C);
}

/* 上电自检
 * 先灭灯，再给两颗芯片的绿色通道点一个低亮度值
 */
void LP5012_PowerOn_SelfTest(void)
{
    uint8_t i;
    uint8_t g = 0x33;  /* 约 20% 亮度 */

    /* U1 全灭后，只点绿色 */
    out_all(0x28, 0x00);
    for (i = 1; i < 12; i += 3)
        wreg(0x28, 0x0B + i, g);

    /* U2 全灭后，只点绿色 */
    out_all(0x2C, 0x00);
    for (i = 1; i < 12; i += 3)
        wreg(0x2C, 0x0B + i, g);

    printf("U1+U2 green 5%%\n");
}

/* PPS 状态灯的绿色 GPIO 初始化 */
void lp5012_pps_led_g_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    g.Pin = GPIO_PIN_4;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);

    /* 默认关灯 */
    BOARD_LP5012_STATUS_LED_SET(0);
}

/* ================= U1 的 4 个 LED ================= */

void LP5012_U1_Set_D2(uint8_t r, uint8_t g, uint8_t b)
{
    wreg(LP5012_U1_ADDR, LP5012_U1_D2_R, r);
    wreg(LP5012_U1_ADDR, LP5012_U1_D2_G, g);
    wreg(LP5012_U1_ADDR, LP5012_U1_D2_B, b);
}

void LP5012_U1_Set_D3(uint8_t r, uint8_t g, uint8_t b)
{
    wreg(LP5012_U1_ADDR, LP5012_U1_D3_R, r);
    wreg(LP5012_U1_ADDR, LP5012_U1_D3_G, g);
    wreg(LP5012_U1_ADDR, LP5012_U1_D3_B, b);
}

void LP5012_U1_Set_D4(uint8_t r, uint8_t g, uint8_t b)
{
    wreg(LP5012_U1_ADDR, LP5012_U1_D4_R, r);
    wreg(LP5012_U1_ADDR, LP5012_U1_D4_G, g);
    wreg(LP5012_U1_ADDR, LP5012_U1_D4_B, b);
}

void LP5012_U1_Set_D5(uint8_t r, uint8_t g, uint8_t b)
{
    wreg(LP5012_U1_ADDR, LP5012_U1_D5_R, r);
    wreg(LP5012_U1_ADDR, LP5012_U1_D5_G, g);
    wreg(LP5012_U1_ADDR, LP5012_U1_D5_B, b);
}

/* ================= U2 的 3 个 LED ================= */

void LP5012_U2_Set_D6(uint8_t r, uint8_t g, uint8_t b)
{
    wreg(LP5012_U2_ADDR, LP5012_U2_D6_R, r);
    wreg(LP5012_U2_ADDR, LP5012_U2_D6_G, g);
    wreg(LP5012_U2_ADDR, LP5012_U2_D6_B, b);
}

void LP5012_U2_Set_D7(uint8_t r, uint8_t g, uint8_t b)
{
    wreg(LP5012_U2_ADDR, LP5012_U2_D7_R, r);
    wreg(LP5012_U2_ADDR, LP5012_U2_D7_G, g);
    wreg(LP5012_U2_ADDR, LP5012_U2_D7_B, b);
}

void LP5012_U2_Set_D8(uint8_t r, uint8_t g, uint8_t b)
{
    wreg(LP5012_U2_ADDR, LP5012_U2_D8_R, r);
    wreg(LP5012_U2_ADDR, LP5012_U2_D8_G, g);
    wreg(LP5012_U2_ADDR, LP5012_U2_D8_B, b);
}

/* ========== HAL I2C1 写寄存器版本 ========== */

/* 用 HAL I2C 写一个寄存器 */
void LP5012_HAL_Write_Reg(uint8_t addr, uint8_t reg, uint8_t data)
{
    HAL_I2C_Mem_Write(&hi2c1, addr, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

/* 给一颗芯片上的所有 LED 写相同 RGB 值 */
void LP5012_HAL_SetAll(uint8_t chip, uint8_t r, uint8_t g, uint8_t b)
{
    static const uint8_t u1_base[4] = {0x0B, 0x0E, 0x11, 0x14}; /* D2~D5 */
    static const uint8_t u2_base[3] = {0x0E, 0x11, 0x14};       /* D6~D8 */
    const uint8_t *base;
    uint8_t i, n;

    if (chip == LP5012_U1_ADDR)
    {
        base = u1_base;
        n = 4;
    }
    else
    {
        base = u2_base;
        n = 3;
    }

    for (i = 0; i < n; i++)
    {
        LP5012_HAL_Write_Reg(chip, base[i] + 0, r);
        LP5012_HAL_Write_Reg(chip, base[i] + 1, g);
        LP5012_HAL_Write_Reg(chip, base[i] + 2, b);
    }
}
