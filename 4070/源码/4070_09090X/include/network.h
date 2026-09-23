#ifndef _NETWORK_H_
#define _NETWORK_H_

/**************************************socket相关**************************************/
#define DEFAULT_PORT_SERVER 7000                    // 端口号7000
#define MAIN_SONAR_DEF_IP   "192.168.0.5"           // 主
#define MINOR_SONAR_DEF_IP  "192.168.0.4"           // 从


/*2026.1.16 新增*/
#define INTERFACE_FILE_DIR      "/etc/network/interface"
#define INTERFACE_BAKFILE_DIR   "/etc/n.etwork/interface.bak"
#define TEMP_FILE               "/tmp/iface.tmp"    //暂存修改interface文件信息
extern struct sockaddr_in Servaddr_server, addr_client;    // socket相关的API结构体

extern int Socket_fd_server;
extern int Connect_fd;
extern char upper_ip[15];
extern void init_socket_server(void);
extern void error_process(void);
extern int Alter_IPaddress(const char* eth,const char* newip);
extern int System_IPconfig(const char* ifname,char ipaddr[15]);
extern int get_peer_ip(int sock_fd, char *ip_buffer, int buffer_size);
#endif /* _NETWORK_H_ */
