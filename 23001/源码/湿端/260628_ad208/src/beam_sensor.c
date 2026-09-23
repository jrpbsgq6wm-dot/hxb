#include "beam.h"

/********************************************************************************
 * 模块：传感器缓存、TMP451和串口采集
 * 说明：由原 beam.c 按功能拆分，函数体保持原有逻辑。
 ********************************************************************************/

/********************************************************************************
 * 名称：                    clear_sensor_data
 * 功能：                    清传感器数据（断开时需要清传感器缓存）
 * 入口参数：            	 无
 * 出口参数：            	 无
 * 修改记录：
 * 		250928 内存越界检查
 *********************************************************************************/
void clear_sensor_data(void)
{
/*---------------------------------------------------------------  内存越界检查_0928_HXB ------------*/	
    int m;
	// 1.指针检查
	if (!uio_sensor_mem_0.mem_ptr || !uio_sensor_mem_1.mem_ptr ||
		!uio_sensor_mem_2.mem_ptr || !uio_sensor_mem_3.mem_ptr ||
		!uio_sensor_mem_4.mem_ptr || !uio_sensor_mem_8.mem_ptr ||
		!uio_sensor_mem_9.mem_ptr || !uio_sensor_mem_A.mem_ptr ||
		!uio_sensor_mem_B.mem_ptr || !uio_sensor_mem_C.mem_ptr) {
		Debug("clear_sensor_data error: NULL pointer detected\n");
		return ;
	}

    for (m=0;m<=50;m++)
    {
		

		size_t offset1 = m * 64;
        size_t offset2 = offset1 + 1;

        // 内存偏移检查
        if (offset2 >= 0x100000) {
            Debug("clear_sensor_data warning: offset out of range at m=%d\n", m);
            break;
        }
/*--------------------------------------------------------------------------------------------------- */
/*
		内存由FPGA维护，以字为单位进行划分，占用两个字节
		所以在清空传感器数据的时候要进行两个字节的清空
		清空就是对FPGA_DDR_FIRST->FrameHead,清空
*/
        *(uio_sensor_mem_0.mem_ptr + offset1) = 0;
        *(uio_sensor_mem_0.mem_ptr + offset2) = 0;
        
        *(uio_sensor_mem_1.mem_ptr + offset1) = 0;
        *(uio_sensor_mem_1.mem_ptr + offset2) = 0;
        
        *(uio_sensor_mem_2.mem_ptr + offset1) = 0;
        *(uio_sensor_mem_2.mem_ptr + offset2) = 0;
        
        *(uio_sensor_mem_3.mem_ptr + offset1) = 0;
        *(uio_sensor_mem_3.mem_ptr + offset2) = 0;
        
        *(uio_sensor_mem_4.mem_ptr + offset1) = 0;
        *(uio_sensor_mem_4.mem_ptr + offset2) = 0;
        
        *(uio_sensor_mem_8.mem_ptr + offset1) = 0;
        *(uio_sensor_mem_8.mem_ptr + offset2) = 0;
        
        *(uio_sensor_mem_9.mem_ptr + offset1) = 0;
        *(uio_sensor_mem_9.mem_ptr + offset2) = 0;
        
        *(uio_sensor_mem_A.mem_ptr + offset1) = 0;
        *(uio_sensor_mem_A.mem_ptr + offset2) = 0;
        
        *(uio_sensor_mem_B.mem_ptr + offset1) = 0;
        *(uio_sensor_mem_B.mem_ptr + offset2) = 0;
        
        *(uio_sensor_mem_C.mem_ptr + offset1) = 0;
        *(uio_sensor_mem_C.mem_ptr + offset2) = 0;
    }
}


static uint8 TMP451_Init(void)
{
	fd_icc = open("/dev/i2c-0", O_RDWR);   //允许读写 
	if(fd_icc < 0)
	{
		perror("Can't open /dev/i2c-0\n"); //打开iic设备文件失败
		exit(1);
	}   
	if(ioctl(fd_icc, I2C_SLAVE, Address)<0)      //设置iic从器件地址
	{    
		printf("fail to set i2c device slave address!\n");
		close(fd_icc);
		return -1;
	}               
	return(1);
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
	for(retries=5; retries; retries--) 
	{
		if(write(fd, data, 2)==2){
			return 0;
		}
		usleep(1000*10);
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
	for(retries=5; retries; retries--){
		if(write(fd, &reg, 1)==1){
			if(read(fd, val, 1)==1){
				return 0;
			}
		}
	}
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
	fd = open( port, O_RDWR|O_NOCTTY|O_NDELAY);//打开和创建文件,O_NOCTTY如果路径名指向终端设备，不要把这个设备用作控制终端。
	if (FALSE == fd)
	{
		perror("Can't Open Serial Port");
		return(FALSE);
	}
	//判断串口的状态是否为阻塞状态,恢复串口为阻塞状态                         
	if(fcntl(fd, F_SETFL, 0) < 0)
	{
		printf("fcntl failed!/n");
		return(FALSE);
	} 
	else
	{
		printf("fcntl=%d\n",fcntl(fd, F_SETFL,0));
	}
	//测试是否为终端设备    
	if(0 == isatty(STDIN_FILENO))
	{
		printf("standard input is not a terminal device\n");
		return(FALSE);
	}
	else
	{
		printf("isatty success!\n");
	}       
	printf("fd->open=%d\n",fd);
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
	{
		cfsetispeed(&newtio, B2400);   
		cfsetospeed(&newtio, B2400);   
		break;   
	}
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
	if( nStop == 1 ){ 
		newtio.c_cflag &=  ~CSTOPB;   
	}else if ( nStop == 2 ){   
		newtio.c_cflag |=  CSTOPB; 
	}  
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
	sum=sum_real%0x100;
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
	ptr_send_to_upper_package_first->temprature = (float)((remote_high_value - 64)+(remote_low_value>>7)*0.5+pow(((remote_low_value>>6)&0x01)*0.5,2)+pow(((remote_low_value>>5)&0x01)*0.5,3)+pow(((remote_low_value>>4)&0x01)*0.5,4)); //计算温度值
	tem_num=change();
	temprature1=ptr_send_to_upper_package_first->temprature;
	usleep(8000);//侧扫5毫秒可以---要不要时间长一些
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
	//初始化姿态传感器的串口
	sleep(3);
//	fd_compass = com_open(com_compass.dev); 
//	set_com_opt(fd_compass,  com_compass.nSpeed,  com_compass.nBits,  com_compass.nEvent, com_compass.nStop);
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
				if (send_locked_bytes(Connect_fd, "<<SY", 4) < 0) //4byte
				{   
					error_process();
					return;
				} 
			}
		}
		sleep(3);

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

