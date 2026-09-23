
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

/******************************************函数声明***************************************************/
static uint8 TMP451_Init(void);
static void get_sensor_data(void);
static void setTimer(int seconds, int mseconds);
static uint8 i2c_write(int fd, uint8 reg, uint8 val);
static uint8 i2c_read(int fd, uint8 reg, uint8 *val);

/********************************************************************************
 * 名称：                    read_TMP451_sensor_and_save
 * 功能：                    读取TMP451的值到指定的位置
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void read_TMP451_sensor_and_save(void)
{
    // 初始化姿态传感器的串口
    // fd_compass = com_open(com_compass.dev);
    // set_com_opt(fd_compass,  com_compass.nSpeed,  com_compass.nBits,  com_compass.nEvent, com_compass.nStop);
    // 初始化温度传感器的IIC接口
    TMP451_Init();
    i2c_write(fd_icc, REG_CONFIG_WRITE, 0x04); // 温度范围是-64度到191度
    // signal(SIGALRM, timer); //relate the signal and function
    // alarm(1);       //trigger the timer
    setTimer(0, 500000); // 500ms
    while (1)
    {
        if (i2c_read_reg != 0)
        {
            TMP451_Init();
            i2c_read_reg = 0;
        }
        else if ((flag_timer == 1) && (Fpga_start_mod == 1))
        {
            flag_timer = 0;
            setTimer(0, 500000);                                                  // 500ms
            i2c_read_reg = i2c_read(fd_icc, REG_REMOTE_HIGH, &remote_high_value); // 读取远程温度值-高位
            // Debug("i2c_read_reg: %d ", i2c_read_reg);
            get_sensor_data(); // 从FPGA读取数据,需要知道GGA传感器协议。
        }
        else
        {
            usleep(1000);
        }
    }
}

/*****************************************************************
 * 名称：                    com_open
 * 功能：                    打开串口并返回串口设备文件描述
 * 入口参数：            	 fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2)
 * 出口参数：            	正确返回为1，错误返回为0
 *****************************************************************/

int com_open(char *port)
{
    int fd;
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY); // 打开和创建文件,O_NOCTTY如果路径名指向终端设备，不要把这个设备用作控制终端。
    if (FALSE == fd)
    {
        perror("Can't Open Serial Port");
        return (FALSE);
    }
    // 判断串口的状态是否为阻塞状态,恢复串口为阻塞状态
    if (fcntl(fd, F_SETFL, 0) < 0)
    {
        printf("fcntl failed!/n");
        return (FALSE);
    }
    else
    {
        printf("fcntl=%d\n", fcntl(fd, F_SETFL, 0));
    }
    // 测试是否为终端设备
    if (0 == isatty(STDIN_FILENO))
    {
        printf("standard input is not a terminal device\n");
        return (FALSE);
    }
    else
    {
        printf("isatty success!\n");
    }
    printf("fd->open=%d\n", fd);
    return fd;
}

/********************************************************************************
 * 名称：                    TMP451_Init
 * 功能：                    TMP451温度传感器初始化
 * 入口参数：            	 无
 * 出口参数：            	 uint8
 *********************************************************************************/
static uint8 TMP451_Init(void)
{
    fd_icc = open("/dev/i2c-0", O_RDWR); // 允许读写
    if (fd_icc < 0)
    {
        perror("Can't open /dev/i2c-0\n"); // 打开iic设备文件失败
        exit(1);
    }
    // printf("open /dev/i2c success !\n");       // 打开iic设备文件成功
    if (ioctl(fd_icc, I2C_SLAVE, Address) < 0) // 设置iic从器件地址
    {
        printf("fail to set i2c device slave address!\n");
        close(fd_icc);
        return -1;
    }
    // printf("set slave address to 0x%x success!\n", Address);
    return (1);
}

/********************************************************************************
 * 名称：                    i2c_write
 * 功能：                    icc写入一字节
 * 入口参数：            	 无
 * 出口参数：            	 uint8
 *********************************************************************************/
static uint8 i2c_write(int fd, uint8 reg, uint8 val)
{
    int retries;
    unsigned char data[2];
    data[0] = reg;
    data[1] = val;
    for (retries = 5; retries; retries--)
    {
        if (write(fd, data, 2) == 2)
            return 0;
        usleep(1000 * 10);
    }
    return -1;
}

/********************************************************************************
 * 名称：                    i2c_read
 * 功能：                    icc读取一字节
 * 入口参数：            	 无
 * 出口参数：            	 uint8
 *********************************************************************************/
static uint8 i2c_read(int fd, uint8 reg, uint8 *val)
{
    int retries;
    for (retries = 5; retries; retries--) 
        if (write(fd, &reg, 1) == 1)    //写入要读取的数据的地址，0x01为还高地址，0x10为低地址  
            if (read(fd, val, 1) == 1)
                return 0;
    return -1;
}

/********************************************************************************
 * 名称：                    get_sensor_data
 * 功能：                    得到TMP451的值，罗经的值
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void get_sensor_data(void)
{
    // write(fd_compass, CompassSendBuf, 5); //罗经--发送读取罗经中roll、pitch、heading的命令
    i2c_read(fd_icc, REG_REMOTE_HIGH, &remote_high_value);
    i2c_read(fd_icc, REG_REMOTE_LOW, &remote_low_value); // 读取远程温度值-低位
    // i2c_read(fd_icc, STATUS_REG, &status_register);
    // Debug("status_register: %d ", status_register);
    ptr_send_to_upper_sonar_first->UP_TMP451 = (float)((remote_high_value - 64) + (remote_low_value >> 7) * 0.5 + pow(((remote_low_value >> 6) & 0x01) * 0.5, 2) + pow(((remote_low_value >> 5) & 0x01) * 0.5, 3) + pow(((remote_low_value >> 4) & 0x01) * 0.5, 4)); // 计算温度值
}

/********************************************************************************
 * 名称：                    setTimer
 * 功能：                    设置定时器到时间标志位置1
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void setTimer(int seconds, int mseconds)
{
    struct timeval temp;
    temp.tv_sec = seconds;
    temp.tv_usec = mseconds;
    select(0, NULL, NULL, NULL, &temp);
    flag_timer = 1;
    // printf("timer\n");
}

/********************************************************************************
 * 名称：                    change
 * 功能：                    判断变量的值是否发生变化
 * 入口参数：            	 无
 * 出口参数：            	 i
 *********************************************************************************/
int change(void)
{
    int i;
    if (temprature1 != ptr_send_to_upper_sonar_first->UP_TMP451)
    {
        i = 0;
        return i;
    }
    else
    {
        i = i + 1;
        return i;
    }
}

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
