#include "./BSP/EEPROM/eeprom.h"
#include "./BSP/GPIO/gpio.h"
#include "./BSP/I2C/I2C.h"
#include "./BSP/KEY/key.h"
#include "./BSP/LED/led.h"
#include "./BSP/LP5012/LP5012.h"
#include "./BSP/TMP175/TMP175.h"
#include "./BSP/uart_Set/uart_Set.h"
#include "./MALLOC/malloc.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./USMART/usmart.h"
#include "freertos_demo.h"
#include "signal_switch.h"

int main(void)
{
/* Core and clock setup. */
    HAL_Init();
    sys_stm32_clock_init(336, 8, 2, 7);
    delay_init(168);

/* 串口初始化 */
    usart_init(115200);

/* 湿端上电初始化 SONA_POWER_EN 对应IO口为PD14,上电初始化为低，延时500ms后拉高，湿端上电 */
    sona_power_en_init();
    led_init();
    delay_ms(500);
    sona_power_en_set(1);

/* 按键初始化 */
    mcu_key_init();

/* 串口初始化 */
    heading_gpio_init();
    motion_gpio_init();
    gnss_gpio_init();
    pps_gpio_init();
	//SVS SYNC 引脚初始化
	sync_init();
    //SVS 引脚初始化
    SVS_init();

/* LP5012 LED 初始化 */
    LP5012_Init_All();
    lp5012_pps_led_g_init();

/* I2C 初始化 */
    i2c1_init();
	/*tmp175*/
    tmp175_init();
	/*eeprom*/
    bl24c128_test();

/* 交换机芯片初始化 初始化默认低，延时100ms 后拉高，启动交换机 */
    sw_reset_init();
    SW();
    printf("Peripherals Starting...\r\n");

/* SRAM初始化 */
    my_mem_init(SRAMIN);
    freertos_demo();
}
