#include "beam.h"
#include "network.h"
#include "upper_to_fpga.h"
#include "fpga_init.h"
#include "fpga_to_upper.h"
#include "sensor.h"

/*******************************全局变量***************************************/

int          Fpga_start_mod = 0;//FPGA开始工作标志;
//sensor1_num代表GGA_ZDA,sensor3_num代表heading条数，sensor4_num代表AT条数,sensor5_num代表SVT条数
int          sensor1_num,sensor2_num,sensor3_num,sensor4_num,sensor5_num;//传感器的条数                                             
unsigned int total_len; //ARM发送给上位机的总字节数

int flag= 0;//进中断次数，每秒输出这个标志位来间接反映ping率
int flag_timer=0;//温度传感器TMP451的定时器
int fd_uio6;//中断的驱动设备名称
int irq_on = 1; //没用上
char  DataHead_S[4]={'<','<','S','T'};// ARM与上位机通信的数据头,“<<ST”,表示上位机下发给ARM的参数设置的数据头
char  DataTail_S[4]={'E','D','>','>'};//ARM与上位机通信的报尾“ED>>”
unsigned short send_crc_S,send_crc_tmp_S; //ARM发送给上位的机数据包进行和校验或者CRC校验的相关变量

/**************************************耗时测试，测试代码执行所需时间**************************************/
#ifdef TEST_TIME
struct timeval tv1;
struct timezone tz1;
struct timeval tv2;
struct timezone tz2;
struct timeval tv3;
struct timezone tz3;
struct timeval tv4;
struct timezone tz4;
#endif 
struct timeval start_time;
struct timeval end_time;
/**************************************与上位机通信结构体**************************************/

RECV_UPPER_CONFIG_PARAMETERS  recv_upper_package;//接收到上位机的参数配置包----结构体变量
RECV_UPPER_CONFIG_PARAMETERS  *ptr_recv_upper_package=&recv_upper_package;//接收到上位机的参数配置包----结构体指针

RECV_UPPER_CONFIG_PARAMETERS  cmd_package;//接收到上位机的参数配置包保存本地----结构体变量

SEND_UPPER_SONAR_STATUS       send_upper_status_information;//发送给上位机状态信息----结构体变量
SEND_UPPER_SONAR_STATUS       *ptr_send_upper_status_information = &send_upper_status_information; //发送给上位机状态信息----结构体指针

SEND_UPPER_PACKAGE_FIRST      send_to_upper_package_first; //ARM发送给上位机的声呐数据包中的前面参数部分----结构体变量
SEND_UPPER_PACKAGE_FIRST      *ptr_send_to_upper_package_first = &send_to_upper_package_first; //发送给上位机的整个数据包前面的参数部分----结构体指针

SEND_UPPER_SENSOR_FIRST    send_to_upper_sensor;//ARM发送给上位机的传感器数据包头
SEND_UPPER_SENSOR_FIRST    *ptr_send_to_upper_sensor=&send_to_upper_sensor;//ARM发送给上位机的传感器数据包头

/**************************************socket相关**************************************/                                                                  

int                 Socket_fd_server, Connect_fd;    //文件描述符
struct              sockaddr_in   Servaddr_server;    //socket相关的API结构体
volatile int        netStatus;                       //socket的连接状态标志

/**************************************UIO结构体**************************************/
UIO_CONFIG_PARAMETER uio_fpga_register = { .physical_addr = (unsigned int *)FPGA_REGISTER_BASEADDR, .uiod = "/dev/uio0", .sysfs_path_file = "/sys/class/uio/uio0/maps/map0/size"}; 
UIO_CONFIG_PARAMETER uio_tvg_register  = { .physical_addr = (unsigned int *)TVG_REGISTER_BASEADDR,  .uiod = "/dev/uio1", .sysfs_path_file = "/sys/class/uio/uio1/maps/map0/size"};  
UIO_CONFIG_PARAMETER uio_share_mem_IQ  = { .physical_addr = (unsigned int *)DDR_SHARE_MEM_BASEADDR_IQ, .uiod = "/dev/uio2", .sysfs_path_file = "/sys/class/uio/uio2/maps/map0/size" }; 
UIO_CONFIG_PARAMETER uio_sensor_mem_0  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_0, .uiod = "/dev/uio3", .sysfs_path_file = "/sys/class/uio/uio3/maps/map0/size" };
//UIO_CONFIG_PARAMETER uio_sensor_mem_1  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_1, .uiod = "/dev/uio5", .sysfs_path_file = "/sys/class/uio/uio5/maps/map0/size" };
//UIO_CONFIG_PARAMETER uio_sensor_mem_2  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_2, .uiod = "/dev/uio6", .sysfs_path_file = "/sys/class/uio/uio6/maps/map0/size" };
//UIO_CONFIG_PARAMETER uio_sensor_mem_3  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_3, .uiod = "/dev/uio7", .sysfs_path_file = "/sys/class/uio/uio7/maps/map0/size" };
//UIO_CONFIG_PARAMETER uio_sensor_mem_4  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_4, .uiod = "/dev/uio8", .sysfs_path_file = "/sys/class/uio/uio8/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_sensor_mem_8  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_8, .uiod = "/dev/uio4", .sysfs_path_file = "/sys/class/uio/uio4/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_share_mem_original  = { .physical_addr = (unsigned int *)DDR_SHARE_MEM_BASEADDR_ORIGINAL, .uiod = "/dev/uio6", .sysfs_path_file = "/sys/class/uio/uio6/maps/map0/size" };
//UIO_CONFIG_PARAMETER uio_sensor_mem_9  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_9, .uiod = "/dev/uio10", .sysfs_path_file = "/sys/class/uio/uio10/maps/map0/size" };
//UIO_CONFIG_PARAMETER uio_sensor_mem_A  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_A, .uiod = "/dev/uio11", .sysfs_path_file = "/sys/class/uio/uio11/maps/map0/size" };
//UIO_CONFIG_PARAMETER uio_sensor_mem_B  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_B, .uiod = "/dev/uio12", .sysfs_path_file = "/sys/class/uio/uio12/maps/map0/size" };
//UIO_CONFIG_PARAMETER uio_sensor_mem_C  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_C, .uiod = "/dev/uio13", .sysfs_path_file = "/sys/class/uio/uio13/maps/map0/size" };
//UIO_CONFIG_PARAMETER uio_share_mem_original  = { .physical_addr = (unsigned int *)DDR_SHARE_MEM_BASEADDR_ORIGINAL, .uiod = "/dev/uio6", .sysfs_path_file = "/sys/class/uio/uio6/maps/map0/size" }; 
UIO_CONFIG_PARAMETER uio_sensor_mem_1,uio_sensor_mem_2,uio_sensor_mem_3,uio_sensor_mem_4,uio_sensor_mem_9,uio_sensor_mem_A,uio_sensor_mem_B,uio_sensor_mem_C;
/**************************************与FPGA协议相关结构体**************************************/  
FPGA_REGISTERS fpga_register_data;//下发给FPGA的参数配置寄存器
FPGA_REGISTERS *ptr_fpga_register_data=NULL;

