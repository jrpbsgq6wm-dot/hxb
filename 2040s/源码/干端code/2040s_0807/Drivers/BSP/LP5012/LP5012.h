   #ifndef __LP5012_H
#define __LP5012_H

#include "./SYSTEM/sys/sys.h"

/* 8-bit I2C address */
#define LP5012_U1_ADDR      0x28U
#define LP5012_U2_ADDR      0x2CU

/* Register addresses */
#define REG_CONFIG0         0x00
#define REG_BANK_BRIGHT     0x03
#define REG_LED_BRIGHT      0x07  /* LED0 brightness */
#define REG_OUT_START       0x0D  /* OUT2 color start */

/******************************************************************************/
/* U1 (0x28): 4 RGB LEDs -- reg 0x0B-0x16 */
/*                           OUT  LED   Color  Name            */
#define LP5012_U1_D2_R     0x0B   /* OUT0 D2 Red   SONA_ETH_RED    */
#define LP5012_U1_D2_G     0x0C   /* OUT1 D2 Green SONA_ETH_GREEN  */
#define LP5012_U1_D2_B     0x0D   /* OUT2 D2 Blue  SONA_ETH_BLUE   */
#define LP5012_U1_D3_R     0x0E   /* OUT3 D3 Red   PPS_SATE_RED    */
#define LP5012_U1_D3_G     0x0F   /* OUT4 D3 Green PPS_SATE_GREEN  */
#define LP5012_U1_D3_B     0x10   /* OUT5 D3 Blue  PPS_SATE_BLUE   */
#define LP5012_U1_D4_R     0x11   /* OUT6 D4 Red   MOTION_SATE_RED   */
#define LP5012_U1_D4_G     0x12   /* OUT7 D4 Green MOTION_SATE_GREEN */
#define LP5012_U1_D4_B     0x13   /* OUT8 D4 Blue  MOTION_SATE_BLUE  */
#define LP5012_U1_D5_R     0x14   /* OUT9 D5 Red   HEADING_SATE_RED  */
#define LP5012_U1_D5_G     0x15   /* OUT10 D5 Green HEADING_SATE_GREEN*/
#define LP5012_U1_D5_B     0x16   /* OUT11 D5 Blue  HEADING_SATE_BLUE */

/* U2 (0x2C): 3 RGB LEDs -- reg 0x0B-0x16 */
/*                           OUT  LED   Color  Name            */
#define LP5012_U2_D6_R     0x0E   /* OUT3 D6 Red   SVP_SATE_RED    */
#define LP5012_U2_D6_G     0x0F   /* OUT4 D6 Green SVP_SATE_GREEN  */
#define LP5012_U2_D6_B     0x10   /* OUT5 D6 Blue  SVP_SATE_BLUE   */
#define LP5012_U2_D7_R     0x11   /* OUT6 D7 Red   GNSS_SATE_RED   */
#define LP5012_U2_D7_G     0x12   /* OUT7 D7 Green GNSS_SATE_GREEN */
#define LP5012_U2_D7_B     0x13   /* OUT8 D7 Blue  GNSS_SATE_BLUE  */
#define LP5012_U2_D8_R     0x14   /* OUT9 D8 Red   SYNC_SATE_RED   */
#define LP5012_U2_D8_G     0x15   /* OUT10 D8 Green SYNC_SATE_GREEN*/
#define LP5012_U2_D8_B     0x16   /* OUT11 D8 Blue  SYNC_SATE_BLUE */

/******************************************************************************/

/* Functions */
void LP5012_Write_Reg(uint8_t addr, uint8_t reg, uint8_t data);
void LP5012_Init_All(void);
void LP5012_PowerOn_SelfTest(void);
void lp5012_pps_led_g_init(void);

/* Color set: RGB 0x00=off ~ 0xFF=max */
void LP5012_U1_Set_D2(uint8_t r, uint8_t g, uint8_t b);
void LP5012_U1_Set_D3(uint8_t r, uint8_t g, uint8_t b);
void LP5012_U1_Set_D4(uint8_t r, uint8_t g, uint8_t b);
void LP5012_U1_Set_D5(uint8_t r, uint8_t g, uint8_t b);
void LP5012_U2_Set_D6(uint8_t r, uint8_t g, uint8_t b);
void LP5012_U2_Set_D7(uint8_t r, uint8_t g, uint8_t b);
void LP5012_U2_Set_D8(uint8_t r, uint8_t g, uint8_t b);

/* HAL I2C1 based writes (usable after i2c1_init, e.g. from FreeRTOS tasks) */
void LP5012_HAL_Write_Reg(uint8_t addr, uint8_t reg, uint8_t data);
void LP5012_HAL_SetAll(uint8_t chip, uint8_t r, uint8_t g, uint8_t b);

#endif
