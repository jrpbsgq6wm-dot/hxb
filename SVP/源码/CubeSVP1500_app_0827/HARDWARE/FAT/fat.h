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
#define DISK_PATH       "0:"                //�̷�
#define SVP_PATH        "0:/SVP1500"        //�ļ��洢·��


#define YEAR(n)     ((n & 0xfe00)>>9) + 1980    //��
#define MOON(n)     ((n & 0x1E0) >> 5)          //��
#define DATA(n)     (n & 0x1F)                  //��
#define SEC(n)      ((n & 0x1F))                //��
#define MIN(n)      ((n & 0x7e0) >> 5)          //��
#define HOU(n)      ((n & 0xf800) >> 11)        //��

#define MAX_FILE   100

#define REQUIRED_SPACE      10 
extern uint64_t total_bytes,free_bytes,used_bytes;	

/*  ����һ���ṹ�� ���ڴ洢��ǰspi flash�е��ļ���Ϣ */
typedef struct{
    char filename[32];     //�ļ���
    uint32_t filesize;        //�ļ���С
    BYTE attrib;            //�ļ�����
    DWORD mod_time;         //�ļ�����ʱ��
}FileInfo;

typedef struct{
    DS3231_TimeType data_time;        //ʱ��ṹ��
    float pri_sv_v;
    float pt100_v;
    float pa_v;
}SVP_DATA_T;


#define FILE_DATA_BUF_SIZE      2048    //1������53�ֽ�

#define DOWNLOAD_FILE_BUF_SIZE  		                    250
#define DOWNLOAD_FILE_MSG_SIZE_BYTE	                        256
#define DOWNLOAD_FILE_CRC_LEN   DOWNLOAD_FILE_BUF_SIZE+2+1

#define DOWNLOAD_FILE_BUF_SIZE_1K                           1024
#define DOWNLOAD_FILE_MSG_SIZE_1K                           (1+1+2+DOWNLOAD_FILE_BUF_SIZE_1K+2+1)
#define DOWNLOAD_FILE_CRC_LEN_1K                            (1+1+2+DOWNLOAD_FILE_BUF_SIZE_1K)

// ==========�ļ�״̬ö��==========
typedef enum {
    // ȫ��״̬
    STATE_FILE_IDLE = 0,             // ����״̬ - ��ˮ - �����¼�ļ� �д��ļ��ر�
	STATE_READLY,					 // ����׼���� - ��ˮ - ��Ҫ���´����ļ�
    STATE_WRITEING_FILE_DATA,      	 // �ļ���������д����
	STATE_FILE_CLOSING
} Filewritedata_State_t;  

typedef struct{
	Filewritedata_State_t file_state;
    volatile uint8_t wfile_status;                      /*��ǰ�ļ���״̬ -> 1�Ѿ��� 0�ر� */
    char filename[128];                                 //�ļ���
    FSIZE_t file_position;                              //�ļ�����д���λ��    //�ļ���/дλ��ָ��
    FIL file_s;                                         //����ʹ�õ��ļ����
    /*ƹ�һ�����*/
    char wfile_buf_a[FILE_DATA_BUF_SIZE];               //1K������   100ms�������
    volatile uint32_t abuf_index;                       //a�������±�
    char wfile_buf_b[FILE_DATA_BUF_SIZE];               //1K������     
    volatile uint32_t bbuf_index;                       //b�������±�    
    volatile uint8_t data_ok_flag;                      //����׼����ɱ�־λ 1��a������׼���� 2��b������׼����
    volatile uint8_t currnet_num;                       //0����һ��ʹ�� 1��a������   1��b������
    volatile uint8_t wf_flag;                           //0:д�����     1:д��ʧ��
    volatile uint32_t   buf_count;                      //�ݴ�ÿһ��д����ֽ���
    volatile uint32_t   ave_count;                      //ȡƽ������
    volatile uint32_t   wfcount;                        //Ƶ�ʼ���
    volatile uint32_t   write_to_file_byte_count_a;     //д�뵽�ļ��е��ֽڼ���
    volatile uint32_t   write_to_file_byte_count_b;     //д�뵽�ļ��е��ֽڼ���     
    volatile uint8_t    pa_flag;                        //ѹ����Χ��־λ
    volatile uint8_t    auto_wmode;                     //����ģʽ�µĻ�׼ģʽѡ�� ����ѹ���仯1�����Թ̶�Ƶ��0
    volatile float      pri_auto_pa_value;              //����ģʽ��ѹ�������ֵ
    volatile uint32_t   pri_auto_bound;                 //����ģʽ�µ�Ƶ�������ֵ
    volatile uint32_t   enter_count;                    //��ˮ����
    volatile uint32_t   leave_count;                    //��ˮ����
}Pri_File_T;

extern Pri_File_T pri_file;  
extern FileInfo filearray[MAX_FILE];
extern uint16_t f_haveFileNumber;           //�ļ�����
 
typedef struct {
    float last_recorded_depth;  //��һ�μ�¼�����
    float noise_thread;         //��������
    uint8_t is_initialized;     //��ʼ����־λ
} PressureRecorder;
extern PressureRecorder recorder;
 
/* Exported types ------------------------------------------------------------*/

extern MKFS_PARM oopt;

/* Exported functions ------------------------------------------------------- */
extern FRESULT Fatfs_mount(void);              //fat�ļ�ϵͳ���� �̷�0:
extern void FLASH_FreeSize(void);           //�ռ�ռ�����
extern FRESULT createFile(char *filename);         //�����ļ�
extern FRESULT DeletTheFile(char *filename);   
extern FRESULT my_scan_file(const char* path);  //���ص�ǰ�ļ�����
extern char* file_name_time(void);
extern FRESULT free_space_by_deleting_oldest(const char* path, const char* current_file,uint32_t min_free_mb);
extern FRESULT FLASH_Fat_Init(void);
extern void pri_data_to_file(void);
extern void init_filestruct(Pri_File_T *p_file);
extern void read_all_file_name_func(void);
extern void download_file(void);
extern void delete_file_func(void);
extern void mkfs_disk_func(void);
extern void pri_auto_parameter(void);
extern FRESULT f_mkfs_func(void);
extern uint8_t frist_create_file(void);
extern uint8_t w_data_to_file(char *buf,uint32_t count,uint8_t bufflag);
extern void PressureRecorder_Init(PressureRecorder *recorder);
void fat_error_func(void);
void pri_start_record(void);
void pri_stop_record(void);
void pri_set_record_enable(uint8_t enable);
void pri_process_record_requests(void);
void pri_flush_record_now(void);
Filewritedata_State_t file_state_return(void);
#endif





