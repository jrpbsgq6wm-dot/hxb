   /**
 ****************************************************************************************************
 * @file        lwip_comm.c
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
 *        :openedv.taobao.com
 *
 *         
 * V1.0 20211202
 *           
 *
 ****************************************************************************************************
 */
 
#include "lwip_comm.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/init.h"
#include "ethernetif.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"
#include "./MALLOC/malloc.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"


__lwip_dev g_lwipdev;                   /* lwip           */
struct netif g_lwip_netif;              /*                       */

#if LWIP_DHCP
__IO uint8_t g_lwip_dhcp_state = LWIP_DHCP_OFF;         /* DHCP           */
#endif

/* LINK         */
#define LWIP_LINK_TASK_PRIO             3                   /*            */
#define LWIP_LINK_STK_SIZE              128 * 2             /*             */
void lwip_link_thread( void * argument );                   /*          */
void lwip_link_status_updated(struct netif *netif);       /*              */

/* DHCP         */
#define LWIP_DHCP_TASK_PRIO             4                   /*            */
#define LWIP_DHCP_STK_SIZE              128 * 2             /*             */
void lwip_periodic_handle(void *argument);                  /* DHCP     */

/**
 * @breif       lwip     IP    
 * @param       lwipx  : lwip              
 * @retval        
 */
void lwip_comm_default_ip_set(__lwip_dev *lwipx)
{
    /*         IP  :192.168.1.134 */
    lwipx->remoteip[0] = 192;
    lwipx->remoteip[1] = 168;
    lwipx->remoteip[2] = 1;
    lwipx->remoteip[3] = 27;
    
    /* MAC         */
    lwipx->mac[0] = 0xB8;
    lwipx->mac[1] = 0xAE;
    lwipx->mac[2] = 0x1D;
    lwipx->mac[3] = 0x00;
    lwipx->mac[4] = 0x01;
    lwipx->mac[5] = 0x00;
    
    /*         IP  :192.168.0.4 */
    lwipx->ip[0] = 192;
    lwipx->ip[1] = 168;
    lwipx->ip[2] = 0;
    lwipx->ip[3] = 4;
    /*             :255.255.255.0 */
    lwipx->netmask[0] = 255;
    lwipx->netmask[1] = 255;
    lwipx->netmask[2] = 255;
    lwipx->netmask[3] = 0;

    /*         :192.168.0.1 */
    lwipx->gateway[0] = 192;
    lwipx->gateway[1] = 168;
    lwipx->gateway[2] = 0;
    lwipx->gateway[3] = 1;
    lwipx->dhcpstatus = 0; /*     DHCP */
}

/**
 * @breif       LWIP      (LWIP              )
 * @param         
 * @retval      0,    
 *              1,       
 *              2,                    
 *              3,            .
 */
