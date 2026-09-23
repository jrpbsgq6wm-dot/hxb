#ifndef FPGA_TO_UPPER_H
#define FPGA_TO_UPPER_H

#include "beam.h"

/*
 * 功能: 从 volatile 源地址按字节拷贝数据到目标地址。
 * 参数: dst 为目标缓冲区；src 为源缓冲区；sz 为拷贝字节数。
 * 返回值: 无。
 */
void my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz);

/*
 * 功能: 计算缓冲区的 16 位累加校验和。
 * 参数: buf 为待校验缓冲区；nword 为参与校验的字节数。
 * 返回值: 返回 16 位校验结果。
 */
unsigned short checksum(char *buf, unsigned int nword);

/*
 * 功能: 将已组好的声呐数据、传感器数据和包尾发送给上位机。
 * 参数: 无。
 * 返回值: 无；发送失败时调用错误处理流程。
 */
void send_all_package_to_upper(void);

/*
 * 功能: 从五个传感器共享缓冲区中查找当前 ping 对应的传感器数据条数。
 * 参数: ptr_sensor1 至 ptr_sensor5 分别为五路传感器共享缓冲区指针。
 * 返回值: 无；结果写入全局 sensor*_num 计数变量。
 */
void lookfor_five_sensor_num(const int *ptr_sensor1,const int *ptr_sensor2,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5);

/*
 * 功能: 将指定五路传感器数据拷贝到待上传 DDR 数据包中。
 * 参数: ptr_sensor1 至 ptr_sensor5 分别为五路传感器共享缓冲区指针。
 * 返回值: 无。
 */
void copy_sensor_data(const int *ptr_sensor1,const int *ptr_sensor2,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5);

/*
 * 功能: 将前三路剩余传感器计数信息拷贝到 DDR 数据包。
 * 参数: 无。
 * 返回值: 无。
 */
void copy_letf_three_sensor_num_to_ddr(void);

/*
 * 功能: 根据 ping-pang 缓冲区状态选择对应传感器缓冲区并拷贝数据。
 * 参数: 无。
 * 返回值: 无。
 */
void copy_sensor_data_from_pingpang_buf(void);

/*
 * 功能: 将 CRC 和数据包尾写入待上传声呐数据包。
 * 参数: 无。
 * 返回值: 无。
 */
void copy_tail_to_sonar_data(void);

/*
 * 功能: 发送完整数据包到上位机，并清空本帧传感器计数。
 * 参数: 无。
 * 返回值: 无。
 */
void send_all_package_to_upper_and_clear_sensor_num(void);

/*
 * 功能: 从 FPGA 帧头中整理上位机需要的前 128 字节声呐参数。
 * 参数: 无。
 * 返回值: 无。
 */
void prepare_first_128_byte_data(void);

/*
 * 功能: 根据 FPGA IQ 数据帧头计算声呐数据长度。
 * 参数: 无。
 * 返回值: 无；结果写入发送包头的 SonarDataLength 字段。
 */
void prepare_IQ_data_len(void);

/*
 * 功能: 根据 FPGA 原始数据帧头计算声呐数据长度。
 * 参数: 无。
 * 返回值: 无；结果写入发送包头的 SonarDataLength 字段。
 */
void prepare_original_data_len(void);

/*
 * 功能: 从 FPGA 共享内存读取声呐帧，组织数据包后发送给上位机。
 * 参数: 无。
 * 返回值: 无；函数内部为循环处理流程。
 */
void read_fpga_send_to_upper(void);

#endif
