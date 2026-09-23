#include "write_file.h"

// 全局写入器结构体
typedef struct {
    char filename[MAX_FILENAME_LEN];
    FILE* fp;
    int is_initialized;
    int total_written;
    pthread_mutex_t mutex;
} hex_writer_t;

static hex_writer_t g_writer = {
    .filename = {0},
    .fp = NULL,
    .is_initialized = 0,
    .total_written = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

// ========== 初始化函数 ==========
int hex_writer_init(const char* filename) {
    if (filename == NULL) {
        printf("Error: filename is NULL\n");
        return -1;
    }
    
    pthread_mutex_lock(&g_writer.mutex);
    
    // 如果已经初始化，先关闭
    if (g_writer.is_initialized) {
        if (g_writer.fp) {
            fclose(g_writer.fp);
            g_writer.fp = NULL;
        }
        g_writer.is_initialized = 0;
    }
    
    // 复制文件名
    strncpy(g_writer.filename, filename, MAX_FILENAME_LEN - 1);
    g_writer.filename[MAX_FILENAME_LEN - 1] = '\0';
    
    // 打开文件（追加模式）
    g_writer.fp = fopen(filename, "a");
    if (g_writer.fp == NULL) {
        perror("Failed to open file");
        pthread_mutex_unlock(&g_writer.mutex);
        return -1;
    }
    
    // 设置行缓冲，提高性能
    setvbuf(g_writer.fp, NULL, _IOLBF, 4096);
    
    g_writer.is_initialized = 1;
    g_writer.total_written = 0;
    
    // 写入文件头
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(g_writer.fp, "\n");
    fprintf(g_writer.fp, "========================================\n");
    fprintf(g_writer.fp, "Hex Data Log Started: %s\n", time_str);
    fprintf(g_writer.fp, "Format: HEX, %d bytes per line\n", HEX_BYTES_PER_LINE);
    fprintf(g_writer.fp, "========================================\n\n");
    fflush(g_writer.fp);
    
    printf("Hex writer initialized, logging to: %s\n", filename);
    
    pthread_mutex_unlock(&g_writer.mutex);
    return 0;
}

// ========== 以十六进制格式写入数据（每16字节换行）==========
int hex_writer_append(const unsigned char* data, size_t data_size, int record_num) {
    if (data == NULL || data_size == 0) {
        printf("Error: Invalid data or size\n");
        return -1;
    }
    
    pthread_mutex_lock(&g_writer.mutex);
    
    if (!g_writer.is_initialized || g_writer.fp == NULL) {
        printf("Error: Writer not initialized\n");
        pthread_mutex_unlock(&g_writer.mutex);
        return -1;
    }
    
    // 写入记录头
    if (record_num > 0) {
        fprintf(g_writer.fp, "---------- Record #%d (size: %zu bytes) ----------\n", 
                record_num, data_size);
    } else {
        fprintf(g_writer.fp, "---------- Data Block (size: %zu bytes) ----------\n", 
                data_size);
    }
    
    // 写入时间戳
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(g_writer.fp, "Timestamp: %s\n", time_str);
    fprintf(g_writer.fp, "Address:  ");
    
    // 写入地址头（偏移量）
    for (int i = 0; i < HEX_BYTES_PER_LINE; i++) {
        fprintf(g_writer.fp, "%02X ", i);
    }
    fprintf(g_writer.fp, "\n");
    fprintf(g_writer.fp, "--------  ");
    for (int i = 0; i < HEX_BYTES_PER_LINE; i++) {
        fprintf(g_writer.fp, "--- ");
    }
    fprintf(g_writer.fp, "\n");
    
    // 写入十六进制数据，每16字节换行
    for (size_t i = 0; i < data_size; i++) {
        // 每行开始显示偏移地址
        if (i % HEX_BYTES_PER_LINE == 0) {
            fprintf(g_writer.fp, "0x%04lX: ", i);
        }
        
        // 写入一个字节的十六进制
        fprintf(g_writer.fp, "%02X ", data[i]);
        
        // 每16字节换行
        if ((i + 1) % HEX_BYTES_PER_LINE == 0) {
            fprintf(g_writer.fp, "\n");
        }
    }
    
    // 如果最后一行不满16字节，也换行
    if (data_size % HEX_BYTES_PER_LINE != 0) {
        fprintf(g_writer.fp, "\n");
    }
    
    // 添加空行分隔
    fprintf(g_writer.fp, "\n");
    
    fflush(g_writer.fp);
    
    g_writer.total_written++;
    
    pthread_mutex_unlock(&g_writer.mutex);
    return 0;
}

// ========== 批量追加写入 ==========
int hex_writer_append_batch(const unsigned char* data, size_t data_size, 
                            int batch_count, int interval_ms) {
    if (data == NULL || data_size == 0 || batch_count <= 0) {
        printf("Error: Invalid parameters\n");
        return -1;
    }
    
    int success_count = 0;
    
    for (int i = 0; i < batch_count; i++) {
        int ret = hex_writer_append(data, data_size, i + 1);
        if (ret == 0) {
            success_count++;
            printf("Write #%d success, total: %d\n", i + 1, success_count);
        } else {
            printf("Write #%d failed\n", i + 1);
            break;
        }
        
        // 如果有间隔时间，等待
        if (interval_ms > 0 && i < batch_count - 1) {
            usleep(interval_ms * 1000);
        }
    }
    
    return success_count;
}

// ========== 获取已写入记录数 ==========
int hex_writer_get_count(void) {
    pthread_mutex_lock(&g_writer.mutex);
    int count = g_writer.total_written;
    pthread_mutex_unlock(&g_writer.mutex);
    return count;
}

// ========== 关闭写入器 ==========
void hex_writer_close(void) {
    pthread_mutex_lock(&g_writer.mutex);
    
    if (g_writer.is_initialized && g_writer.fp != NULL) {
        // 写入结束标记
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        
        fprintf(g_writer.fp, "\n");
        fprintf(g_writer.fp, "========================================\n");
        fprintf(g_writer.fp, "Hex Data Log Ended: %s\n", time_str);
        fprintf(g_writer.fp, "Total records written: %d\n", g_writer.total_written);
        fprintf(g_writer.fp, "========================================\n");
        
        fflush(g_writer.fp);
        fsync(fileno(g_writer.fp));
        fclose(g_writer.fp);
        g_writer.fp = NULL;
        g_writer.is_initialized = 0;
        
        printf("Hex writer closed, total records: %d\n", g_writer.total_written);
    }
    
    pthread_mutex_unlock(&g_writer.mutex);
}