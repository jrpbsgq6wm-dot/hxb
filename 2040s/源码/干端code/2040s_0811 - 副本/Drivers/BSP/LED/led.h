#ifndef __LED_H
#define __LED_H

#include "./SYSTEM/sys/sys.h"

/* Board LEDs: LED1=PC8, LED2=PD15, LED3=PD10 */
#define LED1_GPIO_PORT GPIOC
#define LED1_GPIO_PIN GPIO_PIN_8
#define LED1_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)

#define LED2_GPIO_PORT GPIOD
#define LED2_GPIO_PIN GPIO_PIN_15
#define LED2_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

#define LED3_GPIO_PORT GPIOD
#define LED3_GPIO_PIN GPIO_PIN_10
#define LED3_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

extern uint8_t LED1;
extern uint8_t LED2;
extern uint8_t LED3;

void led_init(void);
void led_sync(void);

#endif
