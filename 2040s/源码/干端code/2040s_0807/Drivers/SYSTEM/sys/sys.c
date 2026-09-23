/**
 ****************************************************************************************************
 * @file        sys.c
 * @author            ()
 * @version     V1.0
 * @date        2021-10-14
 * @brief                            (                  /            /GPIO         )
 * @license     Copyright (c) 2020-2032,                                        
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
 * V1.0 20211014
 *                
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"


/**
 * @brief                                        
 * @param       baseaddr:       
 * @param       offset:          
 * @retval         
 */
void sys_nvic_set_vector_table(uint32_t baseaddr, uint32_t offset)
{
    /*       NVIC                           ,VTOR   9         ,   [8:0]       */
    SCB->VTOR = baseaddr | (offset & (uint32_t)0xFFFFFE00);
}

/**
 * @brief             : WFI      (                                       ,                   )
 * @param          
 * @retval         
 */
void sys_wfi_set(void)
{
    __ASM volatile("wfi");
}

/**
 * @brief                         (               fault   NMI      )
 * @param          
 * @retval         
 */
void sys_intx_disable(void)
{
    __ASM volatile("cpsid i");
}

/**
 * @brief                         
 * @param          
 * @retval         
 */
void sys_intx_enable(void)
{
    __ASM volatile("cpsie i");
}

/**
 * @brief                         
 * @note                          X,       MDK      ,                      
 * @param       addr:             
 * @retval         
 */
void sys_msr_msp(uint32_t addr)
{
    __set_MSP(addr);    /*                    */
}

/**
 * @brief                         
 * @param          
 * @retval         
 */
void sys_standby(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();    /*                    */
    SET_BIT(PWR->CR, PWR_CR_PDDS); /*                    */
}

/**
 * @brief                      
 * @param          
 * @retval         
 */
void sys_soft_reset(void)
{
    NVIC_SystemReset();
}

/**
 * @brief                         
 * @param       plln:    PLL            (PLL      ),             : 64~432.
 * @param       pllm:    PLL         PLL               (   PLL               ),             : 2~63.
 * @param       pllp:    PLL   p            (PLL               ),                            ,             : 2, 4, 6, 8.(         4      )
 * @param       pllq:    PLL   q            (PLL               ),             : 2~15.
 * @note
 *
 *              Fvco: VCO      
 *              Fsys:                   ,          PLL   p                        
 *              Fq:      PLL   q                        
 *              Fs:      PLL                  ,          HSI, HSE   .
 *              Fvco = Fs * (plln / pllm);
 *              Fsys = Fvco / pllp = Fs * (plln / (pllm * pllp));
 *              Fq   = Fvco / pllq = Fs * (plln / (pllm * pllq));
 *
 *                              8M         ,          : plln = 336, pllm = 8, pllp = 2, pllq = 7.
 *                    :Fvco = 8 * (336 / 8) = 336Mhz
 *                   Fsys = pll_p_ck = 336 / 2 = 168Mhz
 *                   Fq   = pll_q_ck = 336 / 7 = 48Mhz
 *
 *              F407                                 :
 *              CPU      (HCLK) = pll_p_ck = 168Mhz
 *              AHB1/2/3(rcc_hclk1/2/3) = 168Mhz
 *              APB1(rcc_pclk1) = pll_p_ck / 4 = 42Mhz
 *              APB1(rcc_pclk2) = pll_p_ck / 2 = 84Mhz
 *
 * @retval                  : 0,       ; 1,       ;
 */
uint8_t sys_stm32_clock_init(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq)
{
    HAL_StatusTypeDef ret = HAL_OK;
    RCC_OscInitTypeDef rcc_osc_init = {0};
    RCC_ClkInitTypeDef rcc_clk_init = {0};

    __HAL_RCC_PWR_CLK_ENABLE();                                         /*       PWR       */

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);      /*                                                                             */

    /*       HSE            HSE      PLL                  PLL1         USB       */
    rcc_osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;        /*             HSE */
    rcc_osc_init.HSEState = RCC_HSE_ON;                          /*       HSE */
    rcc_osc_init.PLL.PLLState = RCC_PLL_ON;                      /*       PLL */
    rcc_osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE;              /* PLL               HSE */
    rcc_osc_init.PLL.PLLN = plln;
    rcc_osc_init.PLL.PLLM = pllm;
    rcc_osc_init.PLL.PLLP = pllp;
    rcc_osc_init.PLL.PLLQ = pllq;
    ret = HAL_RCC_OscConfig(&rcc_osc_init);                      /*          RCC */
    if(ret != HAL_OK)
    {
        return 1;                                                /*                                                              */
    }

    /*       PLL                                 HCLK,PCLK1   PCLK2 */
    rcc_clk_init.ClockType = ( RCC_CLOCKTYPE_SYSCLK \
                                    | RCC_CLOCKTYPE_HCLK \
                                    | RCC_CLOCKTYPE_PCLK1 \
                                    | RCC_CLOCKTYPE_PCLK2);

    rcc_clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;         /*                               PLL */
    rcc_clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;                /* AHB               1 */
    rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV4;                 /* APB1               4 */
    rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV2;                 /* APB2               2 */
    ret = HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_5);   /*             FLASH               5WS            6   CPU       */
    if(ret != HAL_OK)
    {
        return 1;                                                /*                       */
    }
    
    /* STM32F405x/407x/415x/417x Z                                  */
    if (HAL_GetREVID() == 0x1001)
    {
        __HAL_FLASH_PREFETCH_BUFFER_ENABLE();                    /*       flash       */
    }
    return 0;
}


#ifdef  USE_FULL_ASSERT

/**
 * @brief                                                                                     
 * @param       file                  
 *              line                              
 * @retval         
 */
void assert_failed(uint8_t* file, uint32_t line)
{ 
    while (1)
    {
    }
}
#endif




