/*
 ****************************************************************************************************
 * @file        lwip_comm.c
 * @author      ()
 * @version     V1.0
 * @date        2021-12-02
 * @brief       LWIP 通信初始化与运行管理
 * @license     Copyright (c) 2020-2032
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

/* lwIP 设备信息 */
__lwip_dev g_lwipdev;
/* lwIP 网络接口 */
struct netif g_lwip_netif;

#if LWIP_DHCP
/* DHCP 状态机 */
__IO uint8_t g_lwip_dhcp_state = LWIP_DHCP_OFF;
#endif

/* 链路检测任务 */
#define LWIP_LINK_TASK_PRIO     3
#define LWIP_LINK_STK_SIZE      (128 * 2)
void lwip_link_thread(void *argument);
void lwip_link_status_updated(struct netif *netif);

/* DHCP 处理任务 */
#define LWIP_DHCP_TASK_PRIO     4
#define LWIP_DHCP_STK_SIZE      (128 * 2)
void lwip_periodic_handle(void *argument);

/**
 * @brief  设置 lwIP 默认网络参数
 * @param  lwipx: lwIP 设备结构体指针
 */
void lwip_comm_default_ip_set(__lwip_dev *lwipx)
{
    /* 远端主机 IP */
    lwipx->remoteip[0] = 192;
    lwipx->remoteip[1] = 168;
    lwipx->remoteip[2] = 1;
    lwipx->remoteip[3] = 27;

    /* MAC 地址 */
    lwipx->mac[0] = 0xB8;
    lwipx->mac[1] = 0xAE;
    lwipx->mac[2] = 0x1D;
    lwipx->mac[3] = 0x00;
    lwipx->mac[4] = 0x01;
    lwipx->mac[5] = 0x00;

    /* 本机静态 IP */
    lwipx->ip[0] = 192;
    lwipx->ip[1] = 168;
    lwipx->ip[2] = 0;
    lwipx->ip[3] = 4;

    /* 子网掩码 */
    lwipx->netmask[0] = 255;
    lwipx->netmask[1] = 255;
    lwipx->netmask[2] = 255;
    lwipx->netmask[3] = 0;

    /* 网关 */
    lwipx->gateway[0] = 192;
    lwipx->gateway[1] = 168;
    lwipx->gateway[2] = 0;
    lwipx->gateway[3] = 1;

    /* 默认不启用 DHCP */
    lwipx->dhcpstatus = 0;
}

/**
 * @brief  初始化 lwIP 协议栈和网卡
 * @retval 0 成功
 *         1 内存申请失败
 *         2 未使用
 *         3 网卡初始化失败超过重试次数
 *         4 netif_add 失败
 */
