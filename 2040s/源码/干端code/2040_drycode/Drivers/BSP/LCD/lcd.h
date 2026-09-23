   /**
 ****************************************************************************************************
 * @file        lcd.h
 * @author                  ()
 * @version     V1.1
 * @date        2023-05-29
 * @brief       2.8  /3.5  /4.3  /7   TFTLCD(MCU  )         
 *                      IC        :ILI9341/NT35310/NT35510/SSD1963/ST7789/ST7796/ILI9806  
 *
 * @license     Copyright (c) 2020-2032,                           
 ****************************************************************************************************
 * @attention
 *
 *         :                F407      
 *         :www.yuanzige.com
 *         :www.openedv.com
 *         :www..com
 *         :openedv.taobao.com
 *
 *         
 * V1.0 20211016
 *           
 * V1.1 20230529
 * 1        ST7796  ILI9806 IC    
 * 2                          
 ****************************************************************************************************
 */

#ifndef __LCD_H
#define __LCD_H

#include "stdlib.h"
#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/* LCD RST/WR/RD/BL/CS/RS           
 * LCD_D0~D15,            ,                ,      lcd_init        .                ,      
 *   6  IO  ,       LCD_Init      D0~D15      IO  .
 */

/* RESET                                   RESET     */
//#define LCD_RST_GPIO_PORT               GPIOx
//#define LCD_RST_GPIO_PIN                SYS_GPIO_PINx
//#define LCD_RST_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOx_CLK_ENABLE(); }while(0)   /*     IO           */

#define LCD_WR_GPIO_PORT                GPIOD
#define LCD_WR_GPIO_PIN                 GPIO_PIN_5
#define LCD_WR_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)     /*     IO           */

#define LCD_RD_GPIO_PORT                GPIOD
#define LCD_RD_GPIO_PIN                 GPIO_PIN_4
#define LCD_RD_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)     /*     IO           */

#define LCD_BL_GPIO_PORT                GPIOB
#define LCD_BL_GPIO_PIN                 GPIO_PIN_15
#define LCD_BL_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)     /*         IO           */

/* LCD_CS(        LCD_FSMC_NEX          IO  )    LCD_RS(        LCD_FSMC_AX          IO  )           */
#define LCD_CS_GPIO_PORT                GPIOG
#define LCD_CS_GPIO_PIN                 GPIO_PIN_12
#define LCD_CS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)     /*     IO           */

#define LCD_RS_GPIO_PORT                GPIOF
#define LCD_RS_GPIO_PIN                 GPIO_PIN_12
#define LCD_RS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)     /*     IO           */

/* FSMC              
 *     :               FSMC  1      LCD,   1  4      : FSMC_NE1~4
 *
 *     LCD_FSMC_NEX,       LCD_CS_GPIO              
 *     LCD_FSMC_AX ,       LCD_RS_GPIO              
 */
#define LCD_FSMC_NEX         4              /*     FSMC_NE4  LCD_CS,              : 1~4 */
#define LCD_FSMC_AX          6              /*     FSMC_A6  LCD_RS,          : 0 ~ 25 */

#define LCD_FSMC_BCRX        FSMC_Bank1->BTCR[(LCD_FSMC_NEX - 1) * 2]       /* BCR      ,    LCD_FSMC_NEX         */
#define LCD_FSMC_BTRX        FSMC_Bank1->BTCR[(LCD_FSMC_NEX - 1) * 2 + 1]   /* BTR      ,    LCD_FSMC_NEX         */
#define LCD_FSMC_BWTRX       FSMC_Bank1E->BWTR[(LCD_FSMC_NEX - 1) * 2]      /* BWTR      ,    LCD_FSMC_NEX         */

/******************************************************************************************/

/* LCD           */
typedef struct
{
    uint16_t width;     /* LCD      */
    uint16_t height;    /* LCD      */
    uint16_t id;        /* LCD ID */
    uint8_t dir;        /*                   0        1         */
    uint16_t wramcmd;   /*       gram     */
    uint16_t setxcmd;   /*     x         */
    uint16_t setycmd;   /*     y         */
} _lcd_dev;

/* LCD     */
extern _lcd_dev lcddev; /*     LCD         */

/* LCD                   */
extern uint32_t  g_point_color;     /*          */
extern uint32_t  g_back_color;      /*         .           */

/* LCD         */
#define LCD_BL(x)   do{ x ? \
                      HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_SET) : \
                      HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_RESET); \
                     }while(0)

/* LCD           */
typedef struct
{
    volatile uint16_t LCD_REG;
    volatile uint16_t LCD_RAM;
} LCD_TypeDef;


