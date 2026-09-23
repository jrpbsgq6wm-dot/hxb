#ifndef __FAT__H__
#define __FAT__H__


/* Includes ------------------------------------------------------------------*/
#include "ff.h"
#include "diskio.h"
#include "usart.h"
#include "main.h"
#include "stdio.h"
#include "string.h"
#include "rtc.h"


/* DEFINE --------------------------------------------------------------------*/
#define DISK_PATH       "0:"                //盘符
#define SVP_PATH        "0:/SVP1500"        //文件存储路径


#define YEAR(n)     ((n & 0xfe00)>>9) + 1980    //年
#define MOON(n)     ((n & 0x1E0) >> 5)          //月
#define DATA(n)     (n & 0x1F)                  //日
#define SEC(n)      ((n & 0x1F))                //秒
#define MIN(n)      ((n & 0x7e0) >> 5)          //分
#define HOU(n)      ((n & 0xf800) >> 11)        //间

#define MAX_FILE   100

#define REQUIRED_SPACE      1024*1      //空间小于5MB = 1024KB*1024

/*  定义一个结构体 用于存储当前spi flash中的文件信息 */
typedef struct{
    char filename[32];     //文件名
    uint32_t filesize;        //文件大小
    BYTE attrib;            //文件属性
    DWORD mod_time;         //文件创建时间
}FileInfo;
typedef struct{
    DS3231_TimeType data_time;        //时间结构体
    float pri_sv_v;
    float pt100_v;
    float pa_v;
}SVP_DATA_T;

/*自容模式阈值检测结构体*/
typedef struct{
    volatile float now_pa;   //当前压力
    volatile float last_pa;  //上一次判断成功的压力
    volatile float buf_pa;      //暂存
    uint32_t count;
    float up;
    float down;
}AUTO_THRESHOLD_t;
extern AUTO_THRESHOLD_t auto_thrshold;

#define FILE_DATA_BUF_SIZE      1024    //1行数据53字节

#define DOWNLOAD_FILE_BUF_SIZE  250
#define DOWNLOAD_FILE_CRC_LEN   DOWNLOAD_FILE_BUF_SIZE+2+1

typedef struct{
    volatile uint8_t start_flag;            //开始记录标志位
    volatile uint8_t updata_filename_flag;  //更新文件名标志位 -> 记录新文件
    volatile uint8_t wfile_status;          /*当前文件的状态 -> 1已经打开 0关闭 */
    char filename[32];                      //文件名
    FSIZE_t/*u32*/ file_position;           //文件正在写入的位置    //文件读/写位置指针
    FIL file_s;                             //正在使用的文件句柄
    
    char wfile_buf_a[FILE_DATA_BUF_SIZE];   //1K缓冲区   100ms填充数据
    volatile uint32_t abuf_index;           //a缓冲区下标
    char wfile_buf_b[FILE_DATA_BUF_SIZE];   //1K缓冲区   1s向文件写入 --- 在填入flash之前可以进行判断及修正
    /*暂存平均数据*/
    volatile float sv_ave_buf;
    volatile float temp_ave_buf;
    volatile float pa_ave_buf;
    volatile uint32_t ave_count;        //取平均次数
    volatile uint32_t wfcount;          //频率计数
    volatile uint32_t abuf_count;       //暂存每一次写入的字节数
    volatile uint32_t write_to_file_byte_count; //写入到文件中的字节计数   
    volatile uint8_t pa_flag;       //压力范围标志位
    volatile uint8_t auto_wmode;            //自容模式下的基准模式选择 是以压力变化1还是以固定频率0
    volatile float pri_auto_pa_value;       //自容模式下压力输出阈值
    volatile uint32_t pri_auto_bound;       //自容模式下的频率输出阈值
    volatile uint32_t enter_count;          //离水计时
    volatile uint32_t leave_count;          //入水计时
}Pri_File_T;

extern Pri_File_T pri_file; 
    
extern FileInfo filearray[MAX_FILE];
extern uint16_t f_haveFileNumber;           //文件个数
 
 
/* Exported types ------------------------------------------------------------*/

extern MKFS_PARM oopt;

/* Exported functions ------------------------------------------------------- */
extern FRESULT Fatfs_mount(uint8_t f_mkfs_flag);              //fat文件系统挂载 盘符0:
extern uint32_t FLASH_FreeSize(void);           //空间占用情况
extern FRESULT createFile(char *filename);         //创建文件
extern FRESULT DeletTheFile(char *filename);   
extern FRESULT my_scan_file(const char* path);  //返回当前文件个数
extern char* file_name_time(void);
extern FRESULT free_space_by_deleting_oldest(const char* path,uint32_t required_bytes);
extern FRESULT FLASH_Fat_Init(uint8_t f_mkfs_flag);
extern void pri_data_to_file(void);

extern void read_all_file_name_func(void);
extern void download_file(void);
extern void delete_file_func(void);
extern void mkfs_disk_func(void);
extern void pri_auto_parameter(void);

#endif





