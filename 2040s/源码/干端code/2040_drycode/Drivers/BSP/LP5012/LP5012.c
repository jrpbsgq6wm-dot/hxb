   #include "./BSP/LP5012/LP5012.h"
#include "./BSP/I2C/I2C.h"
#include "./SYSTEM/delay/delay.h"
#include <stdio.h>

#define SCL_PORT  GPIOB
#define SCL_PIN   GPIO_PIN_6
#define SDA_PORT  GPIOB
#define SDA_PIN   GPIO_PIN_9

#define SCL_H()   HAL_GPIO_WritePin(SCL_PORT,SCL_PIN,GPIO_PIN_SET)
#define SCL_L()   HAL_GPIO_WritePin(SCL_PORT,SCL_PIN,GPIO_PIN_RESET)
#define SDA_H()   HAL_GPIO_WritePin(SDA_PORT,SDA_PIN,GPIO_PIN_SET)
#define SDA_L()   HAL_GPIO_WritePin(SDA_PORT,SDA_PIN,GPIO_PIN_RESET)
#define SDA_R()   HAL_GPIO_ReadPin(SDA_PORT,SDA_PIN)

static void dly(void){volatile uint16_t i=420;while(i--);}

static void io_init(void)
{
    GPIO_InitTypeDef g={0};
    __HAL_RCC_I2C1_FORCE_RESET();dly();dly();
    __HAL_RCC_I2C1_RELEASE_RESET();dly();dly();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    g.Pin=SCL_PIN|SDA_PIN;g.Mode=GPIO_MODE_OUTPUT_OD;
    g.Pull=GPIO_PULLUP;g.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SCL_PORT,&g);
    SCL_H();SDA_H();dly();
}
static void st(void){SDA_H();dly();SCL_H();dly();SDA_L();dly();SCL_L();}
static void sp(void){SDA_L();dly();SCL_H();dly();SDA_H();dly();}
static uint8_t wa(void){uint8_t t=0;dly();SCL_H();dly();while(SDA_R()){t++;if(t>200){SCL_L();return 1;}dly();}SCL_L();return 0;}
static void sb(uint8_t d){uint8_t i;for(i=0;i<8;i++){if(d&0x80)SDA_H();else SDA_L();d<<=1;dly();SCL_H();dly();SCL_L();}}
static void wreg(uint8_t a,uint8_t r,uint8_t d){st();sb(a);wa();sb(r);wa();sb(d);wa();sp();}

static void dev_init(uint8_t addr){uint8_t i;wreg(addr,0x00,0x40);delay_ms(2);wreg(addr,0x00,0x40);delay_ms(2);wreg(addr,0x03,0xFF);wreg(addr,0x02,0x00);for(i=0;i<4;i++)wreg(addr,0x07+i,0xFF);}
static void out_all(uint8_t addr,uint8_t v){uint8_t i;for(i=0;i<12;i++)wreg(addr,0x0B+i,v);}

void LP5012_Init_All(void)
{
    io_init();delay_ms(10);
    dev_init(0x28);
    dev_init(0x2C);
}

void LP5012_PowerOn_SelfTest(void)
{
    uint8_t i;
    uint8_t g = 0x33;  /* 20%        (51/255     20%) */

    /* U1              5%        */
    out_all(0x28,0x00);
    for(i=1;i<12;i+=3)wreg(0x28,0x0B+i,g);

    /* U2              5%        */
    out_all(0x2C,0x00);
    for(i=1;i<12;i+=3)wreg(0x2C,0x0B+i,g);

    printf("U1+U2 green 5%%\n");
}

void lp5012_pps_led_g_init(void)
{
    GPIO_InitTypeDef g={0};__HAL_RCC_GPIOA_CLK_ENABLE();
    g.Pin=GPIO_PIN_4;g.Mode=GPIO_MODE_OUTPUT_PP;g.Pull=GPIO_NOPULL;g.Speed=GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA,&g);HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
}

/* ========== U1                    ========== */

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

/* ========== U2                    ========== */

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

/* ========== HAL I2C1 based writes (after i2c1_init) ========== */

void LP5012_HAL_Write_Reg(uint8_t addr, uint8_t reg, uint8_t data)
{
    HAL_I2C_Mem_Write(&hi2c1, addr, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

/* Set every LED on one chip to the same RGB.  chip: LP5012_U1_ADDR / LP5012_U2_ADDR */
void LP5012_HAL_SetAll(uint8_t chip, uint8_t r, uint8_t g, uint8_t b)
{
    static const uint8_t u1_base[4] = {0x0B, 0x0E, 0x11, 0x14}; /* D2,D3,D4,D5 */
    static const uint8_t u2_base[3] = {0x0E, 0x11, 0x14};       /* D6,D7,D8     */
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
