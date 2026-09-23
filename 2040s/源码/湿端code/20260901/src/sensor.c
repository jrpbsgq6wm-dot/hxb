#include "beam.h"
#include "network.h"
#include "upper_to_fpga.h"
#include "fpga_init.h"
#include "fpga_to_upper.h"
#include "sensor.h"

/*
 * 功能: 初始化 TMP451 温度传感器 I2C 设备并配置从机地址。
 * 参数: 无。
 * 返回值: 成功返回 0；失败时打印错误并退出进程。
 */
static uint8 TMP451_Init(void)
{
    fd_icc = open("/dev/i2c-0", O_RDWR);   //允许读写 
    if(fd_icc < 0)
    {
        perror("Can't open /dev/i2c-0\n"); //打开iic设备文件失败
        exit(1);
    } 
    printf("open /dev/i2c success !\n");   //打开iic设备文件成功    
    if(ioctl(fd_icc, I2C_SLAVE, Address)<0)      //设置iic从器件地址
    {    
        printf("fail to set i2c device slave address!\n");
        close(fd_icc);
        return -1;
    }        
    printf("set slave address to 0x%x success!\n", Address);        
    return(1);
}

/*
 * 功能: 向 TMP451 指定寄存器写入 1 字节数据。
 * 参数: fd 为 I2C 设备文件描述符；reg 为寄存器地址；val 为待写入值。
 * 返回值: 成功返回 0；失败返回 -1。
 */
static uint8 i2c_write(int fd, uint8 reg, uint8 val)
{
    int retries;
    unsigned char data[2];
    data[0] = reg;
    data[1] = val;
    for(retries=5; retries; retries--) 
    {
        if(write(fd, data, 2)==2)
            return 0;
        usleep(1000*10);
    }
    return -1;
}

/*
 * 功能: 从 TMP451 指定寄存器读取 1 字节数据。
 * 参数: fd 为 I2C 设备文件描述符；reg 为寄存器地址；val 为读取结果保存地址。
 * 返回值: 成功返回 0；失败返回 -1。
 */
static uint8 i2c_read(int fd, uint8 reg, uint8 *val)
{
    int retries;
    for(retries=5; retries; retries--)
        if(write(fd, &reg, 1)==1)
            if(read(fd, val, 1)==1)
                return 0;
    return -1;
}

/*****************************************************************
 * 名称：                    com_open
 * 功能：                    打开串口并返回串口设备文件描述
 * 入口参数：            	 fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2)
 * 出口参数：            	正确返回为1，错误返回为0
 *****************************************************************/

int com_open(char* port)
{
    int fd;
    
    // 1. 打开串口（先用非阻塞模式打开）
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0)  //修复：检查 fd < 0
    {
        perror("Can't Open Serial Port");
        return FALSE;
    }
    printf("open success, fd=%d\n", fd);
    
    // 2. 恢复为阻塞模式
    if (fcntl(fd, F_SETFL, 0) < 0)
    {
        printf("fcntl failed!\n");
        close(fd);
        return FALSE;
    }
    printf("set to blocking mode\n");
    
    // 3. 检查是否是终端设备（检查 fd，不是 STDIN_FILENO）
    if (isatty(fd) == 0)  
    {
        printf("%s is not a terminal device\n", port);
        close(fd);
        return FALSE;
    }
    printf("is a terminal device\n");
    
    return fd;
}


/*****************************************************************
 * 名称：                    set_com_opt
 * 功能：                    串口初始化
 * 入口参数：            	 fd    :文件描述符     
 * 出口参数：            	正确返回为0
 *****************************************************************/

