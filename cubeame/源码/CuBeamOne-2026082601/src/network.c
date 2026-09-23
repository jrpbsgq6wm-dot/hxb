
#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"

/******************************************全局变量***************************************************/
int Socket_fd_server, Connect_fd, Socket_fd_client; // 文件描述符
extern int flag;
extern volatile int netStatus;
extern void clear_sensor_data(void);

/********************************************************************************
 * 名称：                    error_process
 * 功能：                    异常断网的处理机制（如上位机界面闪退或者异常关闭）
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void error_process(void)
{
    DBG("Network interrupt!.\n");
    ptr_fpga_register_data->wsm_con = 0;
    netStatus = 0; // 网络链接状态标志
    Fpga_start_mod = 0;
    flag = 0;
    close(Connect_fd); // 关闭释放网络描述符
    // close(Socket_fd_client);
    clear_sensor_data();
}

/********************************************************************************
 * 名称：                    setkeepalive
 * 功能：                    设置setkeepalive
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void setkeepalive(int lisfd, unsigned int begin, unsigned int cnt, unsigned int intvl)
{
    if (lisfd)
    {
        int keepalive = 1;
        if (setsockopt(lisfd, SOL_SOCKET, SO_KEEPALIVE, (const void *)&keepalive, sizeof(keepalive)) == -1)
        {
            fprintf(stderr, "SO_KEEPALIVE %s\n", strerror(errno)); // 开启调整keepalive的选项
        }
        if (setsockopt(lisfd, IPPROTO_TCP, TCP_KEEPIDLE, (const void *)&begin, sizeof(begin)) == -1)
        {
            fprintf(stderr, "TCP_KEEPIDLE %s\n", strerror(errno)); // 距离上次发送数据多长时间后开始探测
        }
        if (setsockopt(lisfd, IPPROTO_TCP, TCP_KEEPCNT, (const void *)&cnt, sizeof(cnt)) == -1)
        {
            fprintf(stderr, "TCP_KEEPCNT %s\n", strerror(errno)); // 探测没有回应要坚持多少次
        }
        if (setsockopt(lisfd, IPPROTO_TCP, TCP_KEEPINTVL, (const void *)&intvl, sizeof(intvl)) == -1)
        {
            fprintf(stderr, "TCP_KEEPINTVL %s\n", strerror(errno)); // 无数据交互下 每隔多长时间探测一次
        }
    }
}

/*****************************************************************
 * 名称：                    socket_server_init
 * 功能：                    初始化socket_server；网络初始化
 * 入口参数：            	 无
 * 出口参数：            	 无
 *****************************************************************/

void socket_server_init(void)
{
    // 初始化Socket
    if ((Socket_fd_server = socket(AF_INET, SOCK_STREAM, 0)) == -1) // IPV4,socket类型:SOCK_STREAM
    {
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    // 初始化  memset(&Servaddr_server, 0, sizeof(Servaddr_server));
    // setkeepalive(Socket_fd_server,5,1,5);
    Servaddr_server.sin_family = AF_INET;                  // 地址族
    Servaddr_server.sin_addr.s_addr = htonl(INADDR_ANY);   // IP地址设置成IP_ADDRESS(已在side-scan.h中进行宏定义)
    Servaddr_server.sin_port = htons(DEFAULT_PORT_SERVER); // 设置的端口为DEFAULT_PORT:8000(已在side-scan.h中进行宏定义)
    // 将本地地址绑定到所创建的套接字上
    if (bind(Socket_fd_server, (struct sockaddr *)&Servaddr_server, sizeof(Servaddr_server)) == -1)
    {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    // 开始监听是否有客户端连接
    if (listen(Socket_fd_server, 10) == -1) // 第二个参数为相应socket可以排队的最大连接个数
    {
        printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
}
