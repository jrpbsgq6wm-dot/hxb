#include "beam.h"

/********************************************************************************
 * ģ�飺ȫ�ֱ���
 * ˵�������б���ԭ beam.c �п��̡߳���ģ�鹲����״̬��
 ********************************************************************************/

int          Fpga_start_mod = 0;                // FPGA��ʼ������־
int          sensor1_num, sensor2_num, sensor3_num, sensor4_num, sensor5_num; // ����������
unsigned int total_len = 0;                     // ARM���͸���λ�������ֽ���

int flag = 0;                                   // ���жϴ�����ÿ�������ֵ��ӳping��
int flag_timer = 0;                             // �¶ȴ�����TMP451�Ķ�ʱ����־
int syncstatus = 0;                             // �豸ͬ��״̬��־
int fd_uio6 = 0;                                // FPGA�ж�UIO�ļ�������
int irq_on = 1;
char DataHead_S[4] = {'<','<','S','T'};         // ARM����λ����������֡ͷ
char DataTail_S[4] = {'E','D','>','>'};         // ARM����λ����������֡β
unsigned short send_crc_S, send_crc_tmp_S = 0;

#ifdef TEST_TIME
struct timeval tv1;
struct timezone tz1;
struct timeval tv2;
struct timezone tz2;
struct timeval tv3;
struct timezone tz3;
struct timeval tv4;
struct timezone tz4;
#endif
struct timeval start_time;
struct timeval end_time;

RECV_UPPER_CONFIG_PARAMETERS recv_upper_package;
RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package = &recv_upper_package;
RECV_UPPER_CONFIG_PARAMETERS cmd_package;

SEND_UPPER_SONAR_STATUS send_upper_status_information;
SEND_UPPER_SONAR_STATUS *ptr_send_upper_status_information = &send_upper_status_information;

SEND_UPPER_PACKAGE_FIRST send_to_upper_package_first;
SEND_UPPER_PACKAGE_FIRST *ptr_send_to_upper_package_first = &send_to_upper_package_first;

SEND_UPPER_SENSOR_FIRST send_to_upper_sensor;
SEND_UPPER_SENSOR_FIRST *ptr_send_to_upper_sensor = &send_to_upper_sensor;

int Socket_fd_server = 0, Connect_fd = 0;
int Socket_fd_server_8001 = 0, Connect_fd_8001 = 0;
struct sockaddr_in Servaddr_server;
struct sockaddr_in Servaddr_server_8001;
volatile int netStatus_8001 = 0;
volatile int send_to_8001_flag = 0;
volatile int netStatus = 0;

UIO_CONFIG_PARAMETER uio_fpga_register = { .physical_addr = (unsigned int *)FPGA_REGISTER_BASEADDR, .uiod = "/dev/uio0", .sysfs_path_file = "/sys/class/uio/uio0/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_tvg_register  = { .physical_addr = (unsigned int *)TVG_REGISTER_BASEADDR,  .uiod = "/dev/uio1", .sysfs_path_file = "/sys/class/uio/uio1/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_share_mem_IQ  = { .physical_addr = (unsigned int *)DDR_SHARE_MEM_BASEADDR_IQ, .uiod = "/dev/uio2", .sysfs_path_file = "/sys/class/uio/uio2/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_sensor_mem_0  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_0, .uiod = "/dev/uio3", .sysfs_path_file = "/sys/class/uio/uio3/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_sensor_mem_8  = { .physical_addr = (unsigned int *)SENSOR_SHARE_MEM_BASEADDR_8, .uiod = "/dev/uio4", .sysfs_path_file = "/sys/class/uio/uio4/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_share_mem_original = { .physical_addr = (unsigned int *)DDR_SHARE_MEM_BASEADDR_ORIGINAL, .uiod = "/dev/uio6", .sysfs_path_file = "/sys/class/uio/uio6/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_sensor_mem_1, uio_sensor_mem_2, uio_sensor_mem_3, uio_sensor_mem_4;
UIO_CONFIG_PARAMETER uio_sensor_mem_9, uio_sensor_mem_A, uio_sensor_mem_B, uio_sensor_mem_C;

FPGA_REGISTERS fpga_register_data;
FPGA_REGISTERS *ptr_fpga_register_data = NULL;
FPGA_DDR_FIRST fpga_ddr_frame_first;
FPGA_DDR_FIRST *ptr_fpga_frame_first = &fpga_ddr_frame_first;

unsigned char ddr_sonar_data[30000000];

int fd_icc = 0;
unsigned char local_high_value = 0, local_low_value = 0, remote_high_value = 0, remote_low_value = 0;
unsigned char status_register = 0;
float temprature1 = 0;
int tem_num = 1;
uint8 i2c_read_reg = 0;
char pc_ip[16] = {0};

int fd_compass;
char CompassSendBuf[5] = {0x68,0x04,0x00,0x04,0x08};
char CompassRecvBuf[14];
char sum_calculate, recv_sum;
int nread_compass;
COM_CONFIG_PARAMETER com_compass = {.dev = "/dev/ttyPS1", .nSpeed = 9600, .nBits = 8, .nEvent = 'N', .nStop = 1};

pthread_t thread[4];
pthread_mutex_t mut;
pthread_mutex_t mut_8001;