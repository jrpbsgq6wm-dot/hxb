#ifndef UPPER_TO_FPGA_H
#define UPPER_TO_FPGA_H

#include "beam.h"

/*
 * 功能: 按 CRC16 表计算数据校验值。
 * 参数: data 为待校验数据首地址；len 为数据长度；crc 为初始 CRC 值。
 * 返回值: 返回计算后的 CRC16 值。
 */
unsigned short crc16(void* data, unsigned int len, unsigned short crc);

/*
 * 功能: 将上位机配置参数转换为 FPGA 200k 参数寄存器配置。
 * 参数: ptr_recv_upper_package 为上位机配置包；ptr_fpga_config_para 为输出的 FPGA 配置结构体。
 * 返回值: 成功返回 0；参数异常时按原逻辑返回错误值。
 */
int parsing_instructions_200k(const RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package, FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para);

/*
 * 功能: 调试打印转换后的 FPGA 参数。
 * 参数: ptr_fpga_config_para 为待打印的 FPGA 配置结构体。
 * 返回值: 无。
 */
void Debug_pritf_convert_fpga_parameter(FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para);

/*
 * 功能: 调试打印上位机下发的原始配置参数。
 * 参数: receive_upper_package 为待打印的上位机配置包。
 * 返回值: 无。
 */
void Debug_pritf_receive_para(RECV_UPPER_CONFIG_PARAMETERS *receive_upper_package);

/*
 * 功能: 拷贝并保存一份上位机配置命令。
 * 参数: ptr_cmd_des 为目标命令结构体；ptr_cmd_source 为源命令结构体。
 * 返回值: 无。
 */
void copy_recv_cmd(RECV_UPPER_CONFIG_PARAMETERS *ptr_cmd_des, const RECV_UPPER_CONFIG_PARAMETERS *ptr_cmd_source);

/*
 * 功能: 清空各传感器共享内存中的帧头和计数字段。
 * 参数: 无。
 * 返回值: 无。
 */
void clear_sensor_data(void);

/*
 * 功能: 处理通信或运行错误，停止 FPGA 工作并关闭当前连接。
 * 参数: 无。
 * 返回值: 无。
 */
void error_process(void);

/*
 * 功能: 根据上位机配置设置 FPGA 侧传感器波特率、帧头和帧尾寄存器。
 * 参数: ptr_recv_upper_package 为上位机配置包。
 * 返回值: 无。
 */
void config_sensor_baud_and_framehead(const RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package);

/*
 * 功能: 调试打印 FPGA 传感器波特率、帧头和帧尾配置。
 * 参数: fpga_sensor_printf 为待打印的 FPGA 传感器配置结构体。
 * 返回值: 无。
 */
void Debug_pritf_sensor_baud_and_framehead(FPGA_CONFIG_SENSER_PARAMETERS *fpga_sensor_printf);

/*
 * 功能: 接收上位机命令，解析后配置 FPGA，并处理启动、停止、状态查询和同步设置。
 * 参数: 无。
 * 返回值: 无；函数内部为循环处理流程。
 */
void receive_process_sendto_fpga(void);

#endif
