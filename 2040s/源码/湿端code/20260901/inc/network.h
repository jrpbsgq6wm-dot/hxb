#ifndef NETWORK_H
#define NETWORK_H

#include "beam.h"

/*
 * 功能: 设置 TCP keepalive 参数，用于检测连接是否仍然有效。
 * 参数: lisfd 为 socket 文件描述符；begin 为首次探测前的空闲秒数；cnt 为探测次数；intvl 为探测间隔秒数。
 * 返回值: 无。
 */
void setkeepalive(int lisfd, unsigned int begin, unsigned int cnt, unsigned int intvl);

/*
 * 功能: 初始化 TCP 服务端 socket，绑定端口并进入监听状态。
 * 参数: 无。
 * 返回值: 无；失败时打印错误并退出进程。
 */
void init_socket_server(void);

/*
 * 功能: 从 socket 循环接收指定长度的数据，尽量保证收满 len 字节。
 * 参数: fd 为 socket 文件描述符；buf 为接收缓冲区；len 为期望接收字节数。
 * 返回值: 返回最后一次 recv 的字节数；连接关闭或出错时返回 0 或负值。
 */
unsigned int recv_socket(int fd, char *buf, unsigned int len);

/*
 * 功能: 向上位机发送声呐硬件状态信息包。
 * 参数: 无。
 * 返回值: 无；发送失败时调用错误处理流程。
 */
void send_status_to_upper(void);

#endif
