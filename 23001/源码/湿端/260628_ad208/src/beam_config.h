#ifndef BEAM_CONFIG_H
#define BEAM_CONFIG_H

#include "beam_common.h"

unsigned short crc16(void* data, unsigned int len, unsigned short crc);
int parsing_instructions_200k(const RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package, FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para);
void Debug_pritf_convert_fpga_parameter(FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para);
void Debug_pritf_receive_para(RECV_UPPER_CONFIG_PARAMETERS *receive_upper_package);
void copy_recv_cmd(RECV_UPPER_CONFIG_PARAMETERS *ptr_cmd_des, const RECV_UPPER_CONFIG_PARAMETERS *ptr_cmd_source);
void config_sensor_baud_and_framehead(const RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package);
void Debug_pritf_sensor_baud_and_framehead(FPGA_CONFIG_SENSER_PARAMETERS *fpga_sensor_printf);

#endif /* BEAM_CONFIG_H */