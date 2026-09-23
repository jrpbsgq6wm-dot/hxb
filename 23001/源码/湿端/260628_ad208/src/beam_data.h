#ifndef BEAM_DATA_H
#define BEAM_DATA_H

void my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz);
unsigned short checksum(char *buf, unsigned int nword);
void send_all_package_to_upper(void);
void lookfor_five_sensor_num(const int *ptr_sensor1,const int *ptr_sensor2,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5);
void copy_sensor_data(const int *ptr_sensor1,const int *ptr_sensor2,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5);
void copy_letf_three_sensor_num_to_ddr(void);
void copy_sensor_data_from_pingpang_buf(void);
void copy_tail_to_sonar_data(void);
void copy_sensor_data_streaming(const int *ptr_sensor1,const int *ptr_sensor2,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5);
void copy_letf_three_sensor_num_to_ddr_streaming(void);
void copy_sensor_data_from_pingpang_buf_streaming(void);
void send_original_data_streaming(void);
void send_original_data_streaming_8001(void);
void send_all_package_to_upper_and_clear_sensor_num(void);
void prepare_first_128_byte_data(void);
void prepare_IQ_data_len(void);
void prepare_original_data_len(void);
void read_fpga_send_to_upper(void);

#endif /* BEAM_DATA_H */