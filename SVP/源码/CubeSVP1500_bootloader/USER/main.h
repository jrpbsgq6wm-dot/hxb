/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "time.h"
#include <string.h>

#include "malloc.h"	 
#include "eeprom.h"
#include "stmflash.h"
#include "iic.h"

/* DEFINE --------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
    /*FATFS相关*/

extern uint8_t ymodem_flag;
    /*TDC*/
extern volatile uint8_t INTSign;
extern volatile uint8_t  num;  //记录中断次数
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
