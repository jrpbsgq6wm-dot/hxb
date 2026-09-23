   /**
 ****************************************************************************************************
 * @file        main.c
 * @brief       lwIP TCP/IP + I2C        (TMP175, EEPROM, LP5012)             
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./USMART/usmart.h"
#include "./BSP/LED/led.h"
#include "./BSP/GPIO/gpio.h"
#include "./BSP/KEY/key.h"
#include "./BSP/I2C/I2C.h"
#include "./BSP/TMP175/TMP175.h"
#include "./BSP/EEPROM/eeprom.h"
#include "./BSP/uart_Set/uart_Set.h"
#include "./BSP/LP5012/LP5012.h"
#include "./MALLOC/malloc.h"
#include "freertos_demo.h"


int main(void)
{
    HAL_Init();                            
    sys_stm32_clock_init(336, 8, 2, 7);      /*168Mhz*/
    delay_init(168);                            
    usart_init(115200);                      /*115200*/
    usmart_dev.init(84);                     /*USMART */
    sona_power_en_init();                    /* SONA_POWER_EN(PD14), low by default */
    led_init();                              /*       LED1(PC8) LED2(PD15) LED3(PD10) */
    delay_ms(500);                           /* 500ms delay */
    sona_power_en_set(1);                    /* pull high after 500ms */

    /* ==========                 ========== */
    mcu_key_init();                          /*MCU_KEY(PC0)*/
    uart_switch_init();                      /*UART*/
    uart_switch_apply_config();              /*UART_STAR*/

    /* ========== LP5012 LED            +          ========== */
    LP5012_Init_All();
    // LP5012_PowerOn_SelfTest();   /* LED self-test disabled */
    lp5012_pps_led_g_init();
    /* U1/U2 LED colors are cycled by app_task (red->green->blue, 2s each, 30%) */

    i2c1_init();                                /*           I2C1        */
    tmp175_init();                              /*           TMP175                 */
    bl24c128_test();                            /* EEPROM              */
    /* PE2 SW_RESET:         100ms         */
    sw_reset_init();
    SW();

    printf("Peripherals Starting...\r\n");

    
    (SRAMIN);                        /*           SRAM      */

    freertos_demo();                            /*     FreeRTOS + lwIP         */
}
