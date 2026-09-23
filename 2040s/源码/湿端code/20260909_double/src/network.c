#include "beam.h"
#include "network.h"
#include "upper_to_fpga.h"
#include <stddef.h>

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
            fprintf(stderr, "TCP_KEEPIDLE %s\n", strerror(errno));     //距离上次发送数据多长时间后开始探测
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

/*****************************************************************
 * 名称：                    init_socket_server
 * 功能：                    初始化socket_server
 * 入口参数：            	 无
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
 * 名称：                    crc16
 * 功能：                    计算两个字节的CRC
 * 入口参数：            	 *data：数组的起始地址  len：数组的长度  crc：crc的初值
 * 出口参数：            	 两字节的crc
 *****************************************************************/

unsigned int recv_socket(int fd, char *buf, unsigned int len)
{
    unsigned int total = 0;

    while (total < len) {
        ssize_t n = recv(fd, buf + total, len - total, 0);

        if (n == 0) {
            return 0;
        }

        if (n < 0) {
            return -1;
        }

        total += (unsigned int)n;
    }

    return (int)total;
}

/********************************************************************************
 * 名称：                    send_status_to_upper
 * 功能：                    ARM向上位机发送硬件状态信息
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void send_status_to_upper(void)
{
    size_t status_date_offset;
    volatile unsigned int *status_date_reg;
    unsigned int fpga_version;
	char   DataHead[4]={'<','<','S','S'};// 数据头标示“<<SS”,表示数据头 
	ptr_send_upper_status_information->FPGAVersion = ptr_fpga_register_data->fpga_sta.date; 
	ptr_send_upper_status_information->LinuxDriverVersion = LINUX_VERSION;
	ptr_send_upper_status_information->PulseWidth = ptr_send_to_upper_package_first->PWMPulseWidth; 
	ptr_send_upper_status_information->Range = ptr_send_to_upper_package_first->Range;
	ptr_send_upper_status_information->SignalType = (fpga_register_data.fpga_registers_200k.wsm_registers_value.wsm_mod & 0x00000001);
	ptr_send_upper_status_information->power_factor = ptr_send_to_upper_package_first->power_factor;
	ptr_send_upper_status_information->EPLD_VERSIONS=(unsigned int)0;
    ptr_send_upper_status_information->device_type = (unsigned short)1004;
	char *ptr_to_send;
	ptr_to_send=(char *)ptr_send_upper_status_information;
	if (send(Connect_fd, DataHead, SIZE_OF_LONG, 0) < 0) //4byte
	{       
		error_process();
		return; 
	} 
	if (send(Connect_fd,  ptr_to_send, SIZE_OF_STATUS_SEND, 0) < 0) //68byte
	{       
		error_process();
		return; 
	}  
	if (send(Connect_fd, DataTail_S, SIZE_OF_LONG, 0) < 0) //4byte
	{  
		error_process();
		return;
	}
	printf("ok\n");
}
