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

// #define DEBUG
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

/*************************外部引用变量*************************************/
extern int irq_on;
extern pthread_mutex_t mut;

/*************************大小定义*************************************/
#define SIZE_OF_LONG 4               // 数据尾所占字节大小
#define SIZE_OF_SENSOR 4             //传感器长度所占字节数
#define SIZE_OF_USHORT 2             // 校验位所占字节大小
#define SIZE_OF_TVG_MAX 8192//8192         // 2664//干端下发的TVG数据的最大字节数2664=666*4
#define SIZE_OF_CHANNEL_NUM 48       // 48 //AD的IQ数据通道个数
#define SIZE_OF_STATUS_SEND 54       // ARM发送给干端的硬件状态信息共计68字节
#define SIZE_OF_SENSOR_FIRST 16      // ARM给干端上传的传感器前面的固定字节数是16，4字节的总字节数+4字节同步状态+4字节预留+4字节传感器条数
#define SIZE_OF_CONFIG_PARA 3268    // 显控下发给ARM的参数配置数据包字节数  3264字节
#define SIZE_OF_CONFIG_CRC16 3268   //校验位
#define SIZE_OF_ONE_PING_SENSOR 1024 // 一帧传感器包邮1024字节

#define SAMPLE_FACTOR 35  // 23  //IQ抽样因子
#define AD_SAMPLE_FACTOR 4      //AD抽样因子
#define SAMPLE_FACTOR_8 8 // 原始数据的AD

#define SIZE_OF_DATA_FIRST 256           // FPGA给ARM上传的参数数据字节数
#define SIZE_OF_AD_SN_MAX 160020         // 160000补齐到35的倍数
#define SIZE_OF_AD_DATA_MAX_BYTE 3511296 // 最大字节数

#define V_SOUND 1500       // 声速1500m/s
#define DDR_TO_UP_HEAD 128 // ARM发送给显控的DDR_TO_UP_HEAD，有128字节

// #define   DDR_SONAR_DATA_OFFSET                 16
#define NUM_OF_IP 16             // PWM初始相位通道数
#define CODE_VERSION 0x00000001  // FPGA版本信息
#define LINUX_VERSION 2026082601 // ARM版本信息

extern uint8_t eeprom_map_flag;

extern void init_EPLD_MIO32_bit_low(void);
extern void init_EPLD_MIO33_bit_hign(void);
extern void init_EPLD_MIO28_bit_hign(void);

#endif //!_CUBEAMONE_H_