int set_com_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)   
{   
    struct termios newtio,oldtio;   
    /*保存测试现有串口参数设置，在这里如果串口号等出错，会有相关的出错信息*/   
    if  ( tcgetattr( fd,&oldtio)  !=  0) 
    {
        perror("SetupSerial 1");  
        printf("tcgetattr( fd,&oldtio) -> %d\n",tcgetattr( fd,&oldtio));   
        return -1;   
    }   
    bzero( &newtio, sizeof( newtio ) );   
    /*步骤一，设置字符大小*/   
    newtio.c_cflag  |=  CLOCAL | CREAD;    
    newtio.c_cflag &= ~CSIZE;    
    /*设置停止位*/   
    switch( nBits )   
    {   
        case 7:   
            newtio.c_cflag |= CS7;   
            break;   
        case 8:   
            newtio.c_cflag |= CS8;   
            break;   
    }   
    /*设置奇偶校验位*/   
    switch( nEvent )   
    {   
        case 'o':  
        case 'O': //奇数   
            newtio.c_cflag |= PARENB;   
            newtio.c_cflag |= PARODD;   
            newtio.c_iflag |= (INPCK | ISTRIP);   
            break;   
        case 'e':  
        case 'E': //偶数   
            newtio.c_iflag |= (INPCK | ISTRIP);   
            newtio.c_cflag |= PARENB;   
            newtio.c_cflag &= ~PARODD;   
            break;  
        case 'n':  
        case 'N':  //无奇偶校验位   
            newtio.c_cflag &= ~PARENB;   
            break;  
        default:  
            break;  
    }   
    /*设置波特率*/   
    switch( nSpeed )   
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
    if( nStop == 1 )   
        newtio.c_cflag &=  ~CSTOPB;   
    else if ( nStop == 2 )   
        newtio.c_cflag |=  CSTOPB;   
    /*设置等待时间和最小接收字符*/   
    newtio.c_cc[VTIME]  = 0;   
    newtio.c_cc[VMIN] = 0;   
    /*处理未接收字符*/   
    tcflush(fd,TCIFLUSH);   
    /*激活新配置*/   
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)   
    {   
        perror("com set error");   
        return -1;   
    }   
    printf("set done!\n");   
    return 0;   
}   

char sum_check(char *buf,int len)
{
    char sum;
    unsigned short sum_real=0;
    int i;
    for (i=1;i<len;i++) 
    {
        sum_real+=buf[i];
    }
    // printf("sum_real:%x \n",sum_real);
    sum=sum_real%0x100;
    // printf("sum:%x \n",sum);
    return sum;
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
    flag_timer=1;
    //printf("timer\n");
}
/********************************************************************************
 * 名称：                    change
 * 功能：                    判断变量的值是否发生变化
 * 入口参数：            	 无
 * 出口参数：            	 i
 *********************************************************************************/
int	change(void)
{
    int i;
    if(temprature1!=ptr_send_to_upper_package_first->temprature){
        i=0;
        return i;
    }
    else{		
        i=i+1;
        return i;
    }
}

