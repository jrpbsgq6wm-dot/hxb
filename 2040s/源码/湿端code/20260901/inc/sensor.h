#ifndef SENSOR_H
#define SENSOR_H

#include "beam.h"

/*
 * 功能: 打开指定串口设备。
 * 参数: port 为串口设备路径，例如 /dev/ttyPS1。
 * 返回值: 成功返回串口文件描述符；失败返回 FALSE。
 */
int com_open(char* port);

/*
 * 功能: 配置串口波特率、数据位、校验位和停止位。
 * 参数: fd 为串口文件描述符；nSpeed 为波特率；nBits 为数据位；nEvent 为校验模式；nStop 为停止位。
 * 返回值: 成功返回 0；失败返回 FALSE。
 */
int set_com_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop);

/*
 * 功能: 计算罗经接收数据的和校验。
 * 参数: buf 为待校验数据；len 为参与校验的字节数。
 * 返回值: 返回计算出的 1 字节和校验值。
 */
char sum_check(char *buf,int len);

/*
 * 功能: 设置周期性定时器。
 * 参数: seconds 为秒数；mseconds 为微秒数。
 * 返回值: 无。
 */
void setTimer(int seconds, int mseconds);

/*
 * 功能: 触发一次温度读取寄存器状态切换。
 * 参数: 无。
 * 返回值: 返回切换后的内部状态值。
 */
int change(void);

/*
 * 功能: 读取温度和罗经传感器数据并更新待上传状态。
 * 参数: 无。
 * 返回值: 无。
 */
void get_sensor_data(void);

/*
 * 功能: 初始化 TMP451 与罗经串口，并循环读取传感器数据。
 * 参数: 无。
 * 返回值: 无；函数内部为循环处理流程。
 */
void read_TMP451_sensor_and_save(void);

/*
 * 功能: SIGALRM 定时器回调，用于更新定时标志和运行状态。
 * 参数: sig 为信号编号。
 * 返回值: 无。
 */
void timer(int sig);

#endif
