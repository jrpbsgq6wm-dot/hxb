// file_manager_fsm.h
#ifndef FILE_MANAGER_FSM_H
#define FILE_MANAGER_FSM_H

#include "usart.h"
#include "ff.h"
#include "fat.h"

// 命令码
#define CMD_READ_FILE_LIST     0x17  // 读取文件列表
#define CMD_DOWNLOAD_FILE      0x18  // 下载文件
#define CMD_DELETE_FILE        0x19  // 删除单个文件
#define CMD_FORMAT             0x1A  // 格式化/删除所有文件
#define CMD_BATCH_DELETE       0x31  // 批量删除文件

// 帧头帧尾
#define FRAME_HEAD_REQ         0xAA  // 请求帧头
#define FRAME_TAIL_REQ         0x55  // 请求帧尾
#define FRAME_HEAD_RSP         0x55  // 响应帧头
#define FRAME_TAIL_RSP         0xAA  // 响应帧尾

// 方向
#define DIR_PC_TO_STM32        0x01
#define DIR_STM32_TO_PC        0x00

// 包大小定义
#define MAX_PACKET_SIZE        1024
#define FILENAME_LEN           19
#define MAX_FILE_COUNT         100   // 最大文件数

// 状态码
#define STATUS_SUCCESS         0x00
#define STATUS_FILE_NOT_FOUND  0x01
#define STATUS_READ_ERROR      0x02
#define STATUS_WRITE_ERROR     0x03
#define STATUS_DELETE_ERROR    0x04
#define STATUS_FORMAT_ERROR    0x05
#define STATUS_PARAM_ERROR     0x06

//超时时间
#define DOWNLOAD_TIMEOUT_MS     5000    // 5秒超时
#define FILE_LIST_TIMEOUT_MS    5000    // 5秒超时

//其他宏
#define MAX_FILES_PER_PACKET  40

// ========== 状态枚举 ==========
typedef enum {
    // 全局状态
    STATE_IDLE,                      // 空闲状态
    // 文件列表相关状态
    STATE_READING_FILE_LIST,         // 正在读取文件列表
    // 下载相关状态
    STATE_DOWNLOAD_WAIT_FILENAME,    // 等待下载文件名
    STATE_DOWNLOAD_FILE_OPENED,      // 文件已打开
    STATE_DOWNLOADING,               // 下载中
    STATE_DOWNLOAD_WAIT_RETRANS,     // 等待重发请求
    // 删除相关状态
    STATE_DELETING_FILE,             // 正在删除文件
    STATE_BATCH_DELETING,            // 批量删除中
    // 格式化相关状态
    STATE_FORMATTING,                // 正在格式化
    // 错误状态
    STATE_ERROR                      // 错误状态
} FileManagerState;  

// ========== 下载上下文 ==========
typedef struct {
    uint32_t total_packets;           // 总包数
    uint32_t current_packet;          // 当前包号
    uint32_t file_size;               // 文件大小
    char filename[FILENAME_LEN + 1];  // 文件名
    FIL file;                         // 文件对象
    uint8_t retry_count;              // 重试次数
} DownloadContext;

// ========== 批量删除上下文 ==========
typedef struct {
    uint32_t file_count;              // 要删除的文件个数
    char filenames[MAX_FILE_COUNT][FILENAME_LEN + 1];  // 文件名列表
    uint32_t deleted_count;           // 已删除个数
    uint32_t current_index;           // 当前删除索引
} BatchDeleteContext;

// ========== 主上下文（包含所有状态） ==========
typedef struct {
    FileManagerState state;           // 当前状态
    DownloadContext download;         // 下载上下文
    BatchDeleteContext batch_delete;  // 批量删除上下文
    uint32_t last_activity_time;      // 最后活动时间（用于超时）
    uint8_t error_code;               // 错误码
} FileManagerContext;

// ========== 对外接口 ==========

// 初始化状态机
void file_manager_fsm_init(void);

/*  处理串口接收到文件处理指令（主入口）
    (0x17) → handle_read_file_list()    → 扫描，返回文件列表
    (0x18) → handle_download_request()  → 打开文件，返回总包数
            handle_packet_request()    → 读取指定包，返回数据
            handle_retrans_request()   → 重发数据包
            handle_complete_notify()   → 关闭文件
    (0x19) → handle_delete_file()       → 删除单个文件
    (0x1A) → handle_format()            → 删除所有文件
    (0x31) → handle_batch_delete()      → 批量删除文件
*/
void file_manager_fsm_process(uint8_t *buffer, int len);

// 获取当前状态
FileManagerState file_manager_fsm_get_state(void);

// 重置状态机
void file_manager_fsm_reset(void);

// 超时检查（需要在主循环中调用）
void file_manager_fsm_timeout_check(uint32_t current_tick, uint32_t timeout_ms);

// 获取错误码
uint8_t file_manager_fsm_get_error(void);

#endif
