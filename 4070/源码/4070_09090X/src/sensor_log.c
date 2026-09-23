// sensor_log.c
#include "sensor_log.h"
#include "fpga_to_upper.h"
#include "CuBeamOne.h"

// 全局变量定义（只能有一个地方定义）
sensor_queue_t g_sensor_queue = {
    .write_index = 0,
    .read_index = 0,
    .count = 0,
    .stop_flag = 0,
    .total_written = 0,
    .max_records = 2000,
    .is_initialized = 0
};

// ========== 写入线程函数 ==========
void* sensor_write_thread_func(void* arg) {
    sensor_queue_t* queue = (sensor_queue_t*)arg;
    FILE* fp = NULL;
    char filename[256];
    int i;
    
    // 在线程内部进行初始化
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
    queue->stop_flag = 0;
    queue->total_written = 0;
    queue->write_index = 0;
    queue->read_index = 0;
    queue->count = 0;
    queue->max_records = 2000;
    memset(queue->buffers, 0, sizeof(queue->buffers));
    queue->is_initialized = 1;
    
    DBG("Sensor write thread initialized\n");
    
    // 生成带时间戳的文件名
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(filename, sizeof(filename), "/test/sensor_data_%Y%m%d_%H%M%S.txt", tm_info);
    
    fp = fopen(filename, "a");
    if (fp == NULL) {
        perror("Failed to open sensor data file");
        return NULL;
    }
    
    DBG("Sensor write thread started, logging to: %s\n", filename);
    
    while (1) {
        sensor_record_t record;
        int need_write = 0;
        
        pthread_mutex_lock(&queue->mutex);
        
        while (queue->count == 0 && !queue->stop_flag) {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
        
        if (queue->stop_flag && queue->count == 0) {
            pthread_mutex_unlock(&queue->mutex);
            break;
        }
        
        if (queue->total_written >= queue->max_records) {
            pthread_mutex_unlock(&queue->mutex);
            DBG("Reached %d records, stopping write thread\n", queue->max_records);
            break;
        }
        
        if (queue->count > 0) {
            memcpy(&record, &queue->buffers[queue->read_index], sizeof(sensor_record_t));
            queue->buffers[queue->read_index].valid = 0;
            queue->read_index = (queue->read_index + 1) % MAX_BUFFER_COUNT;
            queue->count--;
            need_write = 1;
        }
        
        pthread_mutex_unlock(&queue->mutex);
        
        if (need_write) {
            fprintf(fp, "========== Record %d ==========\n", record.record_num);
            
            time_t t = time(NULL);
            struct tm* tm_info2 = localtime(&t);
            char time_str[64];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info2);
            fprintf(fp, "Timestamp: %s\n", time_str);
            fprintf(fp, "IQ_Ping: %d\n",record.iq_ping);
            fprintf(fp, "GNSS (size: %zu bytes):\n", sizeof(record.gnss_data));
            for (i = 0; i < sizeof(record.gnss_data); i++) {
                fprintf(fp, "%02X ", (unsigned char)record.gnss_data[i]);
                if ((i + 1) % 16 == 0) {
                    fprintf(fp, "\n");
                }
            }
            fprintf(fp, "\n");
            fprintf(fp, "PASHR (size: %zu bytes):\n", sizeof(record.pasht_data));
            for (i = 0; i < sizeof(record.pasht_data); i++) {
                fprintf(fp, "%02X ", (unsigned char)record.pasht_data[i]);
                if ((i + 1) % 16 == 0) {
                    fprintf(fp, "\n");
                }
            }
            fprintf(fp, "\n\n");
            
            fflush(fp);
            
            pthread_mutex_lock(&queue->mutex);
            queue->total_written++;
            pthread_mutex_unlock(&queue->mutex);
            
            DBG("Write thread: saved record %d, total: %d\n", 
                record.record_num, queue->total_written);
        }
    }
    
    if (fp != NULL) {
        fflush(fp);
        fsync(fileno(fp));
        fclose(fp);
    }
    
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    queue->is_initialized = 0;
    
    DBG("Sensor write thread stopped\n");
    return NULL;
}

// ========== 将传感器数据放入队列 ==========
int push_sensor_data_to_queue(char* gnss_data, char* pasht_data, int iq_ping, int record_num) {
    sensor_record_t* record;
    
    if (!g_sensor_queue.is_initialized) {
        DBG("Warning: Sensor queue not initialized yet\n");
        return -2;
    }
    
    pthread_mutex_lock(&g_sensor_queue.mutex);
    
    if (g_sensor_queue.count >= MAX_BUFFER_COUNT) {
        pthread_mutex_unlock(&g_sensor_queue.mutex);
        DBG("Warning: Queue full, dropped record %d\n", record_num);
        return -1;
    }
    
    if (g_sensor_queue.total_written >= g_sensor_queue.max_records) {
        pthread_mutex_unlock(&g_sensor_queue.mutex);
        return 0;
    }
    
    record = &g_sensor_queue.buffers[g_sensor_queue.write_index];
    
    memcpy(record->gnss_data, gnss_data, SENSOR_DATA_SIZE);
    memcpy(record->pasht_data, pasht_data, SENSOR_DATA_SIZE);
    record->iq_ping = iq_ping;
    record->record_num = record_num;
    record->valid = 1;
    
    g_sensor_queue.write_index = (g_sensor_queue.write_index + 1) % MAX_BUFFER_COUNT;
    g_sensor_queue.count++;
    
    pthread_cond_signal(&g_sensor_queue.cond);
    pthread_mutex_unlock(&g_sensor_queue.mutex);
    
    return 1;
}

// ========== 停止写入线程 ==========
void stop_sensor_write_thread(void) {
    if (!g_sensor_queue.is_initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_sensor_queue.mutex);
    g_sensor_queue.stop_flag = 1;
    pthread_cond_signal(&g_sensor_queue.cond);
    pthread_mutex_unlock(&g_sensor_queue.mutex);
}

// ========== 获取写入状态 ==========
int get_sensor_write_status(void) {
    int status;
    if (!g_sensor_queue.is_initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_sensor_queue.mutex);
    status = g_sensor_queue.total_written;
    pthread_mutex_unlock(&g_sensor_queue.mutex);
    return status;
}