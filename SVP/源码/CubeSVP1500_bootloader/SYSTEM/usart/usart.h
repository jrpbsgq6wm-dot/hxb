/*
    STM32F4 USART CODE
    2024/12/24
    HOU XINBO
*/

#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 
#include "dma.h"
#include "string.h"
#include "stmflash.h"
#include "malloc.h"
#include "eeprom.h"
#include "main.h"



/* DEFINE --------------------------------------------------------------------*/
#define EN_USART1_RX 			    1		//使能（1）/禁止（0）串口1接收
#define RX_BUFFER_SIZE              1029
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
    
    READ_ALL_FILE_NAME            ,     /* 读取磁盘的所有文件名 */
    DOWNLOAD_FILE                 ,     /* 下载文件     */
    DELETE_FILE                   ,     /* 删除指定文件 */
    MKFS_DISK                     ,     /* 格式化磁盘   */
    SET_MODE                      ,     /* 设置工作模式(自容/串口) */
    READ_RTC_DATA_TIME            ,     /* 读取RTC日期和时间*/
    READ_MODE                     ,     /*  读取工作模式相关 */
    WRITE_PA_COE                  ,     /* 设置压力系数 */
    READ_PA_COE                   ,     /* 读取压力系数 */
};

extern uint8_t dma_buffer[1029];  //dma接收缓冲区
extern volatile uint16_t dma_rx_len;         //dma接收数据长度
extern uint8_t rx_buffer[1029];   //数据处理接收缓冲区
extern uint8_t tx_buffer[1029];   //数据处理发送缓冲区
extern volatile uint32_t rx_length;                  //数据接收长度
extern volatile uint8_t frame_receied;               //接收完成标志位
extern UART_HandleTypeDef UART1_Handler;    //UART句柄
extern uint8_t mode_flag;              //串口输出标志位


/* Exported functions ------------------------------------------------------- */
void usart_bound(void);
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
//crc func
extern int CRC16(const void *_nData, uint16_t wLength);
/*小端转化大端_32*/
extern uint32_t htonl(uint32_t value);
/*小端转化大端_16*/
extern uint16_t htons(uint16_t value);
#endif