/* LCD_BASE              :
 *             FSMC    1(BANK1)      TFTLCD      (MCU  ),   1                256MB,      4  :
 *       1(FSMC_NE1)        : 0x6000 0000 ~ 0x63FF FFFF
 *       2(FSMC_NE2)        : 0x6400 0000 ~ 0x67FF FFFF
 *       3(FSMC_NE3)        : 0x6800 0000 ~ 0x6BFF FFFF
 *       4(FSMC_NE4)        : 0x6C00 0000 ~ 0x6FFF FFFF
 *
 *                                       (    LCD_CS)        (    LCD_RS)
 *       F407          FSMC_NE4    LCD_CS, FSMC_A6    LCD_RS ,16        ,            :
 *     FSMC_NE4          : 0x6C00 0000;     NEX        (x=1/2/3/4): 0x6000 0000 + (0x400 0000 * (x - 1))
 * FSMC_A6          : 2^6 * 2 = 0x80;    FSMC_Ay            (y = 0 ~ 25): 2^y * 2
 *
 * LCD->LCD_REG,    LCD_RS = 0(LCD      ); LCD->LCD_RAM,    LCD_RS = 1(LCD    )
 *    LCD->LCD_RAM        :  0x6C00 0000 + 2^6 * 2 = 0x6C00 0080
 *    LCD->LCD_REG             LCD->LCD_RAM              .
 *                       LCD_REG    LCD_RAM(REG    ,RAM    ,    16          )
 *                    (LCD_BASE) = LCD_RAM - 2 = 0x6C00 0080 -2
 *
 *                     ((      FSMC_NEX)X=1/2/3/4, (RS        FSMC_Ay)y=0~25):
 *          LCD_BASE = (0x6000 0000 + (0x400 0000 * (x - 1))) | (2^y * 2 -2)
 *                (            )
 *          LCD_BASE = (0x6000 0000 + (0x400 0000 * (x - 1))) | ((1 << y) * 2 -2)
 */
#define LCD_BASE        (uint32_t)((0x60000000 + (0x4000000 * (LCD_FSMC_NEX - 1))) | (((1 << LCD_FSMC_AX) * 2) -2))
#define LCD             ((LCD_TypeDef *) LCD_BASE)

/******************************************************************************************/
/* LCD                    */

/*              */
#define L2R_U2D         0           /*         ,         */
#define L2R_D2U         1           /*         ,         */
#define R2L_U2D         2           /*         ,         */
#define R2L_D2U         3           /*         ,         */

#define U2D_L2R         4           /*         ,         */
#define U2D_R2L         5           /*         ,         */
#define D2U_L2R         6           /*         ,         */
#define D2U_R2L         7           /*         ,         */

#define DFT_SCAN_DIR    L2R_U2D     /*                */

/*              */
#define WHITE           0xFFFF      /*      */
#define BLACK           0x0000      /*      */
#define RED             0xF800      /*      */
#define GREEN           0x07E0      /*      */
#define BLUE            0x001F      /*      */ 
#define MAGENTA         0xF81F      /*       /       = BLUE + RED */
#define YELLOW          0xFFE0      /*      = GREEN + RED */
#define CYAN            0x07FF      /*      = GREEN + BLUE */  

/*            */
#define BROWN           0xBC40      /*      */
#define BRRED           0xFC07      /*        */
#define GRAY            0x8430      /*      */ 
#define DARKBLUE        0x01CF      /*        */
#define LIGHTBLUE       0x7D7C      /*        */ 
#define GRAYBLUE        0x5458      /*        */ 
#define LIGHTGREEN      0x841F      /*        */  
#define LGRAY           0xC618      /*       (PANNEL),           */ 
#define LGRAYBLUE       0xA651      /*         (          ) */ 
#define LBBLUE          0x2B12      /*         (              ) */ 

/******************************************************************************************/
/* SSD1963            (          ) */

/* LCD           */ 
#define SSD_HOR_RESOLUTION      800     /* LCD           */ 
#define SSD_VER_RESOLUTION      480     /* LCD           */ 

/* LCD             */ 
#define SSD_HOR_PULSE_WIDTH     1       /*          */ 
#define SSD_HOR_BACK_PORCH      46      /*          */ 
#define SSD_HOR_FRONT_PORCH     210     /*          */ 

#define SSD_VER_PULSE_WIDTH     1       /*          */ 
#define SSD_VER_BACK_PORCH      23      /*          */ 
#define SSD_VER_FRONT_PORCH     22      /*          */ 

/*                        */ 
#define SSD_HT          (SSD_HOR_RESOLUTION + SSD_HOR_BACK_PORCH + SSD_HOR_FRONT_PORCH)
#define SSD_HPS         (SSD_HOR_BACK_PORCH)
#define SSD_VT          (SSD_VER_RESOLUTION + SSD_VER_BACK_PORCH + SSD_VER_FRONT_PORCH)
#define SSD_VPS         (SSD_VER_BACK_PORCH)
   
/******************************************************************************************/
/*          */

void lcd_wr_data(volatile uint16_t data);            /* LCD       */
void lcd_wr_regno(volatile uint16_t regno);          /* LCD            /     */
void lcd_write_reg(uint16_t regno, uint16_t data);   /* LCD             */

void lcd_init(void);                        /*       LCD */ 
void lcd_display_on(void);                  /*        */ 
void lcd_display_off(void);                 /*        */
void lcd_scan_dir(uint8_t dir);             /*                */ 
void lcd_display_dir(uint8_t dir);          /*                  */ 
void lcd_ssd_backlight_set(uint8_t pwm);    /* SSD1963          */ 

void lcd_write_ram_prepare(void);                           /*       GRAM */ 
void lcd_set_cursor(uint16_t x, uint16_t y);                /*          */ 
uint32_t lcd_read_point(uint16_t x, uint16_t y);            /*     (32      ,    LTDC) */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);/*     (32      ,    LTDC) */

void lcd_clear(uint16_t color);                                                             /* LCD     */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);                   /*            */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);                  /*      */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color);                  /*          */
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);             /*          */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);          /*             (32      ,    LTDC) */
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);   /*              */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);     /*        */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);/*        */

void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);                       /*              */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);                     /*          */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);      /*              */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);   /*            */

#endif

















