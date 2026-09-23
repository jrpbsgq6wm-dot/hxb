   /**
 ****************************************************************************************************
 * @file        myiic.h
 * @author            ()
 * @version     V1.0
 * @date        2021-10-23
 * @brief       IIC             
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
 
#ifndef __MYIIC_H
#define __MYIIC_H

#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/*               */

#define IIC_SCL_GPIO_PORT               GPIOB
#define IIC_SCL_GPIO_PIN                GPIO_PIN_8
#define IIC_SCL_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)   /* PB                */

#define IIC_SDA_GPIO_PORT               GPIOB
#define IIC_SDA_GPIO_PIN                GPIO_PIN_9
#define IIC_SDA_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)   /* PB                */

/******************************************************************************************/

/* IO       */
#define IIC_SCL(x)        do{ BOARD_SOFT_IIC_SCL_SET((x)); }while(0)       /* SCL */

#define IIC_SDA(x)        do{ BOARD_SOFT_IIC_SDA_SET((x)); }while(0)       /* SDA */

#define IIC_READ_SDA     BOARD_SOFT_IIC_SDA_GET() /*       SDA */


/* IIC                   */
void iic_init(void);            /*          IIC   IO    */
void iic_start(void);           /*       IIC             */
void iic_stop(void);            /*       IIC             */
void iic_ack(void);             /* IIC      ACK       */
void iic_nack(void);            /* IIC         ACK       */
uint8_t iic_wait_ack(void);     /* IIC      ACK       */
void iic_send_byte(uint8_t txd);/* IIC                   */
uint8_t iic_read_byte(unsigned char ack);/* IIC                   */

#endif
