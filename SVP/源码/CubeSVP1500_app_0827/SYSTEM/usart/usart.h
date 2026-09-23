/*
    STM32F4 USART CODE
    2024/12/24
    HOU XINBO
*/

#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 
#include "rtc.h"
#include "dma.h"
#include "string.h"
#include "stmflash.h"
#include "malloc.h"
#include "tdc_gp22.h"
#include "eeprom.h"
#include "power_app.h"


/* DEFINE --------------------------------------------------------------------*/

#define EN_USART1_RX 			    1		//使能（1）/禁止（0）串口1接收
#define RX_BUFFER_SIZE              1024

#define RX_CMD_HEAD                 0XAA
#define RX_CMD_TAIL                 0x55
#define TX_CMD_HEAD                 0X55
#define TX_CMD_TAIL                 0XAA

#define CMD_HEAD_INDEX              0X00
#define CMD_CODE_INDEX              0X01
#define CMD_DATA_INDEX              0X03

#define CMD_LEN_INDEX               2
#define CMD_CRC_L_INDEX(n)          (n)-1-2
#define CMD_CRC_H_INDEX(n)          (n)-1-1
/* Exported types ------------------------------------------------------------*/
typedef void (*FuncPtr)(void);
typedef struct _uasrt_cmd
{
    uint8_t cmd_code;
    uint32_t addr;                  //数据所在flash地址
	uint16_t cmd_len;               //指令长度（数据+crc+tail）
	uint16_t send_len;              //发送长度
    uint16_t data_Bsize;            //数据所占字节数
    uint8_t  rw;                    //读写位 1r 0w 3文件操作 4工作模式设置
    FuncPtr func;
}uasrt_cmd;

enum serial_interface
{
    READ_PARAM                    ,     /* 读取波特率 */
    READ_PORT                     ,     /* 读取接口类型 */
    READ_FACTORY_DATA             ,     /* 读取出厂日期 */
    READ_SN                       ,     /* 读取SN */
    READ_BLOCK_DIS                ,     /* 读取距离 */
    READ_LEAVE_VOLTAGE            ,     /* 读取离水阈值 */
    READ_KALMAN_BUF_SIZE          ,     /* kalman  滤波样本大小 */
    READ_PRI_BOUND                ,     /* 读取输出频率 */
    READ_TDC_COE                  ,     /* 读取系数 */
    READ_USE_COE_SCOPE            ,     /* 读取应用系数范围 */
    READ_FRIST_WAVE_VOLTAGE       ,     /* 读取第一波电压阈值 */
	
