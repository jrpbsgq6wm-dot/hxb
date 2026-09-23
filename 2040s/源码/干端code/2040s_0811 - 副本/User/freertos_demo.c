#include "freertos_demo.h"

#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./MALLOC/malloc.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/TMP175/TMP175.h"
#include "./BSP/LP5012/LP5012.h"

#include "lwip_comm.h"
#include "lwip_demo.h"	
#include "lwipopts.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <string.h>
#include "signal_monitor.h"


/* ============================================================
 * FreeRTOS 任务配置
 * ============================================================ */

/* 启动任务：负责初始化 lwIP，然后创建其他业务任务 */
#define START_TASK_PRIO     5
#define START_STK_SIZE      768
TaskHandle_t StartTask_Handler;
void start_task(void *pvParameters);


/* lwIP 任务：运行网络通信 demo */
#define LWIP_DEMO_TASK_PRIO 11
#define LWIP_DEMO_STK_SIZE  1024
TaskHandle_t LWIP_Task_Handler;
void lwip_demo_task(void *pvParameters);


/* 应用任务：按键扫描、LED 跑马灯、RGB 灯循环、温度打印 */
#define APP_TASK_PRIO       10
#define APP_STK_SIZE        256
TaskHandle_t AppTask_Handler;
void app_task(void *pvParameters);


/**
 * @brief  lwIP 状态打印函数
 * @param  mode:
 *         bit0 = 1：打印 TCPServer 启动信息
 *         bit1 = 1：打印 lwIP 初始化成功、IP 地址、网速
 */
void lwip_ui_printf(uint8_t mode)
{
    uint8_t speed;
    uint8_t buf[60];

    if (mode & (1 << 0))
    {
        printf("STM32 lwIP TCPServer Starting...\r\n");
    }

    if (mode & (1 << 1))
    {
        printf("lwIP Init Succeeded\r\n");

        if (g_lwipdev.dhcpstatus == 2)
        {
            sprintf((char *)buf,
                    "DHCP IP:%d.%d.%d.%d",
                    g_lwipdev.ip[0],
                    g_lwipdev.ip[1],
                    g_lwipdev.ip[2],
                    g_lwipdev.ip[3]);
        }
        else
        {
            sprintf((char *)buf,
                    "Static IP:%d.%d.%d.%d",
                    g_lwipdev.ip[0],
                    g_lwipdev.ip[1],
                    g_lwipdev.ip[2],
                    g_lwipdev.ip[3]);
        }

        printf("%s\r\n", (char *)buf);

        speed = ethernet_chip_get_speed();

        if (speed)
        {
            printf("Ethernet Speed:100M\r\n");
        }
        else
        {
            printf("Ethernet Speed:10M\r\n");
        }
    }
}


/**
 * @brief  FreeRTOS demo 入口
 * @note   创建启动任务，然后启动调度器
 */
void freertos_demo(void)
{
    xTaskCreate((TaskFunction_t)start_task,
                (const char *)"start_task",
                (uint16_t)START_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)START_TASK_PRIO,
                (TaskHandle_t *)&StartTask_Handler);

    vTaskStartScheduler();
}


/**
 * @brief  启动任务
 * @note   初始化 lwIP，等待 DHCP，然后创建应用任务和 lwIP 任务
 */
void start_task(void *pvParameters)
{
    (void)pvParameters;

	//设置状态显示回调函数
    g_lwipdev.lwip_display_fn = lwip_ui_printf;
	
	//打印 TCPServer 启动信息
    lwip_ui_printf(1);

    printf("Calling lwip_comm_init...\r\n");

	//初始化 lwIP 协议栈和网卡
	//静态IP  192.168.0.4 
    while (lwip_comm_init() != 0)
    {
        printf("lwIP Init failed!! Retrying...\r\n");
        delay_ms(500);
    }

    printf("lwip_comm_init OK\r\n");

    /* 等待 DHCP 成功，或者 DHCP 超时/失败 */
    while ((g_lwipdev.dhcpstatus != 2) && (g_lwipdev.dhcpstatus != 0xFF))
    {
        vTaskDelay(5);
    }

	//保护关键代码段（临界区）
    taskENTER_CRITICAL();

    /* 创建 lwIP 网络任务 */
    xTaskCreate((TaskFunction_t)lwip_demo_task,
                (const char *)"lwip_demo_task",
                (uint16_t)LWIP_DEMO_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)LWIP_DEMO_TASK_PRIO,
                (TaskHandle_t *)&LWIP_Task_Handler);

    /* 创建应用任务 */
    xTaskCreate((TaskFunction_t)app_task,
                (const char *)"app_task",
                (uint16_t)APP_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)APP_TASK_PRIO,
                (TaskHandle_t *)&AppTask_Handler);

    /* 创建信号监测任务
     * 负责监测 HEADING / MOTION / GNSS / SVS 串口数据有效性，
     * 同时监测 PPS / SYNC 外部脉冲事件，并控制对应 LP5012 指示灯。
     */
    signal_monitor_start();

    /* 启动任务完成使命，删除自己 */
    vTaskDelete(StartTask_Handler);

    taskEXIT_CRITICAL();
}


/**
 * @brief  应用任务
 * @note   负责按键扫描、普通 LED 跑马灯、LP5012 RGB 循环、温度打印
 */
void app_task(void *pvParameters)
{
    TickType_t now;
    TickType_t last_temp = xTaskGetTickCount();


    (void)pvParameters;

    while (1)
    {
        mcu_key_scan();

        now = xTaskGetTickCount();

        /* 每 1 秒通过 UART6 打印一次 TMP175 温度 */
        if ((now - last_temp) >= pdMS_TO_TICKS(1000))
        {
            last_temp = now;
            tmp175_uart6_print_temperature();
        }

        vTaskDelay(50);
    }
}


/**
 * @brief  lwIP 网络任务
 */
void lwip_demo_task(void *pvParameters)
{
    (void)pvParameters;

    lwip_demo();

    while (1)
    {
        vTaskDelay(5);
    }
}

