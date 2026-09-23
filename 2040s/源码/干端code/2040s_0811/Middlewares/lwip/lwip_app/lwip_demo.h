#ifndef _LWIP_DEMO_H
#define _LWIP_DEMO_H
#include "FreeRTOS.h"
#include "./SYSTEM/sys/sys.h"
#include "lwip/netdb.h"
#include <lwip/sockets.h>
#include <string.h>
#include <stdlib.h>
#include "task.h"
#include "queue.h"
#include "semphr.h"


/* ============================================================
 * 客户端连接信息
 * ============================================================ */
struct client_info
{
    int socket_num;                 /* 当前客户端的 socket 句柄 */
    struct sockaddr_in ip_addr;     /* 当前客户端的 IP 和端口信息 */
    int sockaddr_len;               /* sockaddr_in 结构体长度 */
};

/* ============================================================
 * 客户端任务信息
 * ============================================================ */
struct client_task_info
{
    UBaseType_t client_task_pro;    /* 客户端任务优先级 */
    uint16_t client_task_stk;       /* 客户端任务栈大小 */
    TaskHandle_t *client_handler;   /* 客户端任务句柄 */
    char *client_name;              /* 客户端任务名 */
    char *client_num;               /* 客户端编号字符串 */
};

/* ============================================================
 * 监听 socket 和连接 socket 信息
 * ============================================================ */
struct link_socjet_info
{
    int sock_listen;                /* 监听 socket */
    int sock_connect;               /* 已连接 socket */
    struct sockaddr_in listen_addr; /* 监听地址 */
    struct sockaddr_in connect_addr; /* 客户端连接地址 */
};

#define LWIP_SEND_DATA              0X80   
extern uint8_t g_lwip_send_flag;          

void lwip_demo(void);

/*
 * 启动湿端在线监测任务。
 * 任务通过 ICMP Ping 检测 192.168.0.5，并控制 LP5012 D2：
 * 绿色=可达，红色=连续 Ping 失败，蓝色=显控省电且湿端已下电。
 */
BaseType_t wet_end_monitor_start(void);

/*
 * 在湿端电源状态切换完成后调用：
 * sleep_mode 非 0 表示湿端已下电，暂停 Ping 并显示蓝色；
 * sleep_mode 为 0 表示湿端已上电，恢复 Ping 监测。
 */
void wet_end_monitor_sleep_mode_set(uint8_t sleep_mode);

#endif /* _CLIENT_H */
