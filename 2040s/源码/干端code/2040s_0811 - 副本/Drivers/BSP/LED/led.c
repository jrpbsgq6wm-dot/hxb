#include "./BSP/LED/led.h"

/* LED state variables */
uint8_t LED1 = 0;
uint8_t LED2 = 0;
uint8_t LED3 = 0;

/* Initialize the three board LEDs as push-pull outputs. */
void led_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    /* LED1 on PC8 */
    LED1_GPIO_CLK_ENABLE();
    gpio_init_struct.Pin = LED1_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LED1_GPIO_PORT, &gpio_init_struct);
    BOARD_LED1_SET(0);
    LED1 = 0;

    /* LED2 on PD15 */
    LED2_GPIO_CLK_ENABLE();
    gpio_init_struct.Pin = LED2_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LED2_GPIO_PORT, &gpio_init_struct);
    BOARD_LED2_SET(0);
    LED2 = 0;

    /* LED3 on PD10 */
    LED3_GPIO_CLK_ENABLE();
    gpio_init_struct.Pin = LED3_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LED3_GPIO_PORT, &gpio_init_struct);
    BOARD_LED3_SET(0);
    LED3 = 0;
}

/* Apply the cached LED state to the GPIO pins. */
void led_sync(void)
{
    BOARD_LED1_SET(LED1);
    BOARD_LED2_SET(LED2);
    BOARD_LED3_SET(LED3);
}
