#include "time.h"
#include "led.h"
#include "usart.h"
#include "main.h"
#include "ad7124.h"

TIM_HandleTypeDef TIM_Config_2;  
TIM_HandleTypeDef TIM_Config_3; 
TIM_HandleTypeDef TIM_Config_4; 
TIM_HandleTypeDef TIM_Config_5; 
//通用定时器3中断初始化
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

void TIM3_Init(uint16_t arr,uint16_t psc){
    TIM_Config_3.Instance = TIM3;
    TIM_Config_3.Init.Prescaler = psc-1;
    TIM_Config_3.Init.Period  = arr-1;
    TIM_Config_3.Init.CounterMode = TIM_COUNTERMODE_UP;   //向上计数
    TIM_Config_3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; //时钟分频因子
    HAL_TIM_Base_Init(&TIM_Config_3);     //初始化定时器  
    HAL_TIM_Base_Start_IT(&TIM_Config_3); //使能定时器3更新中断 TIM_IT_UPDATE
}

void TIM2_Init(uint16_t arr,uint16_t psc){
    TIM_Config_2.Instance = TIM2;
    TIM_Config_2.Init.Prescaler = psc-1;
    TIM_Config_2.Init.Period  = arr-1;
    TIM_Config_2.Init.CounterMode = TIM_COUNTERMODE_UP;   //向上计数
    TIM_Config_2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; //时钟分频因子
    HAL_TIM_Base_Init(&TIM_Config_2);     //初始化定时器  
    HAL_TIM_Base_Start_IT(&TIM_Config_2); //使能定时器2更新中断 TIM_IT_UPDATE
}

void TIM4_Init(uint16_t arr,uint16_t psc){
    TIM_Config_4.Instance = TIM4;
    TIM_Config_4.Init.Prescaler = psc-1;
    TIM_Config_4.Init.Period  = arr-1;
    TIM_Config_4.Init.CounterMode = TIM_COUNTERMODE_UP;   //向上计数
    TIM_Config_4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; //时钟分频因子
    HAL_TIM_Base_Init(&TIM_Config_4);     //初始化定时器  
    HAL_TIM_Base_Stop_IT(&TIM_Config_4);            //默认自容模式
//    HAL_TIM_Base_Start_IT(&TIM_Config_4); //使能定时器2更新中断 TIM_IT_UPDATE
}

void TIM5_Init(uint16_t arr,uint16_t psc){
    TIM_Config_5.Instance = TIM5;
    TIM_Config_5.Init.Prescaler = psc-1;
    TIM_Config_5.Init.Period  = arr-1;
    TIM_Config_5.Init.CounterMode = TIM_COUNTERMODE_UP;   //向上计数
    TIM_Config_5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; //时钟分频因子
    HAL_TIM_Base_Init(&TIM_Config_5);     //初始化定时器  
    HAL_TIM_Base_Start_IT(&TIM_Config_5); //使能定时器3更新中断 TIM_IT_UPDATE
}

void set_work_mode(void){
    
}

    