    WRITE_PARAM                   ,     /* 写入波特率 */
    WRITE_PORT                    ,     /* 写入接口类型 */
    WRITE_FACTORY_DATA            ,     /* 写入出厂日期 */
    WRITE_SN                      ,     /* 写入SN */
    WRITE_BLOCK_DIS               ,     /* 写入距离 */
    WRITE_LEAVE_VOLTAGE           ,     /* 写入离水阈值 */
    WRITE_KALMAN_BUF_SIZE         ,     /* kalman  滤波样本大小 */
    WRITE_PRI_BOUND               ,     /* 写入输出频率 */
    WRITE_TDC_COE                 ,     /* 写入系数 */
    WRITE_USE_COE_SCOPE           ,     /* 写入应用系数范围 */
    WRITE_FRIST_WAVE_VOLTAGE      ,     /* 写入第一波电压阈值*/
    WRITE_RTC_DATE_TIME           ,     /* 设置RTC日期 时间 */
/*重构*/ 
    READ_ALL_FILE_NAME            ,     /* 读取磁盘的所有文件名 */
    DOWNLOAD_FILE                 ,     /* 下载文件     */
    DELETE_FILE                   ,     /* 删除指定文件 */
    MKFS_DISK                     ,     /* 格式化磁盘   */
/* 2026 05 19 */
    SET_MODE                      ,     /* 设置工作模式(自容/串口) */
    READ_RTC_DATA_TIME            ,     /* 读取RTC日期和时间*/
    READ_MODE                     ,     /*  读取工作模式相关 */
    WRITE_PA_COE                  ,     /* 设置压力系数 */
    READ_PA_COE                   ,     /* 读取压力系数 */
    ENABLE_PRI                    ,     /* 直读模式下 开始输出*/
    DISABLE_PRI                   ,     /* 直读模式下 停止输出*/
    RESET_CMD                     ,     /* 恢复出厂设置         - 0x22*/
    WRITE_TEMP_COE                ,     /* 设置温度系数         - 0x23*/
    READ_TEMP_COE                 ,     /* 读取温度系数         - 0x24*/
    WRITE_SV_SN                   ,     /* 设置声速传感器序列号 - 0x25*/
    READ_SV_SN                    ,     /* 读取声速传感器序列号 - 0x26*/
    WRITE_TEMP_SN                 ,     /* 设置温度传感器序列号 - 0x27*/
    READ_TEMP_SN                  ,     /* 读取温度传感器序列号 - 0x28*/
    WRITE_PRESSURE_SN             ,     /* 设置压力传感器序列号 - 0x29*/
    READ_PRESSURE_SN              ,     /* 读取压力传感器序列号 - 0x2A*/
    READ_CURRENT_POWER            ,     /* 读取当前电量         - 0x2B */
    BATTREY_CALIBRATION           ,     /* 电压校准             - 0x2C */
    SET_DEPTH_EC                  ,     /* 设置压力精度补偿     - 0x2D */
	READ_DEPTH_EC                 ,     /* 读取压力精度补偿     - 0x2E */
	LINK_STATE					  ,		/* 设备上连接状态       - 0x2F */
	UPDATA_SYS					  , 	/*系统升级 - 0x30*/	
	BATCH_DELETE				, 	/* 批量删除 - 0x 31*/
	UNLINK_STATE				  ,		/* 设备断开连接状态     - 0x32*/
};

extern uint8_t dma_buffer[RX_BUFFER_SIZE];  //dma接收缓冲区
extern volatile uint16_t dma_rx_len;        //DMA 接收长度，最大值为 RX_BUFFER_SIZE，不能使用 8 位类型
extern uint8_t rx_buffer[RX_BUFFER_SIZE];   //数据处理接收缓冲区
extern uint8_t tx_buffer[RX_BUFFER_SIZE];   //数据处理发送缓冲区
extern volatile uint16_t rx_length;         //数据接收长度
extern volatile uint8_t frame_receied;      //接收完成标志位
extern UART_HandleTypeDef UART1_Handler;    //UART句柄
extern volatile uint8_t pri_enable_flag;				//用与在直读模式下进行参数配置等操作数据与回复消息混乱标志位
/* Exported functions ------------------------------------------------------- */

void uart_init(u32 bound);
extern void cmd_ProcessData(void);
extern HAL_StatusTypeDef Send_data(uint8_t *pdata,uint16_t size);
extern uasrt_cmd tx_cmd[];
/*cmd handle*/
extern void read_sn(uasrt_cmd *cmd_buf);                  //读取厂商SN
extern void read_tdc_coe(uasrt_cmd *cmd_buf);             //读取TDC系数
extern void read_use_coe_scope(uasrt_cmd *cmd_buf);       //读取使用系数的区间范围
extern void write_sn(uasrt_cmd *cmd_buf);
extern void write_tdc_coe_or_scpoe(uasrt_cmd *cmd_buf);

/*CRC*/
extern uint16_t CRC_16;
extern uint8_t CRC_LOW;
extern uint8_t CRC_HIG;
extern int validate_download_request(uint8_t *buffer, int len);
extern int validate_download_request_file(uint8_t *buffer, int len);
//crc funcs
extern int CRC16(const void *_nData, uint16_t wLength);
/*小端转化大端_32*/
extern uint32_t htonl(uint32_t value);
/*小端转化大端_16*/
extern uint16_t htons(uint16_t value);
/* 指令回复函数 */
extern void send_response(uint8_t cmd, uint8_t *data, uint8_t data_len);
extern void send_response_file(uint8_t cmd, uint8_t *data, uint16_t data_len);

#endif



