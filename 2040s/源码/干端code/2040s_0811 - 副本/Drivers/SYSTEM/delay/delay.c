/**
 ****************************************************************************************************
 * @file        delay.c
 * @author            ()
 * @version     V1.1
 * @date        2023-02-25
 * @brief             SysTick                                          (      ucosii)
 *                    delay_init                   delay_us   delay_ms               
 * @license     Copyright (c) 2022-2032,                                        
 ****************************************************************************************************
 * @attention
 *
 *             :           F407         
 *             :www.yuanzige.com
 *             :www.openedv.com
 *             :www.alientek.com
 *             :openedv.taobao.com
 *
 *             
 * V1.0 20230206
 *                
 * V1.1 20230225
 *       SYS_SUPPORT_OS            ,                UCOSII 2.93.01      ,       OS               
 *       delay_init            8      ,                  MCU      
 *       delay_us                           ,       OS
 *       delay_ms            delay_us            .
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"


static uint32_t g_fac_us = 0;       /* us                */

/*       SYS_SUPPORT_OS         ,               OS   (         UCOS) */
#if SYS_SUPPORT_OS

/*                       ( ucos            ) */
#include "FreeRTOS.h"
#include "task.h"
#include "./BSP/KEY/key.h"

extern void xPortSysTickHandler(void);
/*       g_fac_ms      ,       ms                  ,                      ms   , (            os         ,            ) */
static uint16_t g_fac_ms = 0;

/*
 *     delay_us/delay_ms            OS                        OS                                    
 *           3            :
 *      delay_osrunning    :            OS                        ,                                       
 *      delay_ostickspersec:            OS                     ,delay_init                                 systick
 *      delay_osintnesting :            OS                  ,                                 ,delay_ms                                    
 *           3         :
 *      delay_osschedlock  :            OS            ,            
 *      delay_osschedunlock:            OS            ,                  
 *      delay_ostimedly    :      OS      ,                        .
 *
 *                 UCOSII         ,      OS,                        
 */

/*       FreeRTOS */
#define delay_osrunning     (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
#define delay_ostickspersec configTICK_RATE_HZ
#define delay_osintnesting  0


/**
 * @brief     us            ,                  (            us         )
 * @param        
 * @retval       
 */
void delay_osschedlock(void)
{
    vTaskSuspendAll();                      /* UCOSII         ,                           us       */
}

/**
 * @brief     us            ,                  
 * @param        
 * @retval       
 */
void delay_osschedunlock(void)
{
    xTaskResumeAll();                    /* UCOSII         ,             */
}

/**
 * @brief     us            ,                  
 * @param     ticks:                   
 * @retval       
 */
void delay_ostimedly(uint32_t ticks)
{
    vTaskDelay(ticks);                               /* UCOSII       */
}

/**
 * @brief     systick                  ,      OS         
 * @param     ticks :                     
 * @retval       
 */  
void SysTick_Handler(void)
{
    HAL_IncTick();
    mcu_key_tick_handler();
    /* OS             ,                               */
    if (delay_osrunning == pdTRUE)
    {
        /*        uC/OS-II     SysTick                    */
        xPortSysTickHandler();
    }
    HAL_IncTick();
}
#endif

/**
 * @brief                          
 * @param     sysclk:                   ,    CPU      (rcc_c_ck), 168MHz
 * @retval       
 */  
void delay_init(uint16_t sysclk)
{
#if SYS_SUPPORT_OS                                      /*                   OS */
    uint32_t reload;
#endif
    g_fac_us = sysclk;                                  /*          HAL_Init         systick                                              */
#if SYS_SUPPORT_OS                                      /*                   OS. */
    reload = sysclk;                                    /*                                   M */
    reload *= 1000000 / delay_ostickspersec;            /*       delay_ostickspersec                  ,reload   24   
                                                         *          ,         :16777216,   168M   ,      0.09986s      
                                                         */
    g_fac_ms = 1000 / delay_ostickspersec;              /*       OS                            */
    SysTick->CTRL |= 1 << 1;                            /*       SYSTICK       */
    SysTick->LOAD = reload;                             /*    1/delay_ostickspersec                */
    SysTick->CTRL |= 1 << 0;                            /*       SYSTICK */
#endif 
}

/**
 * @brief           nus
 * @note                        OS,                               us      
 * @param     nus:             us   
 * @note      nus            : 0 ~ (2^32 / fac_us) (fac_us                        ,                   )
 * @retval       
 */
void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;        /* LOAD       */
    ticks = nus * g_fac_us;                 /*                    */
    
#if SYS_SUPPORT_OS                          /*                   OS */
    delay_osschedlock();                    /*        OS                    */
#endif

    told = SysTick->VAL;                    /*                             */
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;        /*                   SYSTICK                                        */
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks) 
            {
                break;                      /*             /                        ,          */
            }
        }
    }

#if SYS_SUPPORT_OS                          /*                   OS */
    delay_osschedunlock();                  /*        OS                    */
#endif 

}

/**
 * @brief           nms
 * @param     nms:             ms    (0< nms <= (2^32 / fac_us / 1000))(fac_us                        ,                   )
 * @retval       
 */
void delay_ms(uint16_t nms)
{
    
#if SYS_SUPPORT_OS  /*                   OS,                      os               CPU */
    if (delay_osrunning && delay_osintnesting == 0)     /*       OS               ,                           (                              ) */
    {
        if (nms >= g_fac_ms)                            /*                      OS                      */
        {
            delay_ostimedly(nms / g_fac_ms);            /* OS       */
        }

        nms %= g_fac_ms;                                /* OS                                       ,                         */
    }
#endif

    delay_us((uint32_t)(nms * 1000));                   /*                    */
}

/**
 * @brief       HAL                              
 * @note        HAL                     Systick                        Systick                                                   
 * @param       Delay :                      
 * @retval      None
 */
void HAL_Delay(uint32_t Delay)
{
     delay_ms(Delay);
}