void timework_init(void){
    /*判断工作模式*/
    if(svp_cmd.WORK_MODE_FLAG == 0x00){ //自容模式
        /*关闭TIM4*/
        HAL_TIM_Base_Stop_IT(&TIM_Config_4);
        /*开启定时器5*/
         TIM5_Init(500,CLOCK_PSC);
        if(svp_cmd.WORK_SET_MODE_FLAG == 0x00){ //自容模式 - 压力
            //printf("这是自容 - 压力模式\r\n");
            //构造压力阈值
            float buf;
            memcpy(&buf,&svp_cmd.WORK_VALUE,4);
            pri_file.auto_wmode = 1;    //将自容模式标志位置为压力变化
            pri_file.pri_auto_pa_value = buf;   //设置压力阈值
            if(pri_file.pri_auto_pa_value == 0.0f){ // 设置压力阈值默认值
                pri_file.pri_auto_pa_value = 0.05f; //默认0.05米
            }
        }else if(svp_cmd.WORK_SET_MODE_FLAG == 0x01){//自容模式 - 频率
           // printf("这是自容 - 频率模式\r\n");
            pri_file.auto_wmode = 0;    //将自容模式标志位置为频率变化
            /*设置频率计数位*/
            switch(svp_cmd.WORK_VALUE){
                case 100:
                case 50:
                case 20:
                    pri_file.pri_auto_bound = svp_cmd.WORK_VALUE;
                    break;
                default:
                    pri_file.pri_auto_bound = 100; //默认100ms
                    break;
            }
        }else{  //自容模式 - def频率
            pri_file.auto_wmode = 0;    //将自容模式标志位置为频率变化
            /*设置频率计数位*/
            switch(svp_cmd.WORK_VALUE){
                case 100:
                case 50:
                case 20:
                    pri_file.pri_auto_bound = svp_cmd.WORK_VALUE;
                    break;
                default:
                    pri_file.pri_auto_bound = 100; //默认100ms
                    break;
            }
        }
    }else if(svp_cmd.WORK_MODE_FLAG == 0x01){ //直读模式
        /*关闭TIM5*/
        HAL_TIM_Base_Stop_IT(&TIM_Config_5);
        pri_file.start_flag = 0;
        pri_file.updata_filename_flag = 0;
        //printf("%d\r\n",svp_cmd.WORK_VALUE);
        /*开启定时器4*/
        if(svp_cmd.WORK_SET_MODE_FLAG == 0xFF){ //标志位正确
            switch(svp_cmd.WORK_VALUE){
                case 100:
                case 50:
                case 20:
                    TIM4_Init((svp_cmd.WORK_VALUE*10),CLOCK_PSC);         //打印 100ms
//                    //重置cnt
//                    __HAL_TIM_SET_AUTORELOAD(&TIM_Config_4,svp_cmd.WORK_VALUE*10);          //打印输出声速 温度 压力   -50ms
//                    __HAL_TIM_SET_COUNTER(&TIM_Config_4,0);
//                    HAL_TIM_GenerateEvent(&TIM_Config_4,TIM_EVENTSOURCE_UPDATE);
                    break;
                default:
                    TIM4_Init(1000,CLOCK_PSC);         //打印 100ms
                    break;
            }
        }else{  //标志位错误 直读模式 默认频率 100ms
            TIM4_Init((100*10),CLOCK_PSC);         //打印 100ms
        }
        HAL_TIM_Base_Start_IT(&TIM_Config_4);
    }else{  //错误 - 默认自容模式 频率输出
        /*关闭TIM5*/
        HAL_TIM_Base_Stop_IT(&TIM_Config_5);
        /*开启定时器4*/
        TIM4_Init(1000,CLOCK_PSC);         //打印 100ms
    }
}

//开启时钟 设置中断优先级  HAL_TIM_Base_Init回调函数
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim){
    if(htim->Instance == TIM3){
        __HAL_RCC_TIM3_CLK_ENABLE();    //定时器3时钟使能
        HAL_NVIC_SetPriority(TIM3_IRQn,1,0);
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
        
    }else if(htim->Instance == TIM2){   //TDC
        __HAL_RCC_TIM2_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM2_IRQn,1,0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
        
    }else if(htim->Instance == TIM4){   //  pa | temp
        __HAL_RCC_TIM4_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM4_IRQn,1,0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
        
    }else if(htim->Instance == TIM5){   //
        __HAL_RCC_TIM5_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM5_IRQn,1,0);
        HAL_NVIC_EnableIRQ(TIM5_IRQn);
        
    }
}


//定时器 3 中断服务函数
void TIM3_IRQHandler(void){
    HAL_TIM_IRQHandler(&TIM_Config_3);
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
    if(htim == (&TIM_Config_3)){
        LED2 = !LED2;
        /*获取当前RTC时间*/
        DS3231_Read_All();
        DS3231_Read_Time();
        /*电池*/
        read_power_data();
        //printf("20%d_%d_%d__%d_%d_%d\r\n",DS3231_Time.year,DS3231_Time.mon,DS3231_Time.date,DS3231_Time.hour,DS3231_Time.min,DS3231_Time.sec);
        //printf("%d ---- voltage%f,charge%f,current%f,temperature%f\r\n",read_power_data(),voltage,charge,current,temperature);
    }else if(htim == (&TIM_Config_2)){              //20ms
        //TDC
        kalman_sound_velocity_posprocessor();
        //ADC
        Filter_func();
    }else if(htim == (&TIM_Config_4)){              //100ms
            printf("S:%.3f,F:%.3f,K:%.3f,P:%.3f,A:%.3f,B:%.3f,I:%.2f%%,FV:%.3f,TV:%.3f,T:%fus-TEMP:%f℃-PA:%fBar %fV %fmA %f℃(B)\r\n",
				True_sv,
				Pretend_sv,
				kalman_sv,
				pri_sv,
				smooth.current_coe_a,
				smooth.current_coe_b,
				PW1STvalue,
				tdc_pse.F_value,
				tdc_pse.T_value,
				echo_times,
                nowData.temperature,
                nowData.pressure,
                voltage,
                current,
                temperature);
    }else if(htim == (&TIM_Config_5)){ 
        //printf("pri_file.start_flag %d\r\n",pri_file.start_flag);  
        //文件写入
        pri_data_to_file();
    }
}


