/*******************************CuBeamOne_main.c-2023-02-23************************/

#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"
#include "tcp_tx_ring.h"

/**************************************main.c全局变量**************************************/
typedef unsigned long int pthread_t;
pthread_t thread[6];
pthread_mutex_t mut;
extern int fd_uio4;

int irq_on = 1;

void thread_wait(void);
void thread_create(void);
void *thread_fpga_to_upper();
void *thread_upper_to_fpga();
void *Read_PT100_Sensor_Temp();
void *tmp451_sensor();

/********************************************************************************
 * 名称：                    main
 * 功能：                    主函数
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
int main(int argc, char **argv)
{
    fpga_interface_init();
    write(fd_uio4, &irq_on, sizeof(irq_on));
    socket_server_init();
    pthread_mutex_init(&mut, NULL); // 初始化互斥锁
    thread_create();                // 创建并开启线程
    DBG("thread preok\n");
    thread_wait();               // 等待回收两个子线程，由于程序是死循环不会执行这一步
    pthread_mutex_destroy(&mut); // 销毁锁
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
    int thread_id;
    memset(&thread, 0, sizeof(thread));                                                  // 初始化thread数组；
    if ((thread_id = pthread_create(&thread[0], NULL, thread_upper_to_fpga, NULL)) != 0) // 创建thread_upper_to_fpga线程
    {
        DBG("upper to FPGA thread created fail!\n");
    }   
    else
        DBG("upper to FPGA thread is created!\n");
    if ((thread_id = pthread_create(&thread[1], NULL, thread_fpga_to_upper, NULL)) != 0) // 创建thread_fpga_to_upper线程；
    {
        DBG("FPGA to UPPER thread created fail!\n");
    }    
    else
        DBG("FPGA to UPPER thread is created!\n");
    if ((thread_id = pthread_create(&thread[2], NULL, Read_PT100_Sensor_Temp, NULL)) != 0)
    {
        DBG("PT100 sensor thread created fail!\n");
    }    
    else
        DBG("PT100 sensor thread is created!\n");
    if ((thread_id = pthread_create(&thread[3], NULL, tmp451_sensor, NULL)) != 0)
    {
        DBG("sensor thread created fail!\n");
    }    
    else
        DBG("TMP451 sensor thread is created!\n");
    if ((thread_id = pthread_create(&thread[4], NULL, thread_tcp_tx_ring_send, NULL)) != 0)
    {
        DBG("thread_tcp_tx_ring_send thread created fail!\n");
    }    
    else
        DBG("thread_tcp_tx_ring_send thread is created!\n");
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
    pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    *read PT100 sensor temp()
 * 功能：                    线程：温度传感器
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void *Read_PT100_Sensor_Temp()
{
    PT100_Sensor_Temp();
    pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    *tmp451_sensor()
 * 功能：                    线程：温度传感器
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void *tmp451_sensor()
{
    read_TMP451_sensor_and_save();
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
        printf("PT100 sensor thread is over \n");
    }
    if (thread[3] != 0)
    {
        pthread_join(thread[3], NULL);
        printf("TMP451 sensor thread is over \n");
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
        "NEONCopyPLD:                          \n"
        "    VLDM %[src]!,{d0-d7}                 \n"
        "    VSTM %[dst]!,{d0-d7}                 \n"
        "    SUBS %[sz],%[sz],#0x40                 \n"
        "    BGT NEONCopyPLD                  \n"

        : [dst] "+r"(dst), [src] "+r"(src), [sz] "+r"(sz)::"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
}
#endif