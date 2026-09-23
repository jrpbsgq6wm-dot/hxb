#ifndef BEAM_THREADS_H
#define BEAM_THREADS_H

void set_fpga_send_to_upper_flag(void);
void *thread_upper_to_fpga(void *arg);
void *thread_fpga_to_upper(void *arg);
void *thread_sensor(void *arg);
void *thread_fpga_to_8001_upper(void *arg);
void thread_create(void);
void thread_wait(void);

#endif /* BEAM_THREADS_H */