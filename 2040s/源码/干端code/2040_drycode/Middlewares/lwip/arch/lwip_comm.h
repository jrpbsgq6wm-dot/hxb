   /**
 ****************************************************************************************************
 * @file        lwip_comm.h
 * @author                  ()
 * @version     V1.0
 * @date        2021-12-02
 * @brief       LWIP            
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
 *         
 * V1.0 20211202
 *           
 *
 ****************************************************************************************************
 */
 
#ifndef _LWIP_COMM_H
#define _LWIP_COMM_H 
#include "./BSP/ETHERNET/ethernet.h"


/* DHCP         */
#define LWIP_DHCP_OFF                   (uint8_t) 0     /* DHCP               */
#define LWIP_DHCP_START                 (uint8_t) 1     /* DHCP               */
#define LWIP_DHCP_WAIT_ADDRESS          (uint8_t) 2     /* DHCP              IP     */
#define LWIP_DHCP_ADDRESS_ASSIGNED      (uint8_t) 3     /* DHCP                     */
#define LWIP_DHCP_TIMEOUT               (uint8_t) 4     /* DHCP               */
#define LWIP_DHCP_LINK_DOWN             (uint8_t) 5     /* DHCP                   */

/*          */
#define LWIP_LINK_OFF                   (uint8_t) 0     /*              */
#define LWIP_LINK_ON                    (uint8_t) 1     /*              */
#define LWIP_LINK_AGAIN                 (uint8_t) 2     /*          */

/* DHCP                   */
#define LWIP_MAX_DHCP_TRIES             (uint8_t) 4

typedef void (*display_fn)(uint8_t index);

/*lwip          */
typedef struct  
{
    uint8_t mac[6];                 /* MAC     */
    uint8_t remoteip[4];            /*         IP     */ 
    uint8_t ip[4];                  /*     IP     */
    uint8_t netmask[4];             /*          */
    uint8_t gateway[4];             /*           IP     */
    uint8_t dhcpstatus;             /* dhcp     */
                                        /* 0,       DHCP    ;*/
                                        /* 1,     DHCP        */
                                        /* 2,         DHCP    */
                                        /* 0XFF,         */
    uint8_t link_status;                                /*          */
    display_fn lwip_display_fn;                         /*              */
}__lwip_dev;

extern __lwip_dev g_lwipdev;          /* lwip           */
 
void    lwip_comm_default_ip_set(__lwip_dev *lwipx);    /* lwip     IP     */
uint8_t lwip_comm_init(void);                           /* LWIP      (LWIP              ) */

#endif













