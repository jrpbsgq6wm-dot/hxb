   #ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/**
SYS_LIGHTWEIGHT_PROT==1:                      
 */
#define SYS_LIGHTWEIGHT_PROT            1

/* NO_SYS                                  1                  0 
                            */
#define NO_SYS                          0

/**
 * NO_SYS_NO_TIMERS==1: Drop support for sys_timeout when NO_SYS==1
 * Mainly for compatibility to old versions.
 */
#define NO_SYS_NO_TIMERS                0

/* ----------          ---------- */
/*               4           */
#define MEM_ALIGNMENT                   4

/*                                                 */
#define MEM_SIZE                        (25*1024)

/* MEMP_NUM_PBUF:                  */
#define MEMP_NUM_PBUF                   15
/* MEMP_NUM_UDP_PCB: UDP              . */
#define MEMP_NUM_UDP_PCB                4
/* MEMP_NUM_TCP_PCB: TCP      . */
#define MEMP_NUM_TCP_PCB                4
/* MEMP_NUM_TCP_PCB_LISTEN:     TCP      . */
#define MEMP_NUM_TCP_PCB_LISTEN         2
/* MEMP_NUM_TCP_SEG:           TCP        . */
#define MEMP_NUM_TCP_SEG                120
/* MEMP_NUM_SYS_TIMEOUT:                  . */
#define MEMP_NUM_SYS_TIMEOUT            6


/* ---------- Pbuf     ---------- */
/* PBUF_POOL                     */
#define PBUF_POOL_SIZE                  20
/* PBUF_POOL_BUFSIZE: pbuf        pbuf      . */
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)


/* ---------- TCP     ---------- */
#define LWIP_TCP                        1
#define TCP_TTL                         255

/*     TCP                         
                                 0. */
#define TCP_QUEUE_OOSEQ                 0

/* TCP         */
#define TCP_MSS                         (1500 - 40)   /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */

/* TCP                (    ). */
#define TCP_SND_BUF                     (11*TCP_MSS)

/*  TCP_SND_QUEUELEN: TCP                           
        (2 * TCP_SND_BUF/TCP_MSS)             */

#define TCP_SND_QUEUELEN                (8* TCP_SND_BUF/TCP_MSS)

/* TCP         */
#define TCP_WND                         (2*TCP_MSS)


/* ---------- ICMP      ---------- */
#define LWIP_ICMP                       1

/* Enable one RAW PCB for the wet-end ICMP Echo monitor. */
#define LWIP_RAW                        1
#define MEMP_NUM_RAW_PCB                1

/* ---------- DNS      ---------- */
#define LWIP_DNS                        1
#include <stdlib.h>
#define LWIP_RAND                       rand


/* ---------- DHCP      ---------- */
/*          DHCP            LWIP_DHCP      1 */
#define LWIP_DHCP                       0


/* ---------- UDP      ---------- */
#define LWIP_UDP                        1
#define UDP_TTL                         255


/* ---------- Statistics      ---------- */
#define LWIP_STATS                      0
#define LWIP_PROVIDE_ERRNO              1

/* ----------              ---------- */
/* WIP_NETIF_LINK_CALLBACK==1:                      
               (              )
 */
#define LWIP_NETIF_LINK_CALLBACK        1
/*
   --------------------------------------
   ----------             ----------
   --------------------------------------
*/

/*
The STM32F4x7 allows computing and verifying the IP, UDP, TCP and ICMP checksums by hardware:
 - To use this feature let the following define uncommented.
 - To disable it and process by CPU comment the  the checksum.

 Note: Hardware checksum offload is DISABLED to avoid HW zeroing the ICMP checksum field,
       which causes lwIP's incremental checksum adjustment to produce wrong results.
       All checksums are handled by software instead.
*/
//#define CHECKSUM_BY_HARDWARE

  /* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP                 1
  /* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP                1
  /* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP                1
  /* CHECKSUM_GEN_ICMP==1: Generate checksums in software for outgoing ICMP packets.*/
  #define CHECKSUM_GEN_ICMP               1
  /* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP               1
  /* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP              1
  /* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP              1
  /* CHECKSUM_CHECK_ICMP==1: Check checksums in software for incoming ICMP packets.*/
  #define CHECKSUM_CHECK_ICMP             1


/*
   ----------------------------------------------
   ----------             ----------
   ----------------------------------------------
*/
/**
 * LWIP_NETCONN==1:    Netconn API(        api_lib.c)
 */
#define LWIP_NETCONN                    1

/*
   ------------------------------------
   ---------- Socket     ----------
   ------------------------------------
*/
/**
 * LWIP_SOCKET==1:    Socket API(        Socket .c)
 */
#define LWIP_SOCKET                     1

/*
   ---------------------------------
   ----------              ----------
   ---------------------------------
*/


#define DEFAULT_UDP_RECVMBOX_SIZE       10
#define DEFAULT_TCP_RECVMBOX_SIZE       10
#define DEFAULT_ACCEPTMBOX_SIZE         10
/*
 * RAW socket 也需要接收邮箱。lwIP 默认值为 0，创建 ICMP RAW socket 时会触发
 * sys_mbox_new() 的 "size > 0" 断言，随后 FreeRTOS 队列创建也会失败。
 * 湿端监测每秒只发送一个 Ping，4 个邮箱项足够缓存回包及少量其他 ICMP 报文。
 */
#define DEFAULT_RAW_RECVMBOX_SIZE       4
#define DEFAULT_THREAD_STACKSIZE        1024


#define TCPIP_THREAD_NAME              "lwip_thread"
#define TCPIP_THREAD_STACKSIZE          2048
#define TCPIP_MBOX_SIZE                 8
#define TCPIP_THREAD_PRIO               5
#define LWIP_SO_RCVTIMEO                1


/*
   ----------------------------------------
   ---------- Lwip         ----------
   ----------------------------------------
*/
#define LWIP_DEBUG                      0            /*     DEBUG     */
#define ICMP_DEBUG                      LWIP_DBG_OFF /*     /    ICMPdebug  */

#endif /* __LWIPOPTS_H__ */
