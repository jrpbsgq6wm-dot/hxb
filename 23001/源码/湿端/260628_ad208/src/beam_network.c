#include "beam.h"

/********************************************************************************
 * 模块：网络连接、断网处理与socket发送
 * 说明：由原 beam.c 按功能拆分，函数体保持原有逻辑。
 ********************************************************************************/


/*****************************************************************
 * * 名称：                    setkeepalive
 * * 功能：                    设置keepalive
 * * 入口参数：                     无
 * * 出口参数：                     无
 * *****************************************************************/
void setkeepalive(int lisfd, unsigned int begin, unsigned int cnt, unsigned int intvl)
{
    if(lisfd){
        int keepalive = 1;
        if(setsockopt(lisfd, SOL_SOCKET, SO_KEEPALIVE,(const void *)&keepalive, sizeof(keepalive)) == -1)
        {
            fprintf(stderr, "SO_KEEPALIVE %s\n", strerror(errno));//开启调整keepalive的选项
        }

        if(setsockopt(lisfd, IPPROTO_TCP, TCP_KEEPIDLE, (const void *)&begin, sizeof(begin)) == -1)
        {
            fprintf(stderr, "TCP_KEEPIDLE %s\n", strerror(errno)); //距离上次发送数据多长时间后开始探测
        }

        if(setsockopt(lisfd, IPPROTO_TCP, TCP_KEEPCNT, (const void *)&cnt, sizeof(cnt))==-1)
        {
            fprintf(stderr, "TCP_KEEPCNT %s\n", strerror(errno));//探测没有回应要坚持多少次
        }

        if(setsockopt(lisfd, IPPROTO_TCP, TCP_KEEPINTVL, (const void *)&intvl, sizeof(intvl))==-1)
        {
            fprintf(stderr, "TCP_KEEPINTVL %s\n", strerror(errno));//无数据交互下 每隔多长时间探测一次

        }

    }
}

void set_send_timeout(int fd, unsigned int seconds)
{
    if (fd > 0)
    {
        struct timeval timeout;
        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const void *)&timeout, sizeof(timeout)) == -1)
        {
            fprintf(stderr, "SO_SNDTIMEO %s\n", strerror(errno));
        }
    }
}

/*****************************************************************
 * 名称：                    init_socket_server
 * 功能：                    初始化socket_server
 * 入口参数：            	 无 8000端口
 * 出口参数：            	 无
 *****************************************************************/
