#ifndef __KEY_H
#define __KEY_H

#include "./SYSTEM/sys/sys.h"

/* MCU key on PC0 */
#define MCU_KEY_GPIO_PORT GPIOC
#define MCU_KEY_GPIO_PIN GPIO_PIN_0
#define MCU_KEY_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)
#define MCU_KEY_READ() BOARD_MCU_KEY_GET()

void mcu_key_init(void);
void mcu_key_scan(void);
void mcu_key_tick_handler(void);

#endif
