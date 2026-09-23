// sensor_log.h
#ifndef SENSOR_LOG_H
#define SENSOR_LOG_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SENSOR_DATA_SIZE (60 * 1024)
#define MAX_BUFFER_COUNT 10

typedef struct {
    char gnss_data[SENSOR_DATA_SIZE];
    char pasht_data[SENSOR_DATA_SIZE];
    int iq_ping;
    int record_num;
    int valid;
} sensor_record_t;

typedef struct {
    sensor_record_t buffers[MAX_BUFFER_COUNT];
    int write_index;
    int read_index;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int stop_flag;
    int total_written;
    int max_records;
    int is_initialized;
} sensor_queue_t;

// 外部声明
extern sensor_queue_t g_sensor_queue;

// 函数声明
void* sensor_write_thread_func(void* arg);
int push_sensor_data_to_queue(char* gnss_data, char* pasht_data,int iq_p, int record_num);
void stop_sensor_write_thread(void);
int get_sensor_write_status(void);

#endif