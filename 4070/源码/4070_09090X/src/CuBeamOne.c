/*******************************CuBeamOne_main.c-2026-1-12************************/
#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"
#include "tcp_tx_ring.h"

#include "sensor_log.h"

#include "eeprom.h"
#include "tmp451.h"

/**************************************main.c全局变量**************************************/
typedef unsigned long int pthread_t;
pthread_t thread[5];
pthread_mutex_t mut;
extern int fd_uio2;

int irq_on = 1;
uint8_t m_s_flag;   //主从探头标志位

char arm_ipaddr[15];
void thread_wait(void);
void thread_create(void);
void *thread_fpga_to_upper();
void *thread_upper_to_fpga();
void *thread_tmp451();
void *thread_pri();
/********************************************************************************
 * 名称：                    main
 * 功能：                    主函数
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
int main(int argc, char **argv)
{
    printf("/*******************************/\n");
    printf("/***********STARTEST************/\n");
    printf("/*********CuBeam_4070***********/\n");
    printf("/*******************************/\n");
    /*uio初始化*/
    fpga_interface_init();
    write(fd_uio2, &irq_on, sizeof(irq_on));
    /*IIC初始化*/
        /*EEPROM*/
        eeprom_fd = at24c256_init();
        if(eeprom_fd != -1){
            printf("Init EEPROM Successully :%d\n",eeprom_fd);
        }
        /*TMP451*/
        tmp451_fd = tmp451_init();
        if(tmp451_fd != -1){
            printf("Init TMP451 Successully :%d\n",tmp451_fd);
        }
    /*创建tcp连接*/
    socket_server_init();
    System_IPconfig("eth0",arm_ipaddr);
    printf("eth0 : IP %s (%d)\r\n",arm_ipaddr,strlen(arm_ipaddr));
    if(strncmp(arm_ipaddr,MASTER_IPADDR,strlen(arm_ipaddr)) == 0){
        printf("THIS IS MASTER IP\r\n");
        printf("%d\n",MASTER_VER);
        m_s_flag = 1;
    }else if(strncmp(arm_ipaddr,SLAVE_IPADDR,strlen(arm_ipaddr)) == 0){
        printf("THIS IS SLAVE IP\r\n");
        m_s_flag = 0;
        printf("%d\n",SLAVE_VER);
    }


    pthread_mutex_init(&mut, NULL); // 初始化互斥锁
    thread_create();                // 创建并开启线程
    thread_wait();                  // 等待回收两个子线程，由于程序是死循环不会执行这一步
    pthread_mutex_destroy(&mut);    // 销毁锁
    printf("********end********\n");
    return 0;
}

/********************************************************************************
 * 名称：                    thread_create
 * 功能：                    创建4个线程
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void thread_create(void)
{
    int thread_id,ret;
    memset(&thread, 0, sizeof(thread));  
    if ((thread_id = pthread_create(&thread[0], NULL, thread_upper_to_fpga, NULL)) != 0){
        DBG("FPGA to UPPER thread created fail! errno=%d, reason=%s\n", ret, strerror(ret));
    }
    if ((thread_id = pthread_create(&thread[1], NULL, thread_fpga_to_upper, NULL)) != 0){
        DBG("FPGA to UPPER thread created fail! errno=%d, reason=%s\n", ret, strerror(ret));
    }
    if ((thread_id = pthread_create(&thread[2], NULL, thread_tmp451, NULL)) != 0){
        DBG("tmp451 thread created fail! errno=%d, reason=%s\n", ret, strerror(ret));
    }
    // if ((thread_id = pthread_create(&thread[3], NULL, sensor_write_thread_func, &g_sensor_queue)) != 0) {
    //     DBG("sensor write thread created fail! errno=%d, reason=%s\n", ret, strerror(ret));
    // } else {
    //     DBG("sensor write thread created successfully\n");
    // }
    /*
     * 单独创建TCP大包发送线程。
     * FPGA->UPPER线程只负责组帧并入队，真正可能阻塞的send操作放到该线程中执行。
     */
    if ((thread_id = pthread_create(&thread[4], NULL, thread_tcp_tx_ring_send, NULL)) != 0) {
        DBG("tcp tx thread created fail! errno=%d, reason=%s\n", thread_id, strerror(thread_id));
    } else {
        DBG("tcp tx thread created successfully\n");
    }
}