uint8_t lwip_comm_init(void)
{
    uint8_t retry = 0;
    struct netif *netif_init_flag;
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;

    printf("lwip_comm_init: tcpip_init...\r\n");
    tcpip_init(NULL, NULL);
    printf("lwip_comm_init: tcpip_init OK\r\n");

    /* 申请 lwIP 所需内存 */
    if (ethernet_mem_malloc())
    {
        printf("lwip_comm_init: mem_malloc failed\r\n");
        return 1;
    }
    printf("lwip_comm_init: mem_malloc OK\r\n");

    /* 设置默认网络参数 */
    lwip_comm_default_ip_set(&g_lwipdev);
    printf("lwip_comm_init: calling ethernet_init...\r\n");

    /* 初始化以太网硬件，失败则重试 */
    while (ethernet_init())
    {
        printf("lwip_comm_init: ethernet_init failed, retry %d\r\n", retry + 1);
        retry++;

        if (retry > 5)
        {
            retry = 0;
            printf("lwip_comm_init: ethernet_init failed after 5 retries\r\n");
            return 3;
        }
    }
    printf("lwip_comm_init: ethernet_init OK\r\n");

#if LWIP_DHCP
    /* 开启 DHCP 时，先把地址置零 */
    ip_addr_set_zero_ip4(&ipaddr);
    ip_addr_set_zero_ip4(&netmask);
    ip_addr_set_zero_ip4(&gw);
    printf("lwip_comm_init: netif_is_up=%d\r\n", netif_is_up(&g_lwip_netif));
#else
    /* 关闭 DHCP 时，直接使用静态 IP */
    printf("DHCP:OFF\r\n");
    IP4_ADDR(&ipaddr, g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);
    IP4_ADDR(&netmask, g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
    IP4_ADDR(&gw, g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);

    printf("MAC: %d.%d.%d.%d.%d.%d\r\n",
           g_lwipdev.mac[0], g_lwipdev.mac[1], g_lwipdev.mac[2],
           g_lwipdev.mac[3], g_lwipdev.mac[4], g_lwipdev.mac[5]);
    printf("IP: %d.%d.%d.%d\r\n", g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);
    printf("MASK: %d.%d.%d.%d\r\n", g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
    printf("GW: %d.%d.%d.%d\r\n", g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);

    g_lwipdev.dhcpstatus = 0xFF;
#endif

    /* 把以太网接口挂到 lwIP 上 */
    netif_init_flag = netif_add(&g_lwip_netif,
                                (const ip_addr_t *)&ipaddr,
                                (const ip_addr_t *)&netmask,
                                (const ip_addr_t *)&gw,
                                NULL,
                                &ethernetif_init,
                                &tcpip_input);

    if (netif_init_flag == NULL)
    {
        return 4;
    }
    else
    {
        netif_set_default(&g_lwip_netif);

        if (netif_is_link_up(&g_lwip_netif))
        {
            netif_set_up(&g_lwip_netif);
        }
        else
        {
            netif_set_down(&g_lwip_netif);
        }

#if LWIP_NETIF_LINK_CALLBACK
        /* 注册链路变化回调 */
        lwip_link_status_updated(&g_lwip_netif);
        netif_set_link_callback(&g_lwip_netif, lwip_link_status_updated);

        /* 创建链路监测线程 */
        sys_thread_new("eth_link",
                       lwip_link_thread,
                       &g_lwip_netif,
                       LWIP_LINK_STK_SIZE,
                       LWIP_LINK_TASK_PRIO);
#endif
    }

    g_lwipdev.link_status = LWIP_LINK_OFF;

#if LWIP_DHCP
    /* 创建 DHCP 处理线程 */
    g_lwipdev.dhcpstatus = 0;
    sys_thread_new("eth_dhcp",
                   lwip_periodic_handle,
                   &g_lwip_netif,
                   LWIP_DHCP_STK_SIZE,
                   LWIP_DHCP_TASK_PRIO);
#endif

    return 0;
}

/**
 * @brief  网线插拔状态回调
 * @param  netif: lwIP 网络接口
 */
void lwip_link_status_updated(struct netif *netif)
{
    if (netif_is_up(netif))
    {
#if LWIP_DHCP
        g_lwip_dhcp_state = LWIP_DHCP_START;
        printf("The network cable is connected\r\n");
#endif
    }
    else
    {
#if LWIP_DHCP
        g_lwip_dhcp_state = LWIP_DHCP_LINK_DOWN;
        printf("The network cable is not connected\r\n");
#endif
    }
}

/* 中断里通知接收任务有新包 */
extern xSemaphoreHandle g_rx_semaphore;
void lwip_pkt_handle(void)
{
    BaseType_t xHigherPriorityTaskWoken;

    xSemaphoreGiveFromISR(g_rx_semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

#if LWIP_DHCP
/**
 * @brief  DHCP 状态机处理任务
 * @param  argument: netif 指针
 */
void lwip_periodic_handle(void *argument)
{
    struct netif *netif = (struct netif *)argument;
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
                /* 清空当前地址，准备申请 DHCP */
                ip_addr_set_zero_ip4(&netif->ip_addr);
                ip_addr_set_zero_ip4(&netif->netmask);
                ip_addr_set_zero_ip4(&netif->gw);

                g_lwip_dhcp_state = LWIP_DHCP_WAIT_ADDRESS;

                printf("State: Looking for DHCP server ...\r\n");
                dhcp_start(netif);
            }
            break;

            case LWIP_DHCP_WAIT_ADDRESS:
            {
                if (dhcp_supplied_address(netif))
                {
                    g_lwip_dhcp_state = LWIP_DHCP_ADDRESS_ASSIGNED;

                    ip = g_lwip_netif.ip_addr.addr;
                    netmask = g_lwip_netif.netmask.addr;
                    gw = g_lwip_netif.gw.addr;

                    sprintf((char *)iptxt, "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
                    printf("IP address assigned by a DHCP server: %s\r\n", iptxt);

                    if (ip != 0)
                    {
                        g_lwipdev.dhcpstatus = 2;

                        printf("    DHCP MAC      : %d.%d.%d.%d.%d.%d\r\n",
                               g_lwipdev.mac[0], g_lwipdev.mac[1], g_lwipdev.mac[2],
                               g_lwipdev.mac[3], g_lwipdev.mac[4], g_lwipdev.mac[5]);

                        g_lwipdev.ip[3] = (uint8_t)(ip >> 24);
                        g_lwipdev.ip[2] = (uint8_t)(ip >> 16);
                        g_lwipdev.ip[1] = (uint8_t)(ip >> 8);
                        g_lwipdev.ip[0] = (uint8_t)(ip);
                        printf("    DHCP IP      : %d.%d.%d.%d\r\n",
                               g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);

                        g_lwipdev.netmask[3] = (uint8_t)(netmask >> 24);
                        g_lwipdev.netmask[2] = (uint8_t)(netmask >> 16);
                        g_lwipdev.netmask[1] = (uint8_t)(netmask >> 8);
                        g_lwipdev.netmask[0] = (uint8_t)(netmask);
                        printf("    DHCP MASK    : %d.%d.%d.%d\r\n",
                               g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);

                        g_lwipdev.gateway[3] = (uint8_t)(gw >> 24);
                        g_lwipdev.gateway[2] = (uint8_t)(gw >> 16);
                        g_lwipdev.gateway[1] = (uint8_t)(gw >> 8);
                        g_lwipdev.gateway[0] = (uint8_t)(gw);
                        printf("    DHCP GW      : %d.%d.%d.%d\r\n",
                               g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);

                        g_lwipdev.lwip_display_fn(2);
                    }
                }
                else
                {
                    dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

                    /* DHCP 超时，回退到静态 IP */
                    if (dhcp->tries > LWIP_MAX_DHCP_TRIES)
                    {
                        g_lwip_dhcp_state = LWIP_DHCP_TIMEOUT;
                        g_lwipdev.dhcpstatus = 0xFF;

                        IP4_ADDR(&(g_lwip_netif.ip_addr),
                                 g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]);
                        IP4_ADDR(&(g_lwip_netif.netmask),
                                 g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
                        IP4_ADDR(&(g_lwip_netif.gw),
                                 g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);
                        netif_set_addr(netif, &g_lwip_netif.ip_addr, &g_lwip_netif.netmask, &g_lwip_netif.gw);

                        sprintf((char *)iptxt, "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
                        printf("DHCP Timeout!!\r\n");
                        printf("Static IP address: %s\r\n", iptxt);

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

            default:
                break;
        }

        /* 每 1 秒处理一次 DHCP 状态 */
        vTaskDelay(1000);
    }
}
#endif

#if LWIP_NETIF_LINK_CALLBACK
/**
 * @brief  链路检测线程
 * @param  argument: netif 指针
 */
void lwip_link_thread(void *argument)
{
    uint32_t regval = 0;
    struct netif *netif = (struct netif *)argument;
    int link_again_num = 0;

    while (1)
    {
        /* 读取 PHY 状态寄存器 */
        HAL_ETH_ReadPHYRegister(&g_eth_handler, PHY_BSR, &regval);

        /* 网线断开 */
        if ((regval & PHY_LINKED_STATUS) == 0)
        {
            g_lwipdev.link_status = LWIP_LINK_OFF;

            link_again_num++;

            if (link_again_num >= 2)
            {
                continue;
            }
            else
            {
#if LWIP_DHCP
                g_lwip_dhcp_state = LWIP_DHCP_LINK_DOWN;
                dhcp_stop(netif);
#endif
                HAL_ETH_Stop(&g_eth_handler);
                netif_set_down(netif);
                netif_set_link_down(netif);
            }
        }
        else
        {
            /* 网线连接 */
            link_again_num = 0;

            if (g_lwipdev.link_status == LWIP_LINK_OFF)
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
