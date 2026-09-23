#ifndef _NETWORK_H_
#define _NETWORK_H_

/**************************************socket相关**************************************/
#define DEFAULT_PORT_SERVER 7000                    // 端口号7000
struct sockaddr_in Servaddr_server, addr_client;    // socket相关的API结构体

extern int Socket_fd_server;
extern int Connect_fd;

extern void init_socket_server(void);
extern void error_process(void);

#endif /* _NETWORK_H_ */
