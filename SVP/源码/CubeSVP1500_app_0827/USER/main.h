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

#include "w25qxx.h"
#include "fat.h"
#include "malloc.h"	 
#include "eeprom.h"
#include "stmflash.h"
#include "exti.h"
#include "tdc_gp22.h"
#include "iic.h"
#include "rtc.h"
#include "communication.h"
#include "power_app.h"
#include "ad7124.h"
#include "myiic.h"
#include "svp_sensor.h"


#include "w25qxx.h"



/* DEFINE --------------------------------------------------------------------*/
#define FW_APP      "20260914"         //软件版本号
                          
/* Exported types ------------------------------------------------------------*/

    /*TDC*/
extern volatile uint8_t INTSign;
extern volatile uint8_t  num;  //记录中断次数
extern uint16_t ssssss;
extern uint8_t buf_num;
extern float Depth;
extern volatile float PT100_TEMP;
extern volatile float PA;
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
