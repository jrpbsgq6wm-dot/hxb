#ifndef BEAM_NETWORK_H
#define BEAM_NETWORK_H

#include "beam_common.h"

void setkeepalive(int lisfd, unsigned int begin, unsigned int cnt, unsigned int intvl);
void set_send_timeout(int fd, unsigned int seconds);
void init_socket_server(void);
void init_socket_server_8001(void);
void error_process(void);
void error_process_8001(void);
int recv_socket(int fd, char *buf, unsigned int len);
int send_all_bytes(int fd, const void *buf, unsigned int len);
int send_locked_bytes(int fd, const void *buf, unsigned int len);
void send_status_to_upper(void);

#endif /* BEAM_NETWORK_H */