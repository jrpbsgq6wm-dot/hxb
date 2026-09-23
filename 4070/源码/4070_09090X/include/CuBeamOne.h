/*******************************CuBeamOne头文件2023.2.23**********************/

#ifndef _CUBEAMONE_H_ // 判断该文件内有没有定义过这个宏

#define _CUBEAMONE_H_ // 若没有则定义

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/syscall.h>
#include <netinet/tcp.h>

#define DEBUG
/*******************************DEBUG宏定义的说明 ****************************************/

#ifdef DEBUG
#define DBG(...)                                                               \
    fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); \
    fprintf(stderr, __VA_ARGS__)
#define Debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DBG(...)
#define Debug(fmt, ...)
#endif

#define PRI 0
/*************************外部引用变量*************************************/
extern int irq_on;
extern pthread_mutex_t mut;
extern char arm_ipaddr[15];
extern uint8_t m_s_flag; 
#define MASTER_IPADDR   "192.168.0.4"
#define SLAVE_IPADDR    "192.168.0.5"

/*************************大小定义*************************************/
#define SIZE_OF_GENERAL_HEAD_LEN    4           
#define SIZE_OF_GENERAL_TAIL_LEN    4
#define SIZE_OF_STATUS_HEAD_LEN     4           //ARM->显控:硬件信息状态 头长度
#define SIZE_OF_STATUS_TAIL_LEN     4           //ARM->显控:硬件信息状态 尾长度
#define SIZE_OF_STATUS_SEND         32          // ARM发送给干端的硬件状态信息（<<RS - <<SS）大小 32 byte
#define SIZE_OF_SONAR_PROBE_LEN     sizeof(RECV_UPPER_SONAR_PROBE_CONFIG)   //显控下发探头配置的长度
#define SIZE_OF_IP_CONFIG_LEN       sizeof(SEND_UPPER_IP_CONFIG)            //ARM->显控：ip信息
#define SIZE_OF_CONFIG_PARA         sizeof(RECV_UPPER_CONFIG_PARAMETERS)    // 显控下发给ARM的参数配置数据包字节数  3264字节
#define SIZE_OF_LONG 4               // 数据尾所占字节大小

#define SIZE_OF_SENSOR 4             //传感器长度所占字节数
#define SIZE_OF_USHORT 2             // 校验位所占字节大小
#define SIZE_OF_TVG_MAX 8192         //8192         // 2664//干端下发的TVG数据的最大字节数2664=666*4
#define SIZE_OF_CHANNEL_NUM 96       //AD的IQ数据通道个数

#define SIZE_OF_SENSOR_FIRST 12      // ARM给干端上传的传感器前面的固定字节数是12，4字节的总字节数+4字节同步状态+4字节预留
#define SIZE_OF_SENSOR_TAIL   
#define SIZE_OF_CONFIG_CRC16 3268   //校验位
#define SIZE_OF_ONE_PING_SENSOR 256 // 一条传感器256字节

#define SAMPLE_FACTOR 17  //      //IQ抽样因子
#define AD_SAMPLE_FACTOR 4          //AD抽样因子
#define SAMPLE_FACTOR_8 8           // 原始数据的AD

#define SIZE_OF_DATA_FIRST 256           // FPGA给ARM上传的参数数据字节数
#define SIZE_OF_AD_SN_MAX 160020         // 160000补齐到35的倍数
#define SIZE_OF_AD_DATA_MAX_BYTE 3511296 // 最大字节数

#define V_SOUND 1500                                               // 声速1500m/s
#define DDR_TO_UP_HEAD      sizeof(SEND_UPPER_SONAR_FIRST)        // ARM发送给显控的参数数据长度

// #define   DDR_SONAR_DATA_OFFSET                 16
#define NUM_OF_IP 16                    // PWM初始相位通道数
#define CODE_VERSION    0x00000001      // FPGA版本信息
#define MASTER_VER      2026090901
#define SLAVE_VER       2026090902

extern uint8_t eeprom_map_flag;

extern void init_EPLD_MIO32_bit_low(void);
extern void init_EPLD_MIO33_bit_hign(void);
extern void init_EPLD_MIO28_bit_hign(void);

#endif //!_CUBEAMONE_H_

