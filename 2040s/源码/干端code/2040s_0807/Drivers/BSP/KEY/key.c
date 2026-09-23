#include "./BSP/KEY/key.h"
#include <stdio.h>

/* Debounce interval in milliseconds. */
#define MCU_KEY_DEBOUNCE_MS 10

static volatile uint8_t g_key_pending = 0;
static volatile uint8_t g_key_event = 0;
static volatile uint32_t g_key_tick = 0;

/* Configure the MCU key on PC0 as a falling-edge EXTI input. */
void mcu_key_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    MCU_KEY_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin = MCU_KEY_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_IT_FALLING;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MCU_KEY_GPIO_PORT, &gpio_init_struct);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/* EXTI0 interrupt handler for the key. */
void EXTI0_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(MCU_KEY_GPIO_PIN);
    EXTI->IMR &= ~(MCU_KEY_GPIO_PIN);

    g_key_tick = HAL_GetTick();
    g_key_pending = 1;
}

/* Call this from the 1 ms SysTick hook to finish debounce handling. */
void mcu_key_tick_handler(void)
{
    if (g_key_pending == 0)
    {
        return;
    }

    if ((HAL_GetTick() - g_key_tick) < MCU_KEY_DEBOUNCE_MS)
    {
        return;
    }

    g_key_pending = 0;

    if (MCU_KEY_READ() == 0)
    {
        g_key_event = 1;
    }
    else
    {
        EXTI->IMR |= MCU_KEY_GPIO_PIN;
    }
}

/* Poll this from the main loop to consume a debounced key event. */
void mcu_key_scan(void)
{
    if (g_key_event == 0)
    {
        return;
    }

    g_key_event = 0;
    EXTI->IMR |= MCU_KEY_GPIO_PIN;

    printf("mcu_key input\r\n");
}
