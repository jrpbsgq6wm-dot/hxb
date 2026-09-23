#ifndef __BOARD_IO_H
#define __BOARD_IO_H

#include "stm32f4xx.h"

#define BITBAND(addr, bitnum) ((addr & 0xF0000000U) + 0x02000000U + ((addr & 0x000FFFFFU) << 5) + ((bitnum) << 2))
#define MEM_ADDR(addr)        (*((volatile unsigned long *)(addr)))
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND((addr), (bitnum)))

#define GPIOA_ODR_Addr   (GPIOA_BASE + 20U)
#define GPIOB_ODR_Addr   (GPIOB_BASE + 20U)
#define GPIOC_ODR_Addr   (GPIOC_BASE + 20U)
#define GPIOD_ODR_Addr   (GPIOD_BASE + 20U)
#define GPIOE_ODR_Addr   (GPIOE_BASE + 20U)
#define GPIOF_ODR_Addr   (GPIOF_BASE + 20U)
#define GPIOG_ODR_Addr   (GPIOG_BASE + 20U)
#define GPIOH_ODR_Addr   (GPIOH_BASE + 20U)

#define GPIOA_IDR_Addr   (GPIOA_BASE + 16U)
#define GPIOB_IDR_Addr   (GPIOB_BASE + 16U)
#define GPIOC_IDR_Addr   (GPIOC_BASE + 16U)
#define GPIOD_IDR_Addr   (GPIOD_BASE + 16U)
#define GPIOE_IDR_Addr   (GPIOE_BASE + 16U)
#define GPIOF_IDR_Addr   (GPIOF_BASE + 16U)
#define GPIOG_IDR_Addr   (GPIOG_BASE + 16U)
#define GPIOH_IDR_Addr   (GPIOH_BASE + 16U)

#define PAout(n) BIT_ADDR(GPIOA_ODR_Addr, (n))
#define PBout(n) BIT_ADDR(GPIOB_ODR_Addr, (n))
#define PCout(n) BIT_ADDR(GPIOC_ODR_Addr, (n))
#define PDout(n) BIT_ADDR(GPIOD_ODR_Addr, (n))
#define PEout(n) BIT_ADDR(GPIOE_ODR_Addr, (n))
#define PFout(n) BIT_ADDR(GPIOF_ODR_Addr, (n))
#define PGout(n) BIT_ADDR(GPIOG_ODR_Addr, (n))
#define PHout(n) BIT_ADDR(GPIOH_ODR_Addr, (n))

#define PAin(n)  BIT_ADDR(GPIOA_IDR_Addr, (n))
#define PBin(n)  BIT_ADDR(GPIOB_IDR_Addr, (n))
#define PCin(n)  BIT_ADDR(GPIOC_IDR_Addr, (n))
#define PDin(n)  BIT_ADDR(GPIOD_IDR_Addr, (n))
#define PEin(n)  BIT_ADDR(GPIOE_IDR_Addr, (n))
#define PFin(n)  BIT_ADDR(GPIOF_IDR_Addr, (n))
#define PGin(n)  BIT_ADDR(GPIOG_IDR_Addr, (n))
#define PHin(n)  BIT_ADDR(GPIOH_IDR_Addr, (n))

#define BOARD_LED1_SET(v)              do { PCout(8)  = ((v) != 0U); } while (0)
#define BOARD_LED2_SET(v)              do { PDout(15) = ((v) != 0U); } while (0)
#define BOARD_LED3_SET(v)              do { PDout(10) = ((v) != 0U); } while (0)

#define BOARD_MCU_KEY_GET()            PCin(0)

#define BOARD_SW_RESET_SET(v)          do { PEout(2)  = ((v) != 0U); } while (0)
#define BOARD_SONA_POWER_EN_SET(v)     do { PDout(14) = ((v) != 0U); } while (0)

#define BOARD_UART1_SW_SET(v)          do { PBout(8)  = ((v) != 0U); } while (0)
#define BOARD_UART2_SW_SET(v)          do { PBout(1)  = ((v) != 0U); } while (0)
#define BOARD_UART3_SW_SET(v)          do { PDout(8)  = ((v) != 0U); } while (0)
#define BOARD_PPS_SW_SET(v)            do { PDout(0)  = ((v) != 0U); } while (0)
#define BOARD_UART5_232_485_SW_SET(v)  do { PDout(4)  = ((v) != 0U); } while (0)
#define BOARD_UART5_RE_DE_SET(v)       do { PDout(3)  = ((v) != 0U); } while (0)
#define BOARD_UART5_485_SD_RE_DE_SET(v) do { PDout(1) = ((v) != 0U); } while (0)
#define BOARD_UART5_6_TTL_SW_SET(v)    do { PDout(12) = ((v) != 0U); } while (0)

#define BOARD_SOFT_IIC_SCL_SET(v)      do { PBout(8)  = ((v) != 0U); } while (0)
#define BOARD_SOFT_IIC_SDA_SET(v)      do { PBout(9)  = ((v) != 0U); } while (0)
#define BOARD_SOFT_IIC_SDA_GET()       PBin(9)

#define BOARD_LP5012_SCL_SET(v)        do { PBout(6)  = ((v) != 0U); } while (0)
#define BOARD_LP5012_SDA_SET(v)        do { PBout(9)  = ((v) != 0U); } while (0)
#define BOARD_LP5012_SDA_GET()         PBin(9)
#define BOARD_LP5012_STATUS_LED_SET(v) do { PAout(4)  = ((v) != 0U); } while (0)

#define BOARD_LCD_BL_SET(v)            do { PBout(15) = ((v) != 0U); } while (0)
#define BOARD_ETH_RESET_SET(v)         do { PBout(0)  = ((v) != 0U); } while (0)

#endif
