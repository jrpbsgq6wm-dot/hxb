/*
    STM32F4 LED CODE
    2024/12/24
    HOU XINBO
*/
#ifndef __LED__H__
#define __LED__H__

#include "sys.h"
/*
	case 0:	//红绿交替闪 init
		LED1 = !LED1;
		LED2 = 0;
		break;
	case 1:    //绿灯闪 入水状态
		LED1 = 1;
		LED2 = !LED2;
		break;
	case 2:    //绿灯常量入水稳定，开始记录
		LED1 = !LED1;
		LED2 = 1;
		break;
	case 3:    //红灯 未入水
		LED1 = 0;
		LED2 = 1;
		break;
	case 4: 	//红灯闪
		LED1 = !LED1;
		LED2 = !LED2;
		break;
*/

enum work_len_mde
{
      init_ledmode,
      survey_ledmode,           
      work_ledmode,             
      error_lenmode,             
      null_ledmode,      
};
extern uint8_t ledflag;

//#define LED_R   PAout(1)
//#define LED_G   PAout(2)


#define LED0 PAout(0)       //LED0
#define LED1 PAout(1)   	//LED1
#define LED2 PAout(2)   	//LED2
#define LED3 PAout(3)   	//LED3
#define LED4 PAout(4)   	//LED4
#define LED5 PAout(5)   	//LED5
#define LED6 PAout(6)   	//LED6
#define LED7 PAout(7)   	//LED7

void LED_Init(void);
void led_test(void);
extern void enter_surveymode_led(void);
extern void enter_workmode_led(void);
extern void enter_errormode_led(void);
#endif

