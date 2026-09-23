
#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"

int flag_timer = 0; // 温度传感器TMP451的定时器
int tem_num = 1;    // 当温度不变化时进行累加，累加的值

float temprature1 = 0;
unsigned char status_register;
unsigned char local_high_value, local_low_value, remote_high_value, remote_low_value; // 读出温度的整数位
int fd_icc;

typedef unsigned char uint8;
uint8 i2c_read_reg = 0;
static const int multiplier = 1 << 12;

extern SEND_UPPER_SONAR_FIRST *ptr_send_to_upper_sonar_first;

/**************************************罗经传感器相关**************************************/

int fd_compass;
char CompassSendBuf[5] = {0x68, 0x04, 0x00, 0x04, 0x08}; // 罗经发送buffer
char CompassRecvBuf[14];                                 // 罗经接收buffer
char sum_calculate, recv_sum;
int nread_compass;
COM_CONFIG_PARAMETER com_compass = {.dev = "/dev/ttyPS1", .nSpeed = 9600, .nBits = 8, .nEvent = 'N', .nStop = 1}; // 罗经串口

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

/************************************************************************************************
 *
 * 函数名称 :                sum_check
 * 函数描述 :                和校验
 * 函数返回值 :              校验值
 * 函数修改人:               fuyanshun
 * 修改备注:                 本函数没有使用过
 *
 *************************************************************************************************/
char sum_check(char *buf, int len)
{
    char sum;
    unsigned short sum_real = 0;
    int i;
    for (i = 1; i < len; i++)
    {
        sum_real += buf[i];
    }
    // printf("sum_real:%x \n",sum_real);
    sum = sum_real % 0x100;
    // printf("sum:%x \n",sum);
    return sum;
}

/*****************************************************************
 * 名称：                    set_com_opt
 * 功能：                    串口初始化
 * 入口参数：            	 fd    :文件描述符
 * 出口参数：            	正确返回为0
 *****************************************************************/

int set_com_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio, oldtio;
    /*保存测试现有串口参数设置，在这里如果串口号等出错，会有相关的出错信息*/
    if (tcgetattr(fd, &oldtio) != 0)
    {
        perror("SetupSerial 1");
        printf("tcgetattr( fd,&oldtio) -> %d\n", tcgetattr(fd, &oldtio));
        return -1;
    }
    bzero(&newtio, sizeof(newtio));
    /*步骤一，设置字符大小*/
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;
    /*设置停止位*/
    switch (nBits)
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }
    /*设置奇偶校验位*/
    switch (nEvent)
    {
    case 'o':
    case 'O': // 奇数
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'e':
    case 'E': // 偶数
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'n':
    case 'N': // 无奇偶校验位
        newtio.c_cflag &= ~PARENB;
        break;
    default:
        break;
    }
    /*设置波特率*/
    switch (nSpeed)
    {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    case 460800:
        cfsetispeed(&newtio, B460800);
        cfsetospeed(&newtio, B460800);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }
    /*设置停止位*/
    if (nStop == 1)
        newtio.c_cflag &= ~CSTOPB;
    else if (nStop == 2)
        newtio.c_cflag |= CSTOPB;
    /*设置等待时间和最小接收字符*/
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;
    /*处理未接收字符*/
    tcflush(fd, TCIFLUSH);
    /*激活新配置*/
    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)
    {
        perror("com set error");
        return -1;
    }
    printf("set done!\n");
    return 0;
}
