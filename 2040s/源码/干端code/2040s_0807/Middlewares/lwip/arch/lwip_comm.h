#ifndef _LWIP_COMM_H
#define _LWIP_COMM_H

#include "./BSP/ETHERNET/ethernet.h"

/* ============================================================
 * DHCP 状态
 * ============================================================ */
#define LWIP_DHCP_OFF              (uint8_t)0    /* 未开启 DHCP */
#define LWIP_DHCP_START            (uint8_t)1    /* 开始申请 DHCP */
#define LWIP_DHCP_WAIT_ADDRESS     (uint8_t)2    /* 等待 DHCP 分配 IP */
#define LWIP_DHCP_ADDRESS_ASSIGNED (uint8_t)3    /* 已获取 DHCP 地址 */
#define LWIP_DHCP_TIMEOUT          (uint8_t)4    /* DHCP 超时 */
#define LWIP_DHCP_LINK_DOWN        (uint8_t)5    /* 网线断开 */

/* ============================================================
 * 链路状态
 * ============================================================ */
#define LWIP_LINK_OFF              (uint8_t)0    /* 网线未连接 */
#define LWIP_LINK_ON               (uint8_t)1    /* 网线已连接 */
#define LWIP_LINK_AGAIN            (uint8_t)2    /* 再次确认链路 */

/* DHCP 最大重试次数 */
#define LWIP_MAX_DHCP_TRIES        (uint8_t)4

/* 状态显示回调函数类型 */
typedef void (*display_fn)(uint8_t index);

/* ============================================================
 * lwIP 设备信息结构体
 * ============================================================ */
typedef struct
{
    uint8_t mac[6];             /* 本机 MAC 地址 */
    uint8_t remoteip[4];        /* 远端主机 IP */
    uint8_t ip[4];              /* 本机 IP */
    uint8_t netmask[4];        /* 子网掩码 */
    uint8_t gateway[4];        /* 网关 IP */

    uint8_t dhcpstatus;        /* DHCP 状态
                                * 0    : 未开启 DHCP
                                * 1    : 正在申请 DHCP
                                * 2    : 已获取 DHCP 地址
                                * 0xFF : DHCP 超时，转静态 IP
                                */

    uint8_t link_status;       /* 链路状态 */
    display_fn lwip_display_fn;/* 状态显示回调 */
} __lwip_dev;

extern __lwip_dev g_lwipdev;

/* ============================================================
 * 函数声明
 * ============================================================ */

/* 设置 lwIP 默认 IP/MAC 参数 */
void lwip_comm_default_ip_set(__lwip_dev *lwipx);

/* 初始化 lwIP 协议栈和以太网接口 */
uint8_t lwip_comm_init(void);

#endif
