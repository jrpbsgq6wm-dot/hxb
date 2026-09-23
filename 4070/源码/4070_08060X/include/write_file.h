#ifndef WRITER_FILE_H
#define WRITER_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// 最大文件名长度
#define MAX_FILENAME_LEN 256
// 每行显示的字节数
#define HEX_BYTES_PER_LINE 16

// 初始化十六进制文件写入器
// filename: 要写入的文件路径
// 返回值: 0成功, -1失败
int hex_writer_init(const char* filename);

// 追加写入数据（十六进制格式，每16字节换行）
// data: 要写入的数据指针
// data_size: 数据大小（字节）
// record_num: 记录编号（用于标识，传0则不显示）
// 返回值: 0成功, -1失败
int hex_writer_append(const unsigned char* data, size_t data_size, int record_num);

// 批量追加写入
// data: 要写入的数据指针
// data_size: 数据大小（字节）
// batch_count: 写入次数
// interval_ms: 间隔时间（毫秒），0表示不间隔
// 返回值: 成功写入次数, -1失败
int hex_writer_append_batch(const unsigned char* data, size_t data_size, 
                            int batch_count, int interval_ms);

// 获取已写入记录数
int hex_writer_get_count(void);

// 关闭写入器
void hex_writer_close(void);

#endif // HEX_FILE_WRITER_H