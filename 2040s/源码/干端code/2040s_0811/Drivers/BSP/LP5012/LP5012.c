#include "./BSP/LP5012/LP5012.h"
#include "./BSP/I2C/I2C.h"
#include "./SYSTEM/delay/delay.h"
#include <stdio.h>

/*
 * LP5012 使用硬件 I2C1 通信。
 *
 * 重要说明：
 * 1. TMP175、EEPROM、LP5012 共用 I2C1 的 PB6/PB9。
 * 2. 这里不能再使用软件 I2C 重新配置 PB6/PB9。
 * 3. main.c 中必须先调用 i2c1_init()，再调用 LP5012_Init_All()。
 */

/**
 * @brief  通过 HAL I2C 写 LP5012 单个寄存器
 * @param  addr: LP5012 8 位 I2C 地址，例如 LP5012_U1_ADDR / LP5012_U2_ADDR
 * @param  reg : LP5012 寄存器地址
 * @param  data: 写入寄存器的数据
 */
void LP5012_HAL_Write_Reg(uint8_t addr, uint8_t reg, uint8_t data)
{
    (void)HAL_I2C_Mem_Write(&hi2c1, addr, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

/**
 * @brief  对外保留的寄存器写接口
 * @param  addr: LP5012 8 位 I2C 地址
 * @param  reg : LP5012 寄存器地址
 * @param  data: 写入寄存器的数据
 */
void LP5012_Write_Reg(uint8_t addr, uint8_t reg, uint8_t data)
{
    LP5012_HAL_Write_Reg(addr, reg, data);
}

/**
 * @brief  初始化一颗 LP5012
 * @param  addr: LP5012 8 位 I2C 地址
 */
static void lp5012_dev_init(uint8_t addr)
{
    uint8_t i;

    /* 使能芯片，0x40 对应 LP5012 芯片使能配置 */
    LP5012_HAL_Write_Reg(addr, REG_CONFIG0, 0x40);

    /* 给芯片一点内部上电配置时间 */
    delay_ms(2);

    /* 再写一次使能，保证上电后配置稳定 */
    LP5012_HAL_Write_Reg(addr, REG_CONFIG0, 0x40);

    /* 给芯片一点内部上电配置时间 */
    delay_ms(2);

    /* 设置全局亮度为最大，后续单色亮度由 RGB 寄存器控制 */
    LP5012_HAL_Write_Reg(addr, REG_BANK_BRIGHT, 0xFF);

    /* 关闭 Bank 混色控制，使用独立 RGB 输出寄存器 */
    LP5012_HAL_Write_Reg(addr, 0x02, 0x00);

    /* 使能 4 组 LED 亮度通道 */
    for (i = 0; i < 4; i++)
    {
        LP5012_HAL_Write_Reg(addr, REG_LED_BRIGHT + i, 0xFF);
    }
}

/**
 * @brief  把一颗 LP5012 的 12 个输出通道写成相同值
 * @param  addr: LP5012 8 位 I2C 地址
 * @param  value: 输出值
 */
static void lp5012_out_all(uint8_t addr, uint8_t value)
{
    uint8_t i;

    for (i = 0; i < 12; i++)
    {
        LP5012_HAL_Write_Reg(addr, 0x0B + i, value);
    }
}

/**
 * @brief  LP5012 总初始化入口
 * @note   调用前必须已经完成 i2c1_init()。
 */
void LP5012_Init_All(void)
{
    delay_ms(10);

    /* 初始化 U1，I2C 地址 0x28 */
    lp5012_dev_init(LP5012_U1_ADDR);

    /* 初始化 U2，I2C 地址 0x2C */
    lp5012_dev_init(LP5012_U2_ADDR);

    /* 初始化后先关闭所有 RGB 输出，后续由 signal_monitor 设置具体状态 */
    lp5012_out_all(LP5012_U1_ADDR, 0x00);
    lp5012_out_all(LP5012_U2_ADDR, 0x00);
}

/**
 * @brief  LP5012 上电自检
 * @note   两颗 LP5012 的绿色通道低亮度点亮。
 */
void LP5012_PowerOn_SelfTest(void)
{
    uint8_t i;
    uint8_t green = 0x33;

    lp5012_out_all(LP5012_U1_ADDR, 0x00);
    for (i = 1; i < 12; i += 3)
    {
        LP5012_HAL_Write_Reg(LP5012_U1_ADDR, 0x0B + i, green);
    }

    lp5012_out_all(LP5012_U2_ADDR, 0x00);
    for (i = 1; i < 12; i += 3)
    {
        LP5012_HAL_Write_Reg(LP5012_U2_ADDR, 0x0B + i, green);
    }

    printf("LP5012 self test: green\r\n");
}

/**
 * @brief  板载普通 PPS 状态 GPIO 初始化
 * @note   这个不是 LP5012 RGB 通道，是 PA4 普通 GPIO 状态灯。
 */
void lp5012_pps_led_g_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init_struct.Pin = GPIO_PIN_4;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);

    BOARD_LP5012_STATUS_LED_SET(0);
}

