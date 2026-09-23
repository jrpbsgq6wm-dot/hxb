#ifndef __SERIAL_H
#define __SERIAL_H

#include "main.h"
#include "delay.h"

#define SERIAL_RX_BUFSIZE           32*2
#define SERIAL_SEND_MSG_TAIL1       0X0D
#define SERIAL_SEND_MSG_TAIL2       0X0A
#define SERIAL_SEND_MSG_HEAD        0X55
#define SERIAL_SEND_MSG_TAIL        0XAA

enum serial_interface
{
    READ_PARAM              = 0x01,     /* read param:bound rs232/485 aml*/
    
    WRITE_PARAM_BOUND       = 0x10,     /* write param:bound */ //串口波特率
    WRITE_PARAM_RS          = 0x11,     /* write param:rs232/485*/ //串口选择232/485
    WRITE_PARAM_AML         = 0x12,     /* write param:aml*/ //串口发送的数据格式
    
    READ_TDC_PARAM          = 0x20,
    READ_SCOPE              = 0X21,
    SET_SCOPE               = 0X22,
    WRITE_TDC_DISTANCE      = 0X2A,
    WRITE_TDC_COE           = 0X2B,
    WRITE_TDC_THRESHOLD     = 0X2C,
    OUTPUT_SOS_FREQ         = 0x2D,
    SET_SOS_FREQ            = 0X2E,      /*设置声速测量频率*/
    SET_FRIST_WAVE_VOLTAGE  = 0X2F,     /*设置第一波阈值电压*/
    
    
    UP_DATA_ONLINE          = 0x30,     /* UP_DATA_ONLINE */ 
    DEFAULT_INIT            = 0x31,     /* Default initialization */ 
    
    OUT_ORIGINAL_DATA       = 0X32, 
    OUT_DEBUG_DATA          = 0X33,
    
    START_OUTPUT            = 0X34, 
    STOP_OUTPUT             = 0X35,
	
    WRITE_FACTORY_TIME		= 0X36,		/*写设备出厂时间*/
    OUT_FACTORY_TIME        = 0x37,     /*输出设备出厂时间*/
    WRITE_DEVICE_SN			= 0X38,		/*写设备SN号*/
    OUT_DEVICE_SN           = 0X39,     /*输出设备SN号*/
};

extern uint8_t cal_ave_flag;


extern uint8_t s_serialRecvBuf[SERIAL_RX_BUFSIZE];
extern uint8_t s_serialSendBuf[SERIAL_RX_BUFSIZE];
extern uint8_t s_Recvlength;
extern unsigned char serial_read_over;
extern volatile uint8_t TimerFlag;


void serialProcess(void);
int CRC16(const void *_nData, uint16_t wLength);
void read_param(void);
void serialInit(void);

void serialParameterReset(void);
void ParameterReset(void);
void ParameterSet(void);
void runParameterSet(void);

void write_param_rs(void);
void write_param_bound(void);
void write_param_aml(void);

void read_tdc_param(void);
void write_tdc_distance(void); 
void write_tdc_coe(void);  
void write_tdc_threshold(void); 

void set_debug_data_out(u16 value);
void gp22ParameterReset(void);
void set_factory_time(void);
void output_factory_time(void);
void set_device_sn(void);
void output_device_sn(void);
void set_sos_freq_func(void);
void read_sos_freq_func(void);
void program_upgrade(void);
void set_frist_wave_v(void);
void read_Scope(void);
void set_Scope(void);


#endif /* __SERIAL_H */

