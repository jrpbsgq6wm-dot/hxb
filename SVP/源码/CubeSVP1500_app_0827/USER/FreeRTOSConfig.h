#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
#include <stddef.h>

extern uint32_t SystemCoreClock;
void vApplicationAssertFailed(const char *file, int line);

/*
 * 调度器基础配置：
 * SysTick 使用 1 kHz。这样既保持原 HAL 的毫秒时间基准，也让串口命令任务
 * 可以按 1 ms 周期检查已接收的数据帧。
 */
#define configUSE_PREEMPTION                            1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION         1
#define configUSE_TICKLESS_IDLE                         0
#define configCPU_CLOCK_HZ                              SystemCoreClock
#define configTICK_RATE_HZ                              1000
#define configMAX_PRIORITIES                            4
#define configMINIMAL_STACK_SIZE                        128
#define configMAX_TASK_NAME_LEN                         16
#define configUSE_16_BIT_TICKS                          0
#define configIDLE_SHOULD_YIELD                         1
#define configUSE_TASK_NOTIFICATIONS                    1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES           1

/*
 * 当前工程使用任务通知完成 TIM5 到文件任务的事件转交；同时 I2C1 挂接 RTC、
 * 电池计量芯片和 EEPROM，多个任务会访问该总线，因此启用互斥锁保护 I2C1。
 */
#define configUSE_MUTEXES                               1
#define configUSE_RECURSIVE_MUTEXES                     0
#define configUSE_COUNTING_SEMAPHORES                   0
#define configQUEUE_REGISTRY_SIZE                       0
#define configUSE_QUEUE_SETS                            0
#define configUSE_TIME_SLICING                          1
#define configUSE_NEWLIB_REENTRANT                      0
#define configENABLE_BACKWARD_COMPATIBILITY             0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS         0

/*
 * heap_4.c 提供一块 16 KiB 的 FreeRTOS 堆，用于任务控制块、任务栈和互斥锁。
 * 文件任务和命令任务各使用 1536 个字（6 KiB），状态任务使用 512 个字（2 KiB）。
 */
#define configSUPPORT_STATIC_ALLOCATION                 0
#define configSUPPORT_DYNAMIC_ALLOCATION                1
#define configTOTAL_HEAP_SIZE                           ( ( size_t ) ( 16 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP                0
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP       0

#define configUSE_IDLE_HOOK                             0
#define configUSE_TICK_HOOK                             0
#define configCHECK_FOR_STACK_OVERFLOW                  2
#define configUSE_MALLOC_FAILED_HOOK                    1
#define configUSE_DAEMON_TASK_STARTUP_HOOK              0

#define configGENERATE_RUN_TIME_STATS                   0
#define configUSE_TRACE_FACILITY                        0
#define configUSE_STATS_FORMATTING_FUNCTIONS            0
#define configUSE_CO_ROUTINES                           0
#define configMAX_CO_ROUTINE_PRIORITIES                 2
#define configUSE_TIMERS                                0

#define INCLUDE_vTaskPrioritySet                        0
#define INCLUDE_uxTaskPriorityGet                       0
#define INCLUDE_vTaskDelete                             0
#define INCLUDE_vTaskSuspend                            0
#define INCLUDE_xResumeFromISR                          0
#define INCLUDE_vTaskDelayUntil                         1
#define INCLUDE_vTaskDelay                              1
#define INCLUDE_xTaskGetSchedulerState                  1
#define INCLUDE_xTaskGetCurrentTaskHandle               0
#define INCLUDE_uxTaskGetStackHighWaterMark             1
#define INCLUDE_xTaskGetIdleTaskHandle                  0
#define INCLUDE_eTaskGetState                           0
#define INCLUDE_xEventGroupSetBitFromISR                0
#define INCLUDE_xTimerPendFunctionCall                  0
#define INCLUDE_xTaskAbortDelay                         0
#define INCLUDE_xTaskGetHandle                          0
#define INCLUDE_xTaskResumeFromISR                      0

/*
 * Cortex-M 的中断优先级有效位位于优先级字节的高四位。
 * 数值优先级为 0 到 4 的中断不能调用 FreeRTOS 的 FromISR API。
 * 本版本保持 TIM2、TIM5、USART/DMA 的原有高优先级；它们仍通过原有 volatile
 * 标志位把事件交给任务上下文处理，避免改变现有采集和通信中断时序。
 */
#ifdef __NVIC_PRIO_BITS
#define configPRIO_BITS                                 __NVIC_PRIO_BITS
#else
#define configPRIO_BITS                                 4
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY                 ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY            ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

#define xPortPendSVHandler                              PendSV_Handler
#define vPortSVCHandler                                 SVC_Handler

/* FreeRTOS 内部断言失败时进入可由调试器观察的停止状态。 */
#define configASSERT( x )                               do { if( ( x ) == 0 ) { vApplicationAssertFailed( __FILE__, __LINE__ ); } } while( 0 )

#endif