/********************************************************************************
 * 名称：                    thread_upper_to_fpga
 * 功能：                    线程：接收显控，处理，发送给FPGA
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void *thread_upper_to_fpga()
{
    Recv_Upper_SendTo_Fpga();
    pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    thread_fpga_to_upper
 * 功能：                    线程：接收FPGA、传感器等外设，处理，发送给显控
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void *thread_fpga_to_upper()
{
    Copy_Fpga_SendTo_Upper();
    printf("Copy_Fpga_SendTo_Upper over\n");
    pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    thread_tmp451
 * 功能：                    线程：温度传感器线程
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void *thread_tmp451()
{
    tmp451_func();
    pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    thread_pri
 * 功能：                    打印输出线程
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void *thread_pri()
{
    thread_pri_func();
    pthread_exit(NULL);
}


/********************************************************************************
 * 名称：                    thread_wait
 * 功能：                    等待线程执行完
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void thread_wait(void)
{
    if (thread[0] != 0)
    {
        pthread_join(thread[0], NULL);
        printf("upper to FPGA thread is over \n");
    }
    if (thread[1] != 0)
    {
        pthread_join(thread[1], NULL);
        printf("FPGA to upper thread is over \n");
    }
    if (thread[2] != 0)
    {
        pthread_join(thread[2], NULL);
        printf("tmp451 thread is over \n");
    }
    if (thread[3] != 0)
    {
        pthread_join(thread[3], NULL);
        printf("sensor write thread is over \n");
    }
    if (thread[4] != 0)
    {
        pthread_join(thread[4], NULL);
        printf("tcp tx thread is over \n");
    }
}

/********************************************************************************
 * 名称：                    my_copy
 * 功能：                    汇编实现memcpy函数
 * 入口参数：            	 volatile unsigned char *dst, volatile unsigned char *src, int sz
 * 出口参数：            	 无
 *********************************************************************************/
#if 0
void my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz) 
{
    int sz_neon = sz & -64;
    int sz_remainder = sz & 63;

    asm volatile(
        "NEONCopyPLD:                          \n"
        "    PLD [%[src], #256]                 \n"
        /*"    VLD1.8 {d0-d7}, [%[src]:64]!    \n"
        "    VST1.8 {d0-d7}, [%[dst]:64]!      \n"*/
        "    VLDM %[src]!,{d0-d7}                 \n"
        "    VSTM %[dst]!,{d0-d7}                 \n"
        "    SUBS %[sz], %[sz], #64             \n"
        "    BGT NEONCopyPLD                    \n"
        /*"NEONCopyRemainder:                     \n"
        "    CMP %[sz_remainder], #0             \n"
        "    BEQ NEONCopyFinish                  \n"
        "    PLD [%[src], #64]                  \n"
        "    VLDM %[src]!,{d0-d7}                 \n"
        "    VSTM %[dst]!,{d0-d7}                 \n"
        "    VLD1.8 {d0-d1}, [%[src]]            \n"
        "    VST1.8 {d0-d1}, [%[dst]]            \n"
        "NEONCopyFinish:                        \n"*/
        : [dst] "+r" (dst), [src] "+r" (src), [sz] "+r" (sz), [sz_remainder] "+r" (sz_remainder) :: "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
}
#endif
#if 1
void my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz)
{
    if (sz & 63)
    {
        sz = (sz & -64) + 64;
    }

    asm volatile(
        "NEONCopyPLD:                               \n"
        "    VLDM %[src]!,{d0-d7}                   \n"
        "    VSTM %[dst]!,{d0-d7}                   \n"
        "    SUBS %[sz],%[sz],#0x40                 \n"
        "    BGT NEONCopyPLD                        \n"

        : [dst] "+r"(dst), [src] "+r"(src), [sz] "+r"(sz)::"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
}
#endif
