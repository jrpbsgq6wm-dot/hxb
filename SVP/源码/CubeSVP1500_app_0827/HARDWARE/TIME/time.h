#ifndef __TIME__H__
#define __TIME__H__

#include "sys.h"
#include "rtc.h"
#include "fat.h"


#define CLOCK_PSC   9600


/* TIM2：TDC 采集；TIM4：直读输出；TIM5：文件任务节拍。 */
extern TIM_HandleTypeDef TIM_Config_2;
extern TIM_HandleTypeDef TIM_Config_4;
extern TIM_HandleTypeDef TIM_Config_5;
/* 显控连接标志，只在本次上电期间有效，不写入 EEPROM。 */
extern volatile uint8_t workmode_flag;
extern volatile uint8_t file_task_pending;
/* TIM4 的直读输出事件标志，仅由 TIM4 中断置位、由 FreeRTOS 命令任务清除。 */
extern volatile uint8_t telemetry_task_pending;
typedef struct{
    uint8_t cal_t_flag;
    uint32_t cal_time;
}cal_t;
extern cal_t cal;
extern uint8_t pri_workmode_flag;

void TIM2_Init(uint16_t arr,uint16_t psc);
void TIM4_Init(uint16_t arr,uint16_t psc);
void TIM5_Init(uint16_t arr,uint16_t psc);
extern void timework_init(void);
extern uint8_t work_status(void);
extern uint32_t get_current_tick(void);
extern int is_timeout(uint32_t start_tick, uint32_t timeout_ms);
extern void battery_level_report(void);
extern void battery_level_report_reset(void);
#endif
