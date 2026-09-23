#include "freertos_app.h"

#include "task.h"
#include "semphr.h"
#include "main.h"
#include <math.h>
#include <stdio.h>

/*
 * 任务分工：
 * - SVP_FILE 独占文件状态机和 FatFs/W25Qxx 访问。
 * - SVP_CMD 处理串口指令，并把文件任务事件转发给 SVP_FILE。
 * - SVP_STATUS 周期刷新 RTC、电池信息和工作状态灯。
 *
 * SVP_FILE 使用较高优先级，保证已有的文件写入请求能够及时处理。
 */
#define SVP_COMMAND_TASK_STACK_WORDS    1536U
#define SVP_FILE_TASK_STACK_WORDS       1536U
#define SVP_STATUS_TASK_STACK_WORDS      512U
#define SVP_COMMAND_TASK_PRIORITY       ( tskIDLE_PRIORITY + 1U )
#define SVP_FILE_TASK_PRIORITY          ( tskIDLE_PRIORITY + 2U )
#define SVP_STATUS_TASK_PRIORITY        ( tskIDLE_PRIORITY + 1U )

/*
 * 文件任务句柄在调度器启动前创建，之后仅由命令任务使用。
 * 任务通知计数会合并多个连续的写文件请求，避免重复堆积事件。
 */
static TaskHandle_t svp_file_task_handle;

/*
 * I2C1 同时连接 DS3231、LTC2943 和 AT24C02。
 * HAL 的一个 I2C 句柄不能被多个任务并发使用，因此用互斥锁保护完整事务。
 */
static SemaphoreHandle_t svp_i2c1_mutex;

/*
 * TIM4 只负责给直读模式提供可变周期的节拍。
 * 浮点格式化和串口输出放在任务中执行，避免中断长期占用 CPU。
 */
static void svp_telemetry_output(void)
{
    char telemetry_buffer[256];
    int telemetry_length;

    if (pri_workmode_flag == 0) {
        telemetry_length = snprintf(telemetry_buffer,
                                    sizeof(telemetry_buffer),
                                    " %8.3f,%8.3f,%8.3f,%8.1f%%\r\n",
                                    pri_sv,
                                    Depth,
                                    PT100_TEMP,
                                    SOC);
    } else {
        /*
         * "\xA1\xE6" 是原工程使用的摄氏度符号字节序列。
         * 使用转义可避免 ARMCC 的多字节字符警告，串口输出保持原有格式。
         */
        telemetry_length = snprintf(telemetry_buffer,
                                    sizeof(telemetry_buffer),
                                    ">S:%.3f,F:%.3f,K:%.3f,P:%.3f,A:%.3f,B:%.3f,I:%.2f%%,FV:%.3f,TV:%.3f,T:%.6fus,TEMP:%.3f\xA1\xE6,Depth:%.4fm %.3fmAh %.3fA %.3fV %.1f\xA1\xE6 %2.1fh,%.1f%%\r\n",
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
                                    PT100_TEMP,
                                    Depth,
                                    charge,
                                    current,
                                    voltage,
                                    temperature,
                                    fabs(charge / (current * 1000)),
                                    SOC);
    }

    /*
     * 直接发送整行缓冲区，避免逐字符调用 printf/fputc，
     * 使上位机更容易一次收到完整数据行。
     */
    if ((telemetry_length > 0) &&
        ((size_t)telemetry_length < sizeof(telemetry_buffer))) {
        (void)Send_data((uint8_t *)telemetry_buffer, (uint16_t)telemetry_length);
    }
}

BaseType_t freertos_i2c1_lock(TickType_t timeout)
{
    /*
     * 系统初始化阶段尚未创建互斥锁，也未启动调度器。
     * 此时只有主流程运行，不存在任务竞争，因此直接放行。
     */
    if ((svp_i2c1_mutex == NULL) ||
        (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)) {
        return pdPASS;
    }

    return xSemaphoreTake(svp_i2c1_mutex, timeout);
}

void freertos_i2c1_unlock(void)
{
    /*
     * 初始化阶段没有真正获取锁，不能执行释放操作。
     * 本函数只允许在任务上下文中调用，不能从中断服务函数调用。
     */
    if ((svp_i2c1_mutex != NULL) &&
        (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)) {
        xSemaphoreGive(svp_i2c1_mutex);
    }
}

/**
 * @brief  文件任务
 * @param  argument: Not used
 * @retval None
 */
static void svp_file_task(void *argument)
{
    (void)argument;

    for (;;) {
        /*
         * 在此阻塞且不占用 CPU，直到 SVP_CMD 发现 file_task_pending 标志。
         * pdTRUE 会清除当前通知计数，每次唤醒执行一次文件状态机步骤。
         */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /*
         * 被唤醒后再次确认工作模式，防止通知和任务获得运行机会之间发生模式切换。
         */
        if ((svp_cmd.WORK_MODE_FLAG == 0x00) &&
            (workmode_flag == 0U)) {
            /* 文件状态机在创建文件和写入记录时会读取 RTC。 */
            if (freertos_i2c1_lock(portMAX_DELAY) == pdPASS) {
                pri_process_record_requests();
                pri_data_to_file();
                freertos_i2c1_unlock();
            }
        }
    }
}

/*
 * @brief  串口命令处理任务
 * @param  argument: Not used
 * @retval None
 */