void LP5012_U1_Set_D2(uint8_t r, uint8_t g, uint8_t b)
{
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D2_R, r);
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D2_G, g);
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D2_B, b);
}

void LP5012_U1_Set_D3(uint8_t r, uint8_t g, uint8_t b)
{
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D3_R, r);
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D3_G, g);
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D3_B, b);
}

void LP5012_U1_Set_D4(uint8_t r, uint8_t g, uint8_t b)
{
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D4_R, r);
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D4_G, g);
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D4_B, b);
}

void LP5012_U1_Set_D5(uint8_t r, uint8_t g, uint8_t b)
{
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D5_R, r);
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D5_G, g);
    LP5012_HAL_Write_Reg(LP5012_U1_ADDR, LP5012_U1_D5_B, b);
}

void LP5012_U2_Set_D6(uint8_t r, uint8_t g, uint8_t b)
{
    LP5012_HAL_Write_Reg(LP5012_U2_ADDR, LP5012_U2_D6_R, r);
    LP5012_HAL_Write_Reg(LP5012_U2_ADDR, LP5012_U2_D6_G, g);
    LP5012_HAL_Write_Reg(LP5012_U2_ADDR, LP5012_U2_D6_B, b);
}

void LP5012_U2_Set_D7(uint8_t r, uint8_t g, uint8_t b)
{
    LP5012_HAL_Write_Reg(LP5012_U2_ADDR, LP5012_U2_D7_R, r);
    LP5012_HAL_Write_Reg(LP5012_U2_ADDR, LP5012_U2_D7_G, g);
    LP5012_HAL_Write_Reg(LP5012_U2_ADDR, LP5012_U2_D7_B, b);
}

void LP5012_U2_Set_D8(uint8_t r, uint8_t g, uint8_t b)
{
    LP5012_HAL_Write_Reg(LP5012_U2_ADDR, LP5012_U2_D8_R, r);
    LP5012_HAL_Write_Reg(LP5012_U2_ADDR, LP5012_U2_D8_G, g);
    LP5012_HAL_Write_Reg(LP5012_U2_ADDR, LP5012_U2_D8_B, b);
}

/**
 * @brief  设置一颗 LP5012 上所有 RGB LED 为相同颜色
 * @param  chip: LP5012_U1_ADDR 或 LP5012_U2_ADDR
 * @param  r: 红色亮度
 * @param  g: 绿色亮度
 * @param  b: 蓝色亮度
 */
void LP5012_HAL_SetAll(uint8_t chip, uint8_t r, uint8_t g, uint8_t b)
{
    static const uint8_t u1_base[4] = {0x0B, 0x0E, 0x11, 0x14};
    static const uint8_t u2_base[3] = {0x0E, 0x11, 0x14};
    const uint8_t *base;
    uint8_t i;
    uint8_t count;

    if (chip == LP5012_U1_ADDR)
    {
        base = u1_base;
        count = 4;
    }
    else
    {
        base = u2_base;
        count = 3;
    }

    for (i = 0; i < count; i++)
    {
        LP5012_HAL_Write_Reg(chip, base[i] + 0, r);
        LP5012_HAL_Write_Reg(chip, base[i] + 1, g);
        LP5012_HAL_Write_Reg(chip, base[i] + 2, b);
    }
}