uint8_t lwip_comm_init(void)
{
    uint8_t retry = 0;
    struct netif *netif_init_flag;              /*     netif_add()              ,                         */
    ip_addr_t ipaddr;                           /* ip     */
    ip_addr_t netmask;                          /*          */
    ip_addr_t gw;                               /*          */
    
    printf("lwip_comm_init: tcpip_init...\r\n");
    tcpip_init(NULL, NULL);
    printf("lwip_comm_init: tcpip_init OK\r\n");

    if (ethernet_mem_malloc())
    {
        printf("lwip_comm_init: mem_malloc failed\r\n");
        return 1;
    }
    printf("lwip_comm_init: mem_malloc OK\r\n");

    lwip_comm_default_ip_set(&g_lwipdev);         /*         IP       */
    printf("lwip_comm_init: calling ethernet_init...\r\n");

    while (ethernet_init())                     /*                 ,                 5   */
    {
        printf("lwip_comm_init: ethernet_init failed, retry %d\r\n", retry + 1);
        retry++;

        if (retry > 5)
        {
            retry = 0;                          /*                      */
            printf("lwip_comm_init: ethernet_init failed after 5 retries\r\n");
            return 3;
        }
    }
    printf("lwip_comm_init: ethernet_init OK\r\n");

#if LWIP_DHCP                                   /*         IP */
    ip_addr_set_zero_ip4(&ipaddr);              /*   IP                         */
    ip_addr_set_zero_ip4(&netmask);
    printf("lwip_comm_init: netif_is_up=%d
", netif_is_up(&g_lwip_netif));
    ip_addr_set_zero_ip4(&gw);
#else   /*         IP */
    printf("DHCP:OFF\r\n");
    IP4_ADDR(&ipaddr, g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);
    IP4_ADDR(&netmask, g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
    IP4_ADDR(&gw, g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);
    printf("MAC: %d.%d.%d.%d.%d.%d\r\n", g_lwipdev.mac[0], g_lwipdev.mac[1], g_lwipdev.mac[2], g_lwipdev.mac[3], g_lwipdev.mac[4], g_lwipdev.mac[5]);
    printf("IP: %d.%d.%d.%d\r\n", g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);
    printf("MASK: %d.%d.%d.%d\r\n", g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
    printf("GW: %d.%d.%d.%d\r\n", g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);
    g_lwipdev.dhcpstatus = 0XFF;
#endif  /*                          */
    netif_init_flag = netif_add(&g_lwip_netif, (const ip_addr_t *)&ipaddr, (const ip_addr_t *)&netmask, (const ip_addr_t *)&gw, NULL, &ethernetif_init, &tcpip_input);


    if (netif_init_flag == NULL)
    {
        return 4;                           /*              */
    }
    else                                    /*               ,    netif        ,      netif     */
    {
        netif_set_default(&g_lwip_netif);     /*     netif           */

        if (netif_is_link_up(&g_lwip_netif))
        {
            netif_set_up(&g_lwip_netif);      /*   netif     */
        }
        else
        {
            netif_set_down(&g_lwip_netif);
        }
        
#if LWIP_NETIF_LINK_CALLBACK
        lwip_link_status_updated(&g_lwip_netif);    /* DHCP                   */
        netif_set_link_callback(&g_lwip_netif, lwip_link_status_updated);
        /*     PHY             */
        sys_thread_new("eth_link",
                       lwip_link_thread,            /*             */
                       &g_lwip_netif,               /*                 */
                       LWIP_LINK_STK_SIZE,          /*            */
                       LWIP_LINK_TASK_PRIO);        /*             */
#endif
    }
    
    g_lwipdev.link_status = LWIP_LINK_OFF;          /*          0 */
#if LWIP_DHCP                                       /*        DHCP     */
    g_lwipdev.dhcpstatus = 0;                       /* DHCP     0 */
    /* DHCP         */
    sys_thread_new("eth_dhcp",
                   lwip_periodic_handle,            /*             */
                   &g_lwip_netif,                   /*                 */
                   LWIP_DHCP_STK_SIZE,              /*            */
                   LWIP_DHCP_TASK_PRIO);            /*             */
#endif
    return 0;                               /*     OK. */
}

void lwip_link_status_updated(struct netif *netif)
{
    if (netif_is_up(netif))
    {
#if LWIP_DHCP
        /* Update DHCP state machine */
        g_lwip_dhcp_state = LWIP_DHCP_START;
        printf ("The network cable is connected \r\n");
#endif /* LWIP_DHCP */
    }
    else
    {
#if LWIP_DHCP
        /* Update DHCP state machine */
        g_lwip_dhcp_state = LWIP_DHCP_LINK_DOWN;
        printf ("The network cable is not connected \r\n");
#endif /* LWIP_DHCP */
    }
}
extern xSemaphoreHandle g_rx_semaphore; /*           */
void lwip_pkt_handle(void)
{
    BaseType_t xHigherPriorityTaskWoken;
    /*            */
    xSemaphoreGiveFromISR(g_rx_semaphore,&xHigherPriorityTaskWoken);/*                */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);                   /*                              */
}



/*        DHCP */
#if LWIP_DHCP

/**
 * @breif       DHCP    
 * @param       argument:         
 * @retval        
 */
void lwip_periodic_handle(void *argument)
{
     struct netif *netif = (struct netif *) argument;
    uint32_t ip = 0;
    uint32_t netmask = 0;
    uint32_t gw = 0;
    struct dhcp *dhcp;
    uint8_t iptxt[20];

    while (1)
    {
        switch (g_lwip_dhcp_state)
        {
            case LWIP_DHCP_START:
            {
                /*   IP                                */
                ip_addr_set_zero_ip4(&netif->ip_addr);
                ip_addr_set_zero_ip4(&netif->netmask);
                ip_addr_set_zero_ip4(&netif->gw);
                ip_addr_set_zero_ip4(&netif->ip_addr);
                ip_addr_set_zero_ip4(&netif->netmask);
                ip_addr_set_zero_ip4(&netif->gw);
                
                g_lwip_dhcp_state = LWIP_DHCP_WAIT_ADDRESS;
                
                printf ("State: Looking for DHCP server ...\r\n");
                dhcp_start(netif);
            }
            break;
            case LWIP_DHCP_WAIT_ADDRESS:
            {
                if (dhcp_supplied_address(netif))
                {
                    g_lwip_dhcp_state = LWIP_DHCP_ADDRESS_ASSIGNED;
                    
                    ip = g_lwip_netif.ip_addr.addr;       /*       IP     */
                    netmask = g_lwip_netif.netmask.addr;  /*              */
                    gw = g_lwip_netif.gw.addr;            /*              */
                    
                    sprintf((char *)iptxt, "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
                    printf ("IP address assigned by a DHCP server: %s\r\n", iptxt);
                    
                    if (ip != 0)
                    {
                        g_lwipdev.dhcpstatus = 2;         /* DHCP     */
                        printf("    en  MAC      :................%d.%d.%d.%d.%d.%d\r\n", g_lwipdev.mac[0], g_lwipdev.mac[1], g_lwipdev.mac[2], g_lwipdev.mac[3], g_lwipdev.mac[4], g_lwipdev.mac[5]);
                        /*           DHCP        IP     */
                        g_lwipdev.ip[3] = (uint8_t)(ip >> 24);
                        g_lwipdev.ip[2] = (uint8_t)(ip >> 16);
                        g_lwipdev.ip[1] = (uint8_t)(ip >> 8);
                        g_lwipdev.ip[0] = (uint8_t)(ip);
                        printf("    DHCP      IP    ..............%d.%d.%d.%d\r\n", g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);
                        /*         DHCP                    */
                        g_lwipdev.netmask[3] = (uint8_t)(netmask >> 24);
                        g_lwipdev.netmask[2] = (uint8_t)(netmask >> 16);
                        g_lwipdev.netmask[1] = (uint8_t)(netmask >> 8);
                        g_lwipdev.netmask[0] = (uint8_t)(netmask);
                        printf("    DHCP              ............%d.%d.%d.%d\r\n", g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
                        /*           DHCP                 */
                        g_lwipdev.gateway[3] = (uint8_t)(gw >> 24);
                        g_lwipdev.gateway[2] = (uint8_t)(gw >> 16);
                        g_lwipdev.gateway[1] = (uint8_t)(gw >> 8);
                        g_lwipdev.gateway[0] = (uint8_t)(gw);
                        printf("    DHCP                ..........%d.%d.%d.%d\r\n", g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);
                        
                        g_lwipdev.lwip_display_fn(2);
                    }
                }
                else
                {
                    dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

                    /* DHCP timeout */
                    if (dhcp->tries > LWIP_MAX_DHCP_TRIES)
                    {
                        g_lwip_dhcp_state = LWIP_DHCP_TIMEOUT;
                        g_lwipdev.dhcpstatus = 0XFF;
                        /*         IP     */
                        IP4_ADDR(&(g_lwip_netif.ip_addr), g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);
                        IP4_ADDR(&(g_lwip_netif.netmask), g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
                        IP4_ADDR(&(g_lwip_netif.gw), g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);
                        netif_set_addr(netif, &g_lwip_netif.ip_addr, &g_lwip_netif.netmask, &g_lwip_netif.gw);

                        sprintf((char *)iptxt, "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
                        printf ("DHCP Timeout !! \r\n");
                        printf ("Static IP address: %s\r\n", iptxt);
                        g_lwipdev.lwip_display_fn(2);
                    }
                }
            }
            break;
            case LWIP_DHCP_LINK_DOWN:
            {
                g_lwip_dhcp_state = LWIP_DHCP_OFF;
            }
            break;
            default: break;
        }

        /* wait 1000 ms */
        vTaskDelay(1000);
    }
}
#endif

#if LWIP_NETIF_LINK_CALLBACK
/**
  * @brief          ETH              netif
  * @param       argument: netif
  * @retval        
  */
void lwip_link_thread( void * argument )
{
    uint32_t regval = 0;
    struct netif *netif = (struct netif *) argument;
    int link_again_num = 0;

    while(1)
    {
        /*     PHY                         */
        HAL_ETH_ReadPHYRegister(&g_eth_handler,PHY_BSR, &regval);

        /*              */
        if((regval & PHY_LINKED_STATUS) == 0)
        {
            g_lwipdev.link_status = LWIP_LINK_OFF;
            
            link_again_num ++ ;
            
            if (link_again_num >= 2)                    /*                      */
            {
                continue;
            }
            else                                        /*                          */
            {
#if LWIP_DHCP                                           /*        DHCP     */
                g_lwip_dhcp_state = LWIP_DHCP_LINK_DOWN;

                dhcp_stop(netif);
#endif
                HAL_ETH_Stop(&g_eth_handler);
                netif_set_down(netif);
                netif_set_link_down(netif);
            }
        }
        else                                            /*            */
        {
            link_again_num = 0;

            if (g_lwipdev.link_status == LWIP_LINK_OFF)/*                      */
            {
                g_lwipdev.link_status = LWIP_LINK_ON;
                HAL_ETH_Start(&g_eth_handler);
                netif_set_up(netif);
                netif_set_link_up(netif);
            }
        }

        vTaskDelay(100);
    }
}
#endif
