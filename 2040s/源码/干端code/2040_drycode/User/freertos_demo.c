   /**
 ****************************************************************************************************
 * @file        freertos_demo.c
 * @author                  ()
 * @version     V1.0
 * @date        2022-08-01
 * @brief       lwIP SOCKET CPServer          
 * @license     Copyright (c) 2020-2032,                           
 ****************************************************************************************************
 * @attention
 *
 *         :                F407      
 *         :www.yuanzige.com
 *         :www.openedv.com
 *         :www..com
 *        :openedv.taobao.com
 *
 ****************************************************************************************************
 */
 
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
#include "stdio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"


/******************************************************************************************************/
/*FreeRTOS    */

/* START_TASK          
 *     :                                    
 */
#define START_TASK_PRIO         5           /*            */
#define START_STK_SIZE          768         /*             */
TaskHandle_t StartTask_Handler;             /*        */
void start_task(void *pvParameters);        /*        */

/* LWIP_DEMO          
 *     :                                    
 */
#define LWIP_DMEO_TASK_PRIO     11          /*            */
#define LWIP_DMEO_STK_SIZE      1024        /*             */
TaskHandle_t LWIP_Task_Handler;             /*        */
void lwip_demo_task(void *pvParameters);    /*        */

/* APP_TASK          
 *     :                                    
 */
#define APP_TASK_PRIO           10          /*            */
#define APP_STK_SIZE            256         /*             */
TaskHandle_t AppTask_Handler;               /*        */
void app_task(void *pvParameters);          /*        */


/******************************************************************************************************/


/**
 * @breif           printf    lwIP    UI
 * @param       mode :  bit0:0,      ;1,            UI
 *                      bit1:0,      ;1,           UI
 * @retval        
 */
void lwip_ui_printf(uint8_t mode)
{
    uint8_t speed;
    uint8_t buf[60];

    if (mode & 1<< 0)
    {
        printf("STM32 lwIP TCPServer Starting...\r\n");
    }

    if (mode & 1 << 1)
    {
        printf("lwIP Init Successed\r\n");

        if (g_lwipdev.dhcpstatus == 2)
        {
            sprintf((char*)buf,"DHCP IP:%d.%d.%d.%d",g_lwipdev.ip[0],g_lwipdev.ip[1],g_lwipdev.ip[2],g_lwipdev.ip[3]);
        }
        else
        {
            sprintf((char*)buf,"Static IP:%d.%d.%d.%d",g_lwipdev.ip[0],g_lwipdev.ip[1],g_lwipdev.ip[2],g_lwipdev.ip[3]);
        }

        printf("%s\r\n", (char*)buf);

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
 * @breif       freertos_demo
 * @param         
 * @retval        
 */
void freertos_demo(void)
{
    /* start_task     */
    xTaskCreate((TaskFunction_t )start_task,
                (const char *   )"start_task",
                (uint16_t       )START_STK_SIZE,
                (void *         )NULL,
                (UBaseType_t    )START_TASK_PRIO,
                (TaskHandle_t * )&StartTask_Handler);

    vTaskStartScheduler(); /*             */
}

/**
 * @brief       start_task
 * @param       pvParameters :        (      )
 * @retval        
 */
void start_task(void *pvParameters)
{
    pvParameters = pvParameters;

    g_lwipdev.lwip_display_fn = lwip_ui_printf;

    lwip_ui_printf(1);    /*                      */

    printf("Calling lwip_comm_init...\r\n");
    while(lwip_comm_init() != 0)
    {
        printf("lwIP Init failed!! Retrying...\r\n");
        delay_ms(500);
    }
    printf("lwip_comm_init OK\r\n");

    while (g_lwipdev.dhcpstatus != 2 && g_lwipdev.dhcpstatus != 0xff)/*                        */
    {
        vTaskDelay(5);
    }

    //tmp175_timer_init();  /* Disabled: I2C in task context instead */

    taskENTER_CRITICAL();           /*            */

    /*     lwIP     */
    xTaskCreate((TaskFunction_t )lwip_demo_task,
                (const char*    )"lwip_demo_task",
                (uint16_t       )LWIP_DMEO_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )LWIP_DMEO_TASK_PRIO,
                (TaskHandle_t*  )&LWIP_Task_Handler);

    /*               : LED     +      */
    xTaskCreate((TaskFunction_t )app_task,
                (const char*    )"app_task",
                (uint16_t       )APP_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )APP_TASK_PRIO,
                (TaskHandle_t*  )&AppTask_Handler);

    vTaskDelete(StartTask_Handler); /*              */
    taskEXIT_CRITICAL();            /*            */

}

/**
 * @brief                   : LED     +     
 * @param       pvParameters :        (      )
 * @retval        
 */
void app_task(void *pvParameters)
{
    pvParameters = pvParameters;
    TickType_t now;
    TickType_t last_chase = xTaskGetTickCount();
    TickType_t last_rgb   = xTaskGetTickCount();
    TickType_t last_temp  = xTaskGetTickCount();
    uint8_t chase_step = 2;   /* first visible step = LED1 */
    uint8_t rgb_step   = 2;   /* first color = red */

    while(1)
    {
        mcu_key_scan();
        now = xTaskGetTickCount();

        /* LED1/2/3 running light: advance one step every 200ms, independent of U1/U2 timing */
        if (now - last_chase >= pdMS_TO_TICKS(200))
        {
            last_chase = now;
            chase_step = (chase_step + 1) % 3;
            LED1 = (chase_step == 0);
            LED2 = (chase_step == 1);
            LED3 = (chase_step == 2);
            led_sync();
        }

        /* U1/U2 color cycling: red -> green -> blue, 2s each, 30% brightness */
        if (now - last_rgb >= pdMS_TO_TICKS(2000))
        {
            last_rgb = now;
            rgb_step = (rgb_step + 1) % 3;
            switch (rgb_step)
            {
                case 0: /* red */
                    LP5012_HAL_SetAll(LP5012_U1_ADDR, 0x4D, 0x00, 0x00);
                    LP5012_HAL_SetAll(LP5012_U2_ADDR, 0x4D, 0x00, 0x00);
                    break;
                case 1: /* green */
                    LP5012_HAL_SetAll(LP5012_U1_ADDR, 0x00, 0x4D, 0x00);
                    LP5012_HAL_SetAll(LP5012_U2_ADDR, 0x00, 0x4D, 0x00);
                    break;
                default: /* blue */
                    LP5012_HAL_SetAll(LP5012_U1_ADDR, 0x00, 0x00, 0x4D);
                    LP5012_HAL_SetAll(LP5012_U2_ADDR, 0x00, 0x00, 0x4D);
                    break;
            }
        }

        if (now - last_temp >= pdMS_TO_TICKS(1000))
        {
            last_temp = now;
            tmp175_uart6_print_temperature();
        }

        vTaskDelay(50);
    }
}
/**
 * @brief       lwIP        
 * @param       pvParameters :        (      )
 * @retval        
 */
void lwip_demo_task(void *pvParameters)
{
    pvParameters = pvParameters;

    lwip_demo();            /* lwip         */

    while (1)
    {
        vTaskDelay(5);
    }
}
