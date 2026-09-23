#ifndef BEAM_GLOBALS_H
#define BEAM_GLOBALS_H

#include "beam_common.h"

extern int Fpga_start_mod;
extern int sensor1_num, sensor2_num, sensor3_num, sensor4_num, sensor5_num;
extern unsigned int total_len;
extern int flag;
extern int flag_timer;
extern int syncstatus;
extern int fd_uio6;
extern int irq_on;
extern char DataHead_S[4];
extern char DataTail_S[4];
extern unsigned short send_crc_S, send_crc_tmp_S;

#ifdef TEST_TIME
extern struct timeval tv1;
extern struct timezone tz1;
extern struct timeval tv2;
extern struct timezone tz2;
extern struct timeval tv3;
extern struct timezone tz3;
extern struct timeval tv4;
extern struct timezone tz4;
#endif
extern struct timeval start_time;
extern struct timeval end_time;

extern RECV_UPPER_CONFIG_PARAMETERS recv_upper_package;
extern RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package;
extern RECV_UPPER_CONFIG_PARAMETERS cmd_package;
extern SEND_UPPER_SONAR_STATUS send_upper_status_information;
extern SEND_UPPER_SONAR_STATUS *ptr_send_upper_status_information;
extern SEND_UPPER_PACKAGE_FIRST send_to_upper_package_first;
extern SEND_UPPER_PACKAGE_FIRST *ptr_send_to_upper_package_first;
extern SEND_UPPER_SENSOR_FIRST send_to_upper_sensor;
extern SEND_UPPER_SENSOR_FIRST *ptr_send_to_upper_sensor;

extern int Socket_fd_server, Connect_fd;
extern int Socket_fd_server_8001, Connect_fd_8001;
extern struct sockaddr_in Servaddr_server;
extern struct sockaddr_in Servaddr_server_8001;
extern volatile int netStatus_8001;
extern volatile int send_to_8001_flag;
extern volatile int netStatus;

extern UIO_CONFIG_PARAMETER uio_fpga_register;
extern UIO_CONFIG_PARAMETER uio_tvg_register;
extern UIO_CONFIG_PARAMETER uio_share_mem_IQ;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_0;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_8;
extern UIO_CONFIG_PARAMETER uio_share_mem_original;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_1, uio_sensor_mem_2, uio_sensor_mem_3, uio_sensor_mem_4;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_9, uio_sensor_mem_A, uio_sensor_mem_B, uio_sensor_mem_C;

extern FPGA_REGISTERS fpga_register_data;
extern FPGA_REGISTERS *ptr_fpga_register_data;
extern FPGA_DDR_FIRST fpga_ddr_frame_first;
extern FPGA_DDR_FIRST *ptr_fpga_frame_first;

extern unsigned char ddr_sonar_data[30000000];
extern int fd_icc;
extern unsigned char local_high_value, local_low_value, remote_high_value, remote_low_value;
extern unsigned char status_register;
extern float temprature1;
extern int tem_num;
extern uint8 i2c_read_reg;
extern char pc_ip[16];
extern int fd_compass;
extern char CompassSendBuf[5];
extern char CompassRecvBuf[14];
extern char sum_calculate, recv_sum;
extern int nread_compass;
extern COM_CONFIG_PARAMETER com_compass;

extern pthread_t thread[4];
extern pthread_mutex_t mut;
extern pthread_mutex_t mut_8001;

#endif /* BEAM_GLOBALS_H */