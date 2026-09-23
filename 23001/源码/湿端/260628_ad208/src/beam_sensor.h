#ifndef BEAM_SENSOR_H
#define BEAM_SENSOR_H

#include "beam_common.h"

void clear_sensor_data(void);
int com_open(char* port);
int set_com_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop);
char sum_check(char *buf,int len);
void setTimer(int seconds, int mseconds);
int change(void);
void get_sensor_data(void);
void read_TMP451_sensor_and_save(void);
void timer(int sig);

#endif /* BEAM_SENSOR_H */