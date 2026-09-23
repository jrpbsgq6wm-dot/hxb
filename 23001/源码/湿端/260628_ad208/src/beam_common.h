#ifndef BEAM_COMMON_H
#define BEAM_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/stat.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>

#include "beam_types.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* TCP 端口配置：8000 为主显控口，8001 为原始数据辅助口。 */
#define DEFAULT_PORT_SERVER        8000
#define PORT_SERVER                8001
#define DEFAULT_PORT_CLIENT        8000

/* FPGA 工作参数与寄存器计算常量。 */
#define FPGA_CLK_FREQUENCY         100000000
#define PWM_LFM_FACTOR             3602879702UL
#define PWM_CW_FACTOR              42.94967296
#define N2_32                      4294967296
#define N2_23                      8388608
#define ENABLE                     1
#define DISABLE                    0
#define TRUE                       0
#define FALSE                      -1
#define BAUD                       38400

/* TMP451/I2C 传感器寄存器配置。 */
#define Address                    0x4C
#define I2C_SLAVE                  0x0703
#define REG_LOCAL_HIGH             0x00
#define REG_REMOTE_HIGH            0x01
#define REG_LOCAL_LOW              0x15
#define REG_REMOTE_LOW             0x10
#define STATUS_REG                 0x02
#define REG_CONFIG_WRITE           0x09

typedef unsigned char uint8;

/* GPIO sysfs 配置。 */
#define SYSFS_GPIO_DIR             "/sys/class/gpio"
#define MAX_BUF                    64

#ifdef DEBUG
#define DBG(...) fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)
#define Debug(fmt,...) printf(fmt,##__VA_ARGS__)
#else
#define DBG(...)
#define Debug(fmt,...)
#endif

#endif /* BEAM_COMMON_H */