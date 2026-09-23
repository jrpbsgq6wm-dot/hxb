#ifndef _KEY_H
#define _KEY_H
#include "sys.h"


//下面的方式是通过位带操作方式读取IO
//#define KEY0   PCin(8)   	
//#define KEY1   PCin(9)
//#define KEY2   PDin(2)	 
//#define WK_UP  PAin(0)	


//下面的方式是通过直接操作HAL库函数方式读取IO
#define KEY0        HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_8)  //KEY0按键PC8
#define KEY1        HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_9)  //KEY1按键PC9
#define KEY2        HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_2)  //KEY2按键PD2
#define WK_UP       HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)  //WKUP按键PA0

#define KEY0_PRES	1	//KEY0  
#define KEY1_PRES	2	//KEY1 
#define KEY2_PRES	3	//KEY2
#define WKUP_PRES	4	//WK_UP 

void KEY_Init(void);
u8 KEY_Scan(u8 mode);
#endif