/********************************************************************************
 * 名称：                    get_sensor_data
 * 功能：                    得到TMP451的值，罗经的值
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void get_sensor_data(void)
{
    write(fd_compass, CompassSendBuf, 5); //罗经--发送读取罗经中roll、pitch、heading的命令
    i2c_read(fd_icc, REG_REMOTE_HIGH,&remote_high_value);
    i2c_read(fd_icc, REG_REMOTE_LOW, &remote_low_value); //读取远程温度值-低位
    //i2c_read(fd_icc, STATUS_REG, &status_register);
    //Debug("status_register: %d ", status_register);
    ptr_send_to_upper_package_first->temprature = (float)((remote_high_value - 64)+(remote_low_value>>7)*0.5+pow(((remote_low_value>>6)&0x01)*0.5,2)+pow(((remote_low_value>>5)&0x01)*0.5,3)+pow(((remote_low_value>>4)&0x01)*0.5,4)); //计算温度值
    //    Debug("temprature1: %f ", ptr_send_to_upper_package_first->temprature);
    tem_num=change();
    //Debug("temprature1: %f ", temprature1);
    temprature1=ptr_send_to_upper_package_first->temprature;
    //	Debug("tem_num: %d ", tem_num);
    usleep(8000);//侧扫5毫秒可以---要不要时间长一些？？？
    nread_compass = read(fd_compass, CompassRecvBuf, 14);//罗经--读取罗经中roll、pitch、heading的命令
    if (nread_compass==14) //计算罗经的值
    {
        sum_calculate=sum_check(CompassRecvBuf,13);
        recv_sum=CompassRecvBuf[13];
        if (recv_sum!=sum_calculate)
        {
            printf("compass crc error! \n");
            printf("sum_calculate:%x\n",sum_calculate);
            printf("recv_sum:     %x\n",recv_sum);
        }  
        if (recv_sum==sum_calculate) 
        {
            if ((CompassRecvBuf[4]&0x10)==0x10)//负数
            {
                ptr_send_to_upper_package_first->pitch = (float)(-((CompassRecvBuf[4] & 0x0F) * 100 + ((CompassRecvBuf[5] & 0xF0) >> 4) * 10 + (CompassRecvBuf[5] & 0x0F) + (float)((CompassRecvBuf[6] & 0xF0) >> 4) / 10 + (float)(CompassRecvBuf[6] & 0x0F) / 100)); 
                Debug("pitch: %f ", ptr_send_to_upper_package_first->pitch); 
            }
            if ((CompassRecvBuf[4]&0x10)==0x00)//正数
            {
                ptr_send_to_upper_package_first->pitch=(float)((CompassRecvBuf[4] & 0x0F) * 100 + ((CompassRecvBuf[5]&0xF0)>>4)*10+(CompassRecvBuf[5]&0x0F) + (float)((CompassRecvBuf[6]&0xF0)>>4)/10+(float)(CompassRecvBuf[6]&0x0F) / 100);
                Debug("pitch: %f ",ptr_send_to_upper_package_first->pitch); 
            }
            if ((CompassRecvBuf[7]&0x10)==0x10)//负数
            {
                ptr_send_to_upper_package_first->roll = (float)(-((CompassRecvBuf[7] & 0x0F) * 100 + ((CompassRecvBuf[8] & 0xF0) >> 4) * 10 + (CompassRecvBuf[8] & 0x0F) + (float)((CompassRecvBuf[9] & 0xF0) >> 4) / 10 + (float)(CompassRecvBuf[9] & 0x0F) / 100));  
                Debug("roll: %f ", ptr_send_to_upper_package_first->roll); 
            }
            if ((CompassRecvBuf[7]&0x10)==0x00)//正数
            {
                ptr_send_to_upper_package_first->roll=(float)((CompassRecvBuf[7] & 0x0F) * 100 + ((CompassRecvBuf[8]&0xF0)>>4)*10+(CompassRecvBuf[8]&0x0F) + (float)((CompassRecvBuf[9]&0xF0)>>4)/10+(float)(CompassRecvBuf[9]&0x0F) / 100);
                Debug("roll: %f ", ptr_send_to_upper_package_first->roll); 
            }
            ptr_send_to_upper_package_first->heading = (float)((CompassRecvBuf[10] & 0x0F) * 100 + ((CompassRecvBuf[11] & 0xF0) >> 4) * 10 + (CompassRecvBuf[11] & 0x0F) + (float)((CompassRecvBuf[12] & 0xF0) >> 4) / 10 + (float)(CompassRecvBuf[12] & 0x0F) / 100); 
            //测试后，确定校准公式ptr_send_to_upper_package_sensor->heading =
            Debug("heading: %f\n", ptr_send_to_upper_package_first->heading); 
        }
    }
}


/********************************************************************************
 * 名称：                    read_TMP451_sensor_and_save
 * 功能：                    读取TMP451的值到指定的位置
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void read_TMP451_sensor_and_save(void)
{
    //初始化温度传感器的IIC接口
    TMP451_Init(); 
    i2c_write(fd_icc,REG_CONFIG_WRITE, 0x04);//温度范围是-64度到191度
    //signal(SIGALRM, timer); //relate the signal and function  
    // alarm(1);       //trigger the timer 
    setTimer(0, 500000);//500ms
    while (1)
    {   
        if(i2c_read_reg != 0)
        {
            TMP451_Init();
            i2c_read_reg=0;
        }
        else if ((flag_timer == 1) && (Fpga_start_mod==1)) 
        { 
            flag_timer=0;
            setTimer(0, 500000);//500ms	
            i2c_read_reg=i2c_read(fd_icc, REG_REMOTE_HIGH,&remote_high_value);//读取远程温度值-高位			
            //	Debug("i2c_read_reg: %d ", i2c_read_reg);
            get_sensor_data(); //从FPGA读取数据,需要知道GGA传感器协议。      
        }
        else
        {
            usleep(1000);
        }

		if(syncstatus == 1)
		{
			if(ptr_fpga_register_data->fpga_sta.wstatus == 1)
			{
				if (send(Connect_fd, "<<SY",4, 0) < 0) //4byte
				{   
					error_process();
					return;
				} 
			}
		}

	}
}

/********************************************************************************
 * 名称：                    timer
 * 功能：                    每秒打印中断的累积次数，间接反映ping率
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void timer(int sig)  
{
    if(SIGALRM == sig)  
    {  
        DBG("flag:%d\n",flag);
        alarm(1);       //we contimue set the timer  
    }  
} 