FPGA_DDR_FIRST fpga_ddr_frame_first;//中断接收到的ddr数据头
FPGA_DDR_FIRST *ptr_fpga_frame_first=&fpga_ddr_frame_first;

/**************************************FPGA相关**************************************/                                                            

/***********************************ddr数据的数组***************************************/
unsigned char  ddr_sonar_data[50000000];
/**************************************TMP451温度传感器相关**************************************/    
int    fd_icc; 
unsigned char local_high_value,local_low_value,remote_high_value,remote_low_value; //读出温度的整数位
unsigned char	status_register;
float	temprature1=0;
int		tem_num=1;//当温度不变化时进行累加，累加的值
uint8	i2c_read_reg=0;

int   fd_compass; 
char CompassSendBuf[5]={0x68,0x04,0x00,0x04,0x08};//罗经发送buffer
char CompassRecvBuf[14];//罗经接收buffer
char sum_calculate,recv_sum; 
int  nread_compass;
COM_CONFIG_PARAMETER com_compass  = {.dev = "/dev/ttyPS1",  .nSpeed = 9600, .nBits=8, .nEvent='N', .nStop=1 };//罗经串口

/**************************************线程3个**************************************/ 
pthread_t                       thread[3];              
pthread_mutex_t                 mut;

/*************************************GPIO相关**************************************/ 


void *thread_upper_to_fpga()
{
    receive_process_sendto_fpga();
    pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    thread_fpga_to_upper
 * 功能：                    线程：接收FPGA、传感器等外设，处理，发送给FPGA
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void *thread_fpga_to_upper()
{
    read_fpga_send_to_upper();
    pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    *thread_sensor()
 * 功能：                    线程：温度传感器
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void *thread_sensor()
{
    read_TMP451_sensor_and_save();
    pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    thread_create
 * 功能：                    创建两个线程
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void thread_create(void)
{
    int temp;
    memset(&thread, 0, sizeof(thread)); //comment1
    if((temp = pthread_create(&thread[0], NULL, thread_upper_to_fpga, NULL)) != 0)//comment2
        printf("upper to FPGA thread created fail!\n");
    else
        printf("upper to FPGA thread is created!\n");
    if((temp = pthread_create(&thread[1], NULL, thread_fpga_to_upper, NULL)) != 0)//comment3
        printf("FPGA to UPPER thread created fail!\n");
    else
        printf("FPGA to UPPER thread is created!\n");
    if ((temp = pthread_create(&thread[2], NULL, thread_sensor, NULL)) != 0) //comment3
        printf("sensor thread created fail!\n");
    else
        printf("sensor thread is created!\n");
}

/********************************************************************************
 * 名称：                    thread_wait
 * 功能：                    等待线程执行完
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void thread_wait(void)
{
    if(thread[0] !=0)
    {
        pthread_join(thread[0],NULL);
        printf("upper to FPGA thread is over \n");
    }
    if(thread[1] !=0) 
    {  
        pthread_join(thread[1],NULL);
        printf("FPGA to upper thread is over \n");
    }
    if(thread[2] !=0) 
    {  
        pthread_join(thread[2],NULL);
        printf("sensor thread is over \n");
    }
}

/*
 * 功能: 程序主入口，初始化 FPGA 接口、EPLD 管脚、TCP 服务端和工作线程。
 * 参数: argc 为命令行参数个数；argv 为命令行参数数组，当前未使用。
 * 返回值: 正常结束返回 0；当前程序主要通过线程循环运行。
 */int main(int argc, char **argv)
{
    fpga_interface_init();
    init_EPLD_MIO33_bit_hign();
    printf("2026-09-02 2040s\n");
    init_EPLD_MIO28_bit_hign();
    init_EPLD_MIO32_bit_low();
    write(fd_uio6, &irq_on, sizeof(irq_on)); 
    init_socket_server();
    //signal(SIGALRM, timer); //relate the signal and function  
    //alarm(1);
    pthread_mutex_init(&mut,NULL); //初始化互斥锁
    thread_create();//开启两个子线程
    DBG("thread preok\n");
    thread_wait();//等待回收两个子线程，由于程序是死循环不会执行这一步
    pthread_mutex_destroy(&mut);
    return 0;
}