void init_socket_server(void)
{
    //初始化Socket 
    if( (Socket_fd_server= socket(AF_INET, SOCK_STREAM, 0)) == -1 )//IPV4,socket类型:SOCK_STREAM
    {  
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  
    setkeepalive(Socket_fd_server,5,1,5);
    //初始化  memset(&Servaddr_server, 0, sizeof(Servaddr_server));  
    Servaddr_server.sin_family = AF_INET;  // 地址族 
    Servaddr_server.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成IP_ADDRESS(已在side-scan.h中进行宏定义)  
    Servaddr_server.sin_port = htons(DEFAULT_PORT_SERVER);//设置的端口为DEFAULT_PORT:8000(已在side-scan.h中进行宏定义)
    //将本地地址绑定到所创建的套接字上  
    if (bind(Socket_fd_server, (struct sockaddr *)&Servaddr_server, sizeof(Servaddr_server)) == -1) 
    {  
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }    	 	
    //开始监听是否有客户端连接  
    if( listen(Socket_fd_server, 10) == -1)//第二个参数为相应socket可以排队的最大连接个数
    {  
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
	} 
}

/*****************************************************************
 * 名称：                    init_socket_server
 * 功能：                    初始化socket_server
 * 入口参数：            	 无 8001端口
 * 出口参数：            	 无
 *****************************************************************/
void init_socket_server_8001(void)
{
    //初始化Socket 8001端口
    if( (Socket_fd_server_8001= socket(AF_INET, SOCK_STREAM, 0)) == -1 )//IPV4,socket类型:SOCK_STREAM
    {  
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  
	int reuse = 1;
    if (setsockopt(Socket_fd_server_8001, SOL_SOCKET, SO_REUSEADDR, 
                   &reuse, sizeof(reuse)) == -1) {
        printf("setsockopt SO_REUSEADDR error: %s\n", strerror(errno));
    }
	//设置端口复用
    setkeepalive(Socket_fd_server_8001,5,1,5);
    //初始化  memset(&Servaddr_server, 0, sizeof(Servaddr_server));  
    Servaddr_server_8001.sin_family = AF_INET;  // 地址族 
    Servaddr_server_8001.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成IP_ADDRESS(已在side-scan.h中进行宏定义)      
    Servaddr_server_8001.sin_port = htons(PORT_SERVER);//设置的端口为8001
    //将本地地址绑定到所创建的套接字上  
    if (bind(Socket_fd_server_8001, (struct sockaddr *)&Servaddr_server_8001, sizeof(Servaddr_server_8001)) == -1) 
    {  
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }    	 	
    //开始监听是否有客户端连接  
    if( listen(Socket_fd_server_8001, 10) == -1)//第二个参数为相应socket可以排队的最大连接个数
    {  
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
	} 
}

/********************************************************************************
 * 名称：                    error_process
 * 功能：                    异常断网的处理机制（如上位机界面闪退或者异常关闭）
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void error_process(void)
{
    printf("Network interrupt!.\n");
    netStatus = 0;
    ptr_fpga_register_data->wsm_con=0;
    // Send_ss_status=0;
    Fpga_start_mod = 0;
    //Send_data_status=0;
    flag=0;
    close(Connect_fd);
    Connect_fd = -1;
    clear_sensor_data();
}

/********************************************************************************
 * 名称：                    error_process
 * 功能：                    异常断网的处理机制8001端口（如上位机界面闪退或者异常关闭）
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void error_process_8001(void)
{
	DBG("send data to 8001 error\n");
    netStatus_8001 = 0;
	send_to_8001_flag = 0;
    //ptr_fpga_register_data->wsm_con=0;
    // Send_ss_status=0;
    //Fpga_start_mod = 0;
    //Send_data_status=0;
    //flag=0;
    close(Connect_fd_8001);
	Connect_fd_8001 = -1;
    // 8001 is auxiliary; do not clear shared acquisition data here.
}

/********************************************************************************
 * 名称：                    recv_socket
 * 功能：                    socket接收函数（接收socket长度很大时用此函数能保证接受全）
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
int recv_socket(int fd, char *buf, unsigned int len)
{
	unsigned int left = len;
	char *ptr = buf;

	while (left > 0)
	{
		ssize_t received = recv(fd, ptr, left, 0);
		if (received < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			return -1;
		}
		if (received == 0)
		{
			return -1;
		}
		ptr += received;
		left -= (unsigned int)received;
	}

	return (int)len;
}
int send_all_bytes(int fd, const void *buf, unsigned int len)
{
	const char *ptr = (const char *)buf;
	while (len > 0)
	{
		ssize_t sent = send(fd, ptr, len, MSG_NOSIGNAL);
		if (sent < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			return -1;
		}
		if (sent == 0)
		{
			return -1;
		}
		ptr += sent;
		len -= (unsigned int)sent;
	}
	return 0;
}

int send_locked_bytes(int fd, const void *buf, unsigned int len)
{
	int ret;
	pthread_mutex_lock(&mut);
	ret = send_all_bytes(fd, buf, len);
	pthread_mutex_unlock(&mut);
	return ret;
}


/********************************************************************************
 * 名称：                    send_status_to_upper
 * 功能：                    ARM向上位机发送硬件状态信息
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void send_status_to_upper(void)
{
	char   DataHead[4]={'<','<','S','S'};// 数据头标示“<<SS”,表示数据头 
	ptr_send_upper_status_information->FPGAVersion = ptr_fpga_register_data->fpga_sta.date; 
	ptr_send_upper_status_information->LinuxDriverVersion = LINUX_VERSION;
	ptr_send_upper_status_information->PulseWidth = ptr_send_to_upper_package_first->PWMPulseWidth; 
	ptr_send_upper_status_information->Range = ptr_send_to_upper_package_first->Range;
	ptr_send_upper_status_information->SignalType = (fpga_register_data.fpga_registers_200k.wsm_registers_value.wsm_mod & 0x00000001);
	ptr_send_upper_status_information->power_factor = ptr_send_to_upper_package_first->power_factor;
	memcpy(ptr_fpga_frame_first, uio_share_mem_IQ.mem_ptr, SIZE_OF_DATA_FIRST);
	ptr_send_upper_status_information->EPLD_VERSIONS=(unsigned int)ptr_fpga_frame_first->EPLD_VERSIONS;
	char *ptr_to_send;
	ptr_to_send=(char *)ptr_send_upper_status_information;
	if (send_all_bytes(Connect_fd, DataHead, SIZE_OF_LONG) < 0) //4byte
	{       
		error_process();
		return; 
	} 
	if (send_all_bytes(Connect_fd, ptr_to_send, SIZE_OF_STATUS_SEND) < 0) 
	{       
		error_process();
		return; 
	}  
	if (send_all_bytes(Connect_fd, DataTail_S, SIZE_OF_LONG) < 0) //4byte
	{  
		error_process();
		return;
	}
	printf("Send ok\r\n");
}