static void svp_command_task(void *argument)
{
    (void)argument;

    for (;;) {
        /*
         * 串口命令可能设置 RTC、读写 EEPROM 或执行电池校准。
         * 获取并立即释放互斥锁可确保命令访问 I2C1 时不会与其他任务重叠。
         */
        if (freertos_i2c1_lock(portMAX_DELAY) == pdPASS) {
            cmd_ProcessData();
            freertos_i2c1_unlock();
        }

        /*
         * TIM4 的 ISR 只置位 telemetry_task_pending。
         * 这里与命令解析处于同一任务上下文，直读输出和协议回复不会并发操作 USART1。
         */
        if ((TIM_Config_4.Instance == TIM4) &&
            ((TIM_Config_4.Instance->CR1 & TIM_CR1_CEN) != 0U)) {
            if ((!pri_enable_flag) && telemetry_task_pending) {
                /*
                 * 该标志是“至少有一次周期到达”的合并事件。
                 * 如果任务来不及处理多个连续节拍，只输出最新一行即可。
                 */
                telemetry_task_pending = 0;
                svp_telemetry_output();
            }
        } else {
            /*
             * 直读模式停止后丢弃已经到达但尚未处理的旧节拍，
             * 避免 DISABLE_PRI 回复之后额外输出一行数据。
             */
            telemetry_task_pending = 0;
        }

        /*
         * TIM5 中断只置位 volatile 标志，不直接调用任务通知 API。
         * 这里在任务上下文中完成安全的事件转交。
         */
        if (workmode_flag != 0U) {
            /*
             * 显控连接后，丢弃尚未处理的文件任务通知，防止连接后重新写文件。
             */
            file_task_pending = 0;
        } else if ((svp_cmd.WORK_MODE_FLAG == 0x00) && file_task_pending) {
            file_task_pending = 0;
            xTaskNotifyGive(svp_file_task_handle);
        }

        /* 保持串口命令响应速度，同时避免前台循环空转。 */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief  状态任务
 * @param  argument: 未使用
 * @retval None
 */
static void svp_status_task(void *argument)
{
    TickType_t last_wake_time;

    (void)argument;
    last_wake_time = xTaskGetTickCount();

    for (;;) {
        /*
         * RTC 和 LTC2943 共用 I2C1。
         * 这里保护 RTC 与电池读取的完整过程，防止其他任务同时使用 HAL I2C 句柄。
         */
        if (freertos_i2c1_lock(portMAX_DELAY) == pdPASS) {
            DS3231_Read_All();
            DS3231_Read_Time();
            read_power_data();
            battery_level_report();
            freertos_i2c1_unlock();
        }

        switch (ledflag) {
            case 0:
                LED1 = !LED1;
                LED2 = 0;
                break;
            case 1:
                LED1 = 1;
                LED2 = !LED2;
                break;
            case 2:
                LED1 = 1;
                LED2 = 0;
                break;
            case 3:
                LED1 = 0;
                LED2 = 1;
                break;
            case 4:
                LED1 = !LED1;
                LED2 = !LED2;
                break;
            default:
                break;
        }

        /*
         * 使用绝对节拍保持 500 ms 周期，单次 I2C 访问时间轻微变化时不会让
         * 状态刷新周期不断累积漂移。
         */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(500));
    }
}

/*
 * @brief  创建 FreeRTOS 任务和 I2C 互斥锁
 * @param  None
 * @retval pdPASS：成功创建所有任务和互斥锁
 *         pdFAIL：创建失败
 */
BaseType_t freertos_app_create(void)
{
    /* HAL 使用抢占式优先级，优先级分组 4 满足 FreeRTOS 的要求。 */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    /* 先创建 I2C1 互斥锁，再创建访问 RTC、电池和 EEPROM 的任务。 */
    svp_i2c1_mutex = xSemaphoreCreateMutex();
    if (svp_i2c1_mutex == NULL) {
        return pdFAIL;
    }

    /* 创建状态任务。 */
    if (xTaskCreate(svp_status_task,
                    "SVP_STAT",
                    SVP_STATUS_TASK_STACK_WORDS,
                    NULL,
                    SVP_STATUS_TASK_PRIORITY,
                    NULL) != pdPASS) {
        return pdFAIL;
    }

    /* 创建文件任务 */
    if (xTaskCreate(svp_file_task,
                    "SVP_FILE",
                    SVP_FILE_TASK_STACK_WORDS,
                    NULL,
                    SVP_FILE_TASK_PRIORITY,
                    &svp_file_task_handle) != pdPASS) {
        return pdFAIL;
    }

    return xTaskCreate(svp_command_task,
                       "SVP_CMD",
                       SVP_COMMAND_TASK_STACK_WORDS,
                       NULL,
                       SVP_COMMAND_TASK_PRIORITY,
                       NULL);
}

void vApplicationMallocFailedHook(void)
{
    /* 任务、控制块或互斥锁分配失败后，调度器无法保证正常工作。 */
    taskDISABLE_INTERRUPTS();

    for (;;) {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;

    /* 保留故障现场供调试器检查，不执行自动复位。 */
    taskDISABLE_INTERRUPTS();

    for (;;) {
    }
}

void vApplicationAssertFailed(const char *file, int line)
{
    (void)file;
    (void)line;

    /* FreeRTOS 配置或 API 使用违反 configASSERT 条件时会进入这里。 */
    taskDISABLE_INTERRUPTS();

    for (;;) {
    }
}
