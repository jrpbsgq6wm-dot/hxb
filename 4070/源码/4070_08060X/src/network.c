
#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"
#include "net/if.h"
#include "sys/ioctl.h"

/******************************************全局变量***************************************************/
int Socket_fd_server, Connect_fd, Socket_fd_client; // 文件描述符
extern int flag;
extern volatile int netStatus;
extern void clear_sensor_data(void);
struct sockaddr_in Servaddr_server, addr_client;
char upper_ip[15];  //存放已经建立TCP连接的IP
/********************************************************************************
 * 名称：                    error_process
 * 功能：                    异常断网的处理机制（如上位机界面闪退或者异常关闭）
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void error_process(void)
{
    ptr_fpga_register_data->wsm_con = 0;
    netStatus = 0; // 网络链接状态标志
    Fpga_start_mod = 0;
    flag = 0;
    close(Connect_fd); // 关闭释放网络描述符
    clear_sensor_data();
}

/********************************************************************************
 * 名称：                    setkeepalive
 * 功能：                    设置setkeepalive
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void setkeepalive(int client_fd)
{
    if (client_fd)
    {
         // 1. 设置发送/接收缓冲区大小
        int send_buf = 5 * 1024 * 1024;   // 1MB
        int recv_buf = 5 * 1024 * 1024;   // 1MB
        
        if (setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf)) < 0) {
            perror("设置 SO_SNDBUF 失败");
        }
        if (setsockopt(client_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf, sizeof(recv_buf)) < 0) {
            perror("设置 SO_RCVBUF 失败");
        }
        
        // 2. 开启 KeepAlive
        int keepalive = 1;
        setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
        
        // 3. 设置 KeepAlive 参数
        int keepidle = 5;     // 5秒空闲后开始探测
        int keepintvl = 2;    // 探测间隔2秒
        int keepcnt = 3;      // 探测3次后断开
        setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
        setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));
        setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
        
        // 4. 关闭 Nagle 算法（小数据包立即发送）
        int nodelay = 1;
        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
        
        // 5. 可选：设置 TCP 快速确认
        int quickack = 1;
        setsockopt(client_fd, IPPROTO_TCP, TCP_QUICKACK, &quickack, sizeof(quickack));
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
    //setkeepalive(Socket_fd_server);
    // 初始化  memset(&Servaddr_server, 0, sizeof(Servaddr_server));
    Servaddr_server.sin_family = AF_INET;                  // 地址族
    Servaddr_server.sin_addr.s_addr = htonl(INADDR_ANY);   // IP地址设置成IP_ADDRESS(已在side-scan.h中进行宏定义)
    Servaddr_server.sin_port = htons(DEFAULT_PORT_SERVER); // 设置端口
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

/*****************************************************************
 * 名称：                    Alter_IPaddress
 * 功能：                    修改IP地址 （备份 修改interface文件） 
 * 入口参数：            	 const char* eth 网口名
                            const char* newip   ip
 * 出口参数：            	 无
 *****************************************************************/
int Alter_IPaddress(const char* eth,const char* newip){
    char sys_cmd[128];
    char temp_file[] = TEMP_FILE;
    
    //检查文件是否存在
    if(access(INTERFACE_FILE_DIR,F_OK) != 0){
        printf("interface file no exist\r\n");
        return -1;
    }
    //备份文件
    snprintf(sys_cmd, sizeof(sys_cmd), "cp %s %s", INTERFACE_FILE_DIR,INTERFACE_BAKFILE_DIR);
    if(system(sys_cmd) != 0){
        printf("backup failed \r\n");
        return -1;
    }
    //修改interface文件中的ip
    FILE *fpin = fopen(INTERFACE_FILE_DIR,"r");
    FILE *fpout =fopen(TEMP_FILE,"w");
    if(!fpin || !fpout){
        printf("open interface file failed\r\n");
        fclose(fpin);
        fclose(fpout);
    }
    char line[128];
    int in_target_iface = 0;
    //逐行读取并替换ip
    while(fgets(line,sizeof(line),fpin)){
        char match[64];
        snprintf(match,sizeof(match),"iface %s net static",eth);
        if(strstr(line,match)){
            in_target_iface = 1;
            fputs(line,fpout);
            continue;
        }
        //离开当前接口配置段
        if(in_target_iface && strstr(line,"iface ")){
            in_target_iface = 0;
        }
        //替换address
        if(in_target_iface && strstr(line,"address ")){
            fprintf(fpout, "address %s\n",newip);
        }else{
            fputs(line,fpout);
        }
    }
    fclose(fpin);
    fclose(fpout);

    //覆盖原文件
    snprintf(sys_cmd,sizeof(sys_cmd),"mv %s %s",TEMP_FILE,INTERFACE_FILE_DIR);
    if(system(sys_cmd) != 0){
        printf("replace interface file failed\r\n");
        return -1;
    }
    return 0;
}

/*****************************************************************
 * 名称：                    System_IPconfig
 * 功能：                    获取当前系统IP
 * 入口参数：            	 const char* ifname 网卡名称：eth0
                            char ipaddr[15]：存放ip地址的缓冲区指针
 * 出口参数：            	 成功  0
                            失败 -1
 *****************************************************************/
int System_IPconfig(const char* ifname,char ipaddr[15]){
    int fd;
    struct ifreq ifr;
    struct sockaddr_in *addr;
    memset(ipaddr,0,15);
    fd = socket(AF_INET,SOCK_DGRAM,0);
    if(fd < 0){
        DBG("socket\r\n");
        return -1;
    }
    //设置网卡名称
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    //执行ioctl命令获取IP地址
    if(ioctl(fd,SIOCGIFADDR,&ifr) < 0 ){
        DBG("ioctl SIOCGIFADDR\r\n");
        close(fd);
        return -1;
    }
    close(fd);
    addr = (struct sockaddr_in*)&ifr.ifr_addr;
    strncpy(ipaddr, inet_ntoa(addr->sin_addr), 14);
    ipaddr[14] = '\0';
    return 0;
}

/*
    获取与我建立TCP连接的设备的IP地址
*/
int get_peer_ip(int sock_fd, char *ip_buffer, int buffer_size) {
    struct sockaddr_in peer_addr;
    socklen_t addr_len = sizeof(peer_addr);
    
    if (getpeername(sock_fd, (struct sockaddr*)&peer_addr, &addr_len) == 0) {
        const char *ip = inet_ntop(AF_INET, &peer_addr.sin_addr, ip_buffer, buffer_size);
        strncpy(ip_buffer, ip, buffer_size - 1);
        ip_buffer[buffer_size - 1] = '\0';
        return 0;
    }
    return -1;
}

