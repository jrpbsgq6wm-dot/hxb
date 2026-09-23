   /**
 ****************************************************************************************************
 * @file        24cxx.c
 * @author            ()
 * @version     V1.0
 * @date        2021-10-23
 * @brief       24CXX             
 * @license     Copyright (c) 2020-2032,                                        
 ****************************************************************************************************
 * @attention
 *
 *             :           F407         
 *             :www.yuanzige.com
 *             :www.openedv.com
 *             :www..com
 *             :openedv.taobao.com
 *
 *             
 * V1.0 20211023
 *                
 *
 ****************************************************************************************************
 */

#include "./BSP/IIC/myiic.h"
#include "./BSP/24CXX/24cxx.h"
#include "./SYSTEM/delay/delay.h"


/**
 * @brief                IIC      
 * @param          
 * @retval         
 */
void at24cxx_init(void)
{
    iic_init();
}

/**
 * @brief          AT24CXX                              
 * @param       readaddr:                      
 * @retval                     
 */
uint8_t at24cxx_read_one_byte(uint16_t addr)
{
    uint8_t temp = 0;
    iic_start();    /*                    */

    /*                24CXX      ,                   
     * 1, 24C16               ,    2                     
     * 2, 24C16                  ,    1                   +                      bit1~bit3                            ,       11         
     *          24C01/02,                      (8bit)   : 1  0  1  0  A2  A1  A0  R/W
     *          24C04,                         (8bit)   : 1  0  1  0  A2  A1  a8  R/W
     *          24C08,                         (8bit)   : 1  0  1  0  A2  a9  a8  R/W
     *          24C16,                         (8bit)   : 1  0  1  0  a10 a9  a8  R/W
     *    R/W      :    /             0,         ; 1,         ;
     *    A0/A1/A2 :                1,2,3      (      24C01/02/04/8            )
     *    a8/a9/a10:                                  , 11bit                        2048         ,             24C16                  
     */    
    if (EE_TYPE > AT24C16)      /* 24C16               ,    2                      */
    {
        iic_send_byte(0xA0);    /*                , IIC                  0,              */
        iic_wait_ack();         /*                            ,            ACK */
        iic_send_byte(addr >> 8);   /*                       */
    }
    else 
    {
        iic_send_byte(0xA0 + ((addr >> 8) << 1));   /*              0xA0 +       a8/a9/a10      ,          */
    }
    
    iic_wait_ack();             /*                            ,            ACK */
    iic_send_byte(addr % 256);  /*                    */
    iic_wait_ack();             /*       ACK,                             */
    
    iic_start();                /*                          */ 
    iic_send_byte(0xA1);        /*                   , IIC                  1,              */
    iic_wait_ack();             /*                            ,            ACK */
    temp = iic_read_byte(0);    /*                          */
    iic_stop();                 /*                          */
    return temp;
}

/**
 * @brief          AT24CXX                              
 * @param       addr:                            
 * @param       data:                   
 * @retval         
 */
void at24cxx_write_one_byte(uint16_t addr, uint8_t data)
{
    /*                :at24cxx_read_one_byte      ,                       */
    iic_start();    /*                    */

    if (EE_TYPE > AT24C16)      /* 24C16               ,    2                      */
    {
        iic_send_byte(0xA0);    /*                , IIC                  0,              */
        iic_wait_ack();         /*                            ,            ACK */
        iic_send_byte(addr >> 8);   /*                       */
    }
    else
    {
        iic_send_byte(0xA0 + ((addr >> 8) << 1));   /*              0xA0 +       a8/a9/a10      ,          */
    }
    
    iic_wait_ack();             /*                            ,            ACK */
    iic_send_byte(addr % 256);  /*                    */
    iic_wait_ack();             /*       ACK,                             */
    
    /*                         ,                              ,                                              */
    iic_send_byte(data);        /*       1       */
    iic_wait_ack();             /*       ACK */
    iic_stop();                 /*                          */
    delay_ms(10);               /*       : EEPROM                ,            10ms                         */
}
 
/**
 * @brief             AT24CXX            
 *   @note                  :                            0X55,                ,                   0X55
 *                                   .       ,                     .
 *
 * @param          
 * @retval                  
 *              0:             
 *              1:             
 */
uint8_t at24cxx_check(void)
{
    uint8_t temp;
    uint16_t addr = EE_TYPE;

    temp = at24cxx_read_one_byte(addr);     /*                         AT24CXX */
    if (temp == 0x55)   /*                    */
    {
        return 0;
    }
    else    /*                                   */
    {
        at24cxx_write_one_byte(addr, 0x55); /*                 */
        temp = at24cxx_read_one_byte(255);  /*                 */

        if (temp == 0x55)return 0;
    }

    return 1;
}

/**
 * @brief          AT24CXX                                                      
 * @param       addr    :                          24c02   0~255
 * @param       pbuf    :                      
 * @param       datalen :                         
 * @retval         
 */
void at24cxx_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
    while (datalen--)
    {
        *pbuf++ = at24cxx_read_one_byte(addr++);
    }
}

/**
 * @brief          AT24CXX                                                      
 * @param       addr    :                          24c02   0~255
 * @param       pbuf    :                      
 * @param       datalen :                         
 * @retval         
 */
void at24cxx_write(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
    while (datalen--)
    {
        at24cxx_write_one_byte(addr, *pbuf);
        addr++;
        pbuf++;
    }
}






