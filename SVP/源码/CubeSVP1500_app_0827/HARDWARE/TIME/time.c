#include "time.h"
#include "led.h"
#include "usart.h"
#include "main.h"
#include "ad7124.h"
#include "myiic.h"
#include "svp_sensor.h"

uint8_t pri_workmode_flag = 0;
//电池上报超时时间变量
uint32_t battery_level_report_start_time;
uint32_t battery_level_report_timeout_ms;

cal_t cal;
TIM_HandleTypeDef TIM_Config_2;  
TIM_HandleTypeDef TIM_Config_4; 
TIM_HandleTypeDef TIM_Config_5; 
volatile uint8_t file_task_pending = 0;
/* TIM4 中断到达时置位，由 FreeRTOS 命令任务完成实际的直读数据输出。 */
volatile uint8_t telemetry_task_pending = 0;
//通用定时器初始化
//arr : 自动重装值
//psc : 时钟预分频
//定时器溢出时间计算方法： Tout = ((arr+1)*(psc+1))/Ft us
//Ft : 定时器工作频率，单位:MHz

/*
__HAL_TIM_ENABLE_IT(htim, TIM_IT_UPDATE);//使能句柄制定的定时器更新中断
__HAL_TIM_DISABLE_IT(htim, TIM_IT_UPDATE);//关闭句柄指定的定时器更新中断
__HAL_TIM_ENABLE(htim);//使能句柄 htim 指定的定时器
__HAL_TIM_DISABLE(htim);//关闭句柄 htim 指定的定时器
*/

void TIM2_Init(uint16_t arr,uint16_t psc){
    TIM_Config_2.Instance = TIM2;
    TIM_Config_2.Init.Prescaler = psc-1;
    TIM_Config_2.Init.Period  = arr-1;
    TIM_Config_2.Init.CounterMode = TIM_COUNTERMODE_UP;   //向上计数
    TIM_Config_2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; //时钟分频因子
    HAL_TIM_Base_Init(&TIM_Config_2);     //初始化定时器  
    HAL_TIM_Base_Stop_IT(&TIM_Config_2); //使能定时器2更新中断 TIM_IT_UPDATE
}

void TIM4_Init(uint16_t arr,uint16_t psc){
    TIM_Config_4.Instance = TIM4;
    TIM_Config_4.Init.Prescaler = psc-1;
    TIM_Config_4.Init.Period  = arr-1;
    TIM_Config_4.Init.CounterMode = TIM_COUNTERMODE_UP;   //向上计数
    TIM_Config_4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; //时钟分频因子
    HAL_TIM_Base_Init(&TIM_Config_4);     //初始化定时器  
    HAL_TIM_Base_Stop_IT(&TIM_Config_4);            //默认自容模式
}

void TIM5_Init(uint16_t arr,uint16_t psc){
    TIM_Config_5.Instance = TIM5;
    TIM_Config_5.Init.Prescaler = psc-1;
    TIM_Config_5.Init.Period  = arr-1;
    TIM_Config_5.Init.CounterMode = TIM_COUNTERMODE_UP;   //向上计数
    TIM_Config_5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; //时钟分频因子
    HAL_TIM_Base_Init(&TIM_Config_5);     //初始化定时器  
    HAL_TIM_Base_Stop_IT(&TIM_Config_5); 
}


//开启时钟 设置中断优先级  HAL_TIM_Base_Init回调函数
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim){
    if(htim->Instance == TIM2){   //TDC
        __HAL_RCC_TIM2_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM2_IRQn,1,0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
    }else if(htim->Instance == TIM4){   //  pa | temp
        __HAL_RCC_TIM4_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM4_IRQn,1,0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    }else if(htim->Instance == TIM5){   //
        __HAL_RCC_TIM5_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM5_IRQn,0,0);
        HAL_NVIC_EnableIRQ(TIM5_IRQn);  
    }
}


//定时器 2 中断服务函数
void TIM2_IRQHandler(void){
    HAL_TIM_IRQHandler(&TIM_Config_2);
}

//定时器 4 中断服务函数
void TIM4_IRQHandler(void){
    HAL_TIM_IRQHandler(&TIM_Config_4);
}

//定时器 5 中断服务函数
void TIM5_IRQHandler(void){
	HAL_TIM_IRQHandler(&TIM_Config_5);
}


//定时器中断处理函数 HAL_TIM_IRQHandler回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    if(htim == (&TIM_Config_2)){              //50ms
        //声速
        kalman_sound_velocity_posprocessor();
        /*
         * 由统一传感器适配层根据上电识别结果执行采集：
         * - 新版：AD7124 温度 + IIC 压力；
         * - 旧版：AD7124 温度、参考电压、压力三通道轮询。
         *
         * TDC 有效采集仍然由原有 INTSign 外部中断逻辑判定，
         * 此处不改变 TDC 的中断采集规则。
         */
        SVP_Sensor_Update();
    }else if(htim == (&TIM_Config_4)){
        /*
         * 中断只生成直读输出事件。浮点格式化和 printf 由 FreeRTOS 任务处理，
         * 避免在中断中阻塞串口或延长 TIM2/TIM5 的响应延迟。
         */
        telemetry_task_pending = 1;
    }else if(htim == (&TIM_Config_5)){
		file_task_pending = 1;
	}
}

// 更新当前时间
uint32_t get_current_tick(void) {
    return HAL_GetTick();
}

// 检查是否超时
int is_timeout(uint32_t start_tick, uint32_t timeout_ms) {
    return (get_current_tick() - start_tick) >= timeout_ms;
}

