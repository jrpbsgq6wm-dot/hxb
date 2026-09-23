   /**
 ****************************************************************************************************
 * @file        lwip_demo.h
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
 *         :openedv.taobao.com
 *
 ****************************************************************************************************
 */
 
#ifndef _LWIP_DEMO_H
#define _LWIP_DEMO_H
#include "./SYSTEM/sys/sys.h"


#define LWIP_SEND_DATA              0X80      /*                */
extern uint8_t g_lwip_send_flag;              /*                */

void lwip_demo(void);

#endif /* _CLIENT_H */
