#ifndef BEAM_H
#define BEAM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/stat.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <netinet/tcp.h>
/*******************************GeoBeam2400头文件2018.10.26刘颖**********************/ 

//定义了DEBUG宏，则为调试模式，若注释此行，则不会打印任何的提示信息，最终产品会注释此行
#define DEBUG 

/********************************FPGA memory UIO0******************************/
#define   FPGA_REGISTER_BASEADDR                 0x43C00000//ARM给FPGA进行参数配置的寄存器基地址    256byte
#define   TVG_REGISTER_BASEADDR                  0x40000000//ARM给FPGA进行TVG下发的基地址           

/*********************************DDR memory UIO1*****************************/
#define   DDR_SHARE_MEM_BASEADDR_ORIGINAL        0x30000000//DDR基地址_原始
#define   DDR_SHARE_MEM_BASEADDR_IQ              0x38000000//DDR基地址_IQ                                                       
#define   SENSOR_SHARE_MEM_BASEADDR_0            0x2F000000//传感器基地址----buffer0
//#define   SENSOR_SHARE_MEM_BASEADDR_1            0x2F100000//传感器基地址
//#define   SENSOR_SHARE_MEM_BASEADDR_2            0x2F200000//传感器基地址
//#define   SENSOR_SHARE_MEM_BASEADDR_3            0x2F300000//传感器基地址
//#define   SENSOR_SHARE_MEM_BASEADDR_4            0x2F400000//传感器基地址
#define   SENSOR_SHARE_MEM_BASEADDR_8            0x2F800000//传感器基地址----buffer1
//#define   SENSOR_SHARE_MEM_BASEADDR_9            0x2F900000//传感器基地址
//#define   SENSOR_SHARE_MEM_BASEADDR_A            0x2FA00000//传感器基地址
//#define   SENSOR_SHARE_MEM_BASEADDR_B            0x2FB00000//传感器基地址
//#define   SENSOR_SHARE_MEM_BASEADDR_C            0x2FC00000//传感器基地址

/*************************大小定义*************************************/
#define   SIZE_OF_USHORT                        2// unsigned short 2字节
#define   SIZE_OF_LONG                          4// unsigned long 和 unsigned long 均为4字节
#define   SIZE_OF_TVG_MAX                       3200//2664//上位机下发的TVG数据的最大字节数2664=666*4
#define   SIZE_OF_STATUS_SEND                   68//ARM发送给上位机的硬件状态信息共计68字节
#define   SIZE_OF_CONFIG_PARA                   3368//2832//上位机下发给ARM的参数配置数据包字节数
#define   SIZE_OF_ONE_PING_SENSOR               256//一帧传感器包邮256字节
#define   SIZE_OF_SENSOR_FIRST                  16//ARM给上位机上传的传感器前面的固定字节数是16，4字节的总字节数+4字节同步状态+4字节预留+4字节GGA_ZDA
#define   SIZE_OF_LEN_OF_SENSOR_DATA            4//4字节传感器数据总长度
#define   SIZE_OF_SENSOR_WITHOUT_SENSOR_DATA    28//4字节同步状态+4字节预留+4字节GGA_ZDA等条数*5
#define   SIZE_OF_CHANNEL_NUM                   208 //AD的IQ数据通道个数
#define   SIZE_OF_CHANNEL_NUM_ORIGINAL          16  //AD原始数据通道数
#define   SAMPLE_FACTOR                         35  //IQ抽样因子
#define   SAMPLE_FACTOR_8                       8    //原始数据的AD

#define   SIZE_OF_DATA_FIRST                    256    //FPGA给ARM上传的参数数据字节数
#define   SIZE_OF_AD_SN_MAX                     160020//160000补齐到35的倍数
#define   SIZE_OF_AD_DATA_MAX_BYTE              3511296//最大字节数
#define   SIZE_OF_SEND_UPPER_PACKAGE_FIRST      128    //ARM发送给上位机的声呐数据包前面的128字节参数

#define   V_SOUND                               1500   //声速1500m/s
#define   PWM_IP_NUM                            16     //PWM初始相位通道数
#define   SONAR_DATA_OFFSET                     128     //ARM发送给上位机的声呐数据包前面有128字节参数

//#define   DDR_SONAR_DATA_OFFSET                 16                     
#define   NUM_OF_IP                             16        //PWM初始相位通道数
#define   CODE_VERSION                          0x00000001//FPGA版本信息
#define   LINUX_VERSION                         2023010701//ARM版本信息

/************************工作状态机WSM_SS_REGISTERS******************/
typedef struct 
{
    unsigned int  wsm_mod;/*工作模式寄存器  Bit0，0：CW(DEF)，1：LFM 
                                            Bit1，0：升频，1：降频(DEF) 
                                            Bit2，0：原始数据，1：处理数据(DEF)*/
    unsigned int  wsm_ct;//工作周期寄存器   1/ping 率*FPGA 时钟频率
}WSM_REGISTERS;

/**************************PWM寄存器PWM_SS_REGISTERS 30*4=120个字节***********************/
typedef struct 
{
   unsigned int  pwm_bf;//PWM基频寄存器         //  Cw: 232 *pwm 频率 / FPGA 时钟频率
                                                //  Lfm: 232 * (pwm 频率±带宽/ 2) / FPGA 时钟频率) -pwm_lfm / 2 / 223 

   unsigned int  pwm_lfm;//PWM调频寄存器        //  Cw:0
                                                //  Lfm: 232* 223* 带宽/  脉宽/ FPGA 时钟频率2

   unsigned int  pwm_pulse;//PWM脉冲时宽寄存器  //  脉宽 * FPGA 时钟频率
   unsigned int	 TransGear;//挡位
   unsigned int  sync_state;//0x48              
                                                /* 
                                                    Bit 0 and Bit 1:
                                                        01：同步输入
                                                        10：同步输出
                                                        00：正常模式
                                                    Bit 2：无
                                                    Bit 3
                                                        0: HEHDT模式101
                                                        1: SYNC模式
                                                    Bit 4
                                                        0：0183
                                                        1 ：SEAPATH
                                                */
   unsigned int  sync_selec;//0x50
                                                /*
                                                    Bit 0
                                                        0：上升沿，1:下降沿
                                                */
   unsigned int  sync_delaytime;//0x58          同步输入输出延迟
   unsigned int  pwm_ip_5;//PWM初始相位寄存器5
   unsigned int  UART_REQ;//串口请求寄存器
   unsigned int  pwm_ip_RSV[10];//
   unsigned int  pwm_start;//PWM开关            开始：1	关闭：0
   unsigned int  power_factor;//功率系数        
   unsigned int  pwm_frequency;//PWM中心频率
   unsigned int  sample_rate;//AD采样率
   unsigned int  range;//量程
   unsigned int  bandwidth;//带宽
   unsigned int  AD_sn_after;//35抽样后的AD_SN
   unsigned int  adc_num_fpga;//ad通道组数选择
   int	phcoe;
   int	final_coe;
   unsigned int	 pitch_oe;
}PWM_REGISTERS;

/***********************************ADC_REGISTERS******************************/
typedef struct 
{
   unsigned int  adc_sct;       //adc采样周期寄存器     FPGA 时钟频率/采样率-1
   unsigned int  adc_sn;        //adc采样次数寄存器     量程 *   2 * 采样率/ 1500-1
}ADC_REGISTERS;

/***********************************DAC_REGISTERS******************************/
typedef struct 
{
   unsigned int  dac_sct;//dac采样周期寄存器            量程 *   2 * 采样率/ 1500/1200
   unsigned int  dac_sn;//dac采样次数寄存器             TVG 采样点数
}DAC_REGISTERS;

/***************FPGA状态寄存器(当fpga完成本帧的数据传输时此寄存器为0)**************/
typedef struct 
{
    unsigned int  wstatus;//FPGA工作状态寄存器
    unsigned int  date;//FPGA版本日期
    unsigned int  version;//FPGA版本信息
}FPGA_STATUS;

/****************************FPGA参数配置的寄存器总和**********************/
typedef struct
{
	WSM_REGISTERS    wsm_registers_value;//2*4
	ADC_REGISTERS    adc_registers_value;//2*4
	DAC_REGISTERS    dac_registers_value;//2*4
	PWM_REGISTERS    pwm_registers_value;//92,不是92应该是108
}FPGA_CONFIG_PARAMETERS;

/****************************ARM下发给FPGA的传感器配置**********************/
typedef struct
{
    unsigned int                    GGA_ZDA_BAUD;//GGA和ZDA波特率
    unsigned int                    HEADING_BAUD;//Heading波特率
    unsigned int                    AT_BAUD; //AT波特率
    unsigned int                    SVT_BAUD;//SVT波特率
    unsigned int                    GGA_ZDA_FH;//GGA和ZDA帧头
    unsigned int                    HEADING_FH;//Heading帧头
    unsigned int                    AT_FH;   //AT帧头
    unsigned int                    SVT_FH;  //SVT帧头
    unsigned int                    GGA_ZDA_FT;//GGA和ZDA帧尾
    unsigned int                    HEADING_FT;//Heading帧尾
    unsigned int                    AT_FT;   //AT帧尾
    unsigned int                    SVT_FT;  //SVT帧尾
}FPGA_CONFIG_SENSER_PARAMETERS;

/****************************FPGA预留字节**********************/
typedef struct
{
    unsigned int                    rsv[11];//预留字节
}FPGA_RSV;


/****************************FPGA所有寄存器 共256字节**********************/
typedef struct
{
	unsigned int                    wsm_con;        //控制FPGA开始/停止工作
    unsigned int                    set_pr;         //参数更新终端寄存器
    FPGA_CONFIG_PARAMETERS          fpga_registers_200k;    // 寄存器结构体变量132字节
    FPGA_RSV                        rsv_data;//56字节
    FPGA_CONFIG_SENSER_PARAMETERS   fpga_sensor;//12*4=48字节
    FPGA_STATUS                     fpga_sta;//12字节
}FPGA_REGISTERS;


/****************************FPGA给ARM的DDR数据前面的共4*64=256字节的参数数据**********************/
typedef struct
{
	unsigned int                    FrameHead;//报头0xaaaa_aaaa
    unsigned int                    AD_sn;//AD采样个数
    unsigned int                    FrameNumber;//帧号
    unsigned int                    PPS_s;//PPS秒计数
    unsigned int                    PPS_10ns;//PPS纳秒计数
    unsigned int                    Data_Type;//数据类型0-原始数据 1-IQ数据
    unsigned int                    Work_Mode;//工作模式0-cw       1-LFM
    unsigned int                    LFM_Mode; //LFM模式0-升频     1降频
    unsigned int                    PWM_FREQUENCY;//Pwm频率
    unsigned int                    PWM_BAND_WIDTH;//PWM带宽
    unsigned int                    PWM_PULSE_WIDTH;;//PWM脉宽
    unsigned int                    SAMPLING_RATE;//采样率
    unsigned int                    RANGE;//量程
    unsigned int                    POWER_FACTOR;//功率系数
    unsigned int                    PWM_START;//PWM开关
    unsigned int                    INIT_PHASE[24];//初始相位
	unsigned int		            TransGear;
	unsigned int		            EPLD_VERSIONS;
    //unsigned int                    INIT_PHASE_ADD[8];//初始相位
	unsigned int                    RSV[22];//预留4*32
    unsigned int                    SmallFrameHead;//小帧头：0xbbbbbbbb为原始数据 0xcccccccc为IQ数据 
}FPGA_DDR_FIRST;

/****************************ARM接收到上位机的参数数据包 40+3200+16+4+8+16+1+64+19 =3368字节****************************/
typedef struct
{
    unsigned int DataType;//0- 表示带通采样数据，1- 表示 IQ 解调数据
    unsigned int WorkMode;//0:CW,1:LFM
    unsigned int LFMMode;//0:升频，1:降频
    unsigned int PWMFreq;//signal Freq
    unsigned int PWMBandWidth;//CW为0
    unsigned int SamplingRate;//current K
    unsigned int Range;//5m/15m/30m/50m/100m/150m/200m/250m/300m 共9档
    int ManualGain;   //手动增益的值，-40dB 至40dB
    int AbsorbGainCoef;//吸收系数值，0 至120 dB/Km
    int SpreadGainCoef;//扩散系数值，0 至60 dB/Km
    int tvgGain[800];//tvgGain[666];//1600
    unsigned int TransGear;//发射档位:最大，中间，常用
    float PulseWidth;//脉宽us
    float PingRate; //设置的帧率,如果这里的值为 1，则为最大频率，为 2，则最大帧率/2，3 则为最大帧频/3 
    float power_factor;// 功率系数，目前是4.8-52.8每间隔1v一个，共49个，加上12、24、36、48这4个，共计53个档位
    unsigned char sensor_fh_GGA_ZDA;//GGA_ZDA帧头1字节
    unsigned char sensor_fh_SVT;//SVT帧头1字节
    unsigned char sensor_fh_HEADING;//HEADING帧头1字节
    unsigned char sensor_fh_AT;//AT帧头1字节    
    unsigned char sensor_ft_GGA_ZDA[2];//GGA_ZDA帧尾2个字节
    unsigned char sensor_ft_SVT[2];//SVT帧尾2个字节
    unsigned char sensor_ft_HEADING[2];//HEADING帧尾2个字节
    unsigned char sensor_ft_AT[2];//AT帧尾2个字节
    unsigned int  sensor_boud_GGA_ZDA;//GGA_ZDA波特率
    unsigned int  sensor_boud_SVT;//SVT波特率
    unsigned int  sensor_boud_HEADING;//HEADING波特率
    unsigned int  sensor_boud_AT;//AT波特率
    char  pwm_start; //PWM开关
    float  pwm_ip[16]; //初始相位，修改为float型
    unsigned int AD_NUM;//原始数据通道组数选择
	char	pitch_oe;   //纵摇稳定
    /*
        Bit1:读取MCP3221ADC请求
        bit2:设置MCP4725DAC请求
        bit3:读取NST175温度请求
        Bit4:向BL24CM2A WWPROM写入8字节请求
        bit5:从BL24CM2A WWPROM读取8字节请求
        bit6:读取MCP4725 DAC寄存器值请求
        （每一位0为默认值 1为开启）
    */
    char    UART_REQ;   
    unsigned short MCP4725_DAC;
	char  Reserved[11];//11 Byte
}__attribute__((packed)) RECV_UPPER_CONFIG_PARAMETERS;

/********************ARM回复给上位机的参数数据包 128+8******************/
typedef struct
{ 
    unsigned short DataType; //2 Byte  数据类型 0-波形，1-原始数据
    unsigned short WorkMode;//2:CW,3:LFM
    unsigned short LFMMode;//2:升频，3:降频
    unsigned int PWMFreq;//signal Freq
    unsigned int PWMBandWidth;//CW为0
    unsigned int PingCount;//4 Byte  帧计数 当前数据帧数
    unsigned short Range;//2 Byte 量程 生成当前ping数据时使用的量程
    unsigned short Sampling;//2 Byte 采样率 当前数据的采样率，单位 k
    unsigned int TimeStamp;//4 Byte 声学数据时间戳 当前ping声学数据的时间偏移值，单位毫秒
    unsigned int PPS_number;//4 Byte pps计数
    float PWMPulseWidth;//CW为0
    float  power_factor; //功率系数
    char pwm_start; //  PWM开关
    float INIT_PHASE[16]; // 初始相位,修改为float型16
    float  temprature;//TMP451温度
    float  roll;//横摇
    float  pitch;//纵摇
    float  heading;//航向
	unsigned int	AD_NUM;//声速
    char   TransGear;//Reserved[1];//5 Byte 预留字节 3个空字节  
    //unsigned int res[17];
    unsigned int SonarDataLength;//4 Byte 声学数据长度 第 8 项声学数据的总长度
}__attribute__((packed)) SEND_UPPER_PACKAGE_FIRST;

/********************回复给上位机的传感器数据包前16字节******************/
typedef struct
{
    unsigned int Senor_total_len;//总个数
    unsigned int SynchState;//同步状态
    char   Reserved[4];// 预留字节
    unsigned int GGA_ZDA_NUM;
}__attribute__((packed)) SEND_UPPER_SENSOR_FIRST;

/****************************回复给上位机的硬件状态信息SS 共计40+4+7+25=76-8=68 字节****************************/
typedef struct
{
        unsigned int FPGAVersion;//FPGA版本号
        unsigned int LinuxDriverVersion;//Linux 驱动版本号
		char PPSWorkStatus; //PPS系统工作状态
        char GPSWorkStatus;//GPS 系统工作状态
        char TimeSysWorkStatus;//时间系统工作状态
        char PostureWorkStatus;//姿态系统工作状态
        char SoundVelocityWorkStatus;//声速系统工作状态
        char CourseWorkStatus;//航向系统
        int  Range;//5m/15m/30m/50m/100m/150m/200m/250m/300m 共9档
        int ManualGain;//手动增益值
        int AbsorbGainCoef;//吸收增益系数
        int SpreadGainCoef;//扩散增益系数
        int TransGear;//发射档位
        char SignalType;//信号类型 0:CW,1:LFM
        float PulseWidth;//脉宽
        int BoardTemperature;//板载温度
        int ChamberTemperature;//内仓环境温度
        int power_factor;
        char pwm_start;
		unsigned int    EPLD_VERSIONS;//EPLD版本号
		char sync_state;//同步模式
		char sync_selec;//同步类型
        char sync_status;
		float sync_delaytime;//时间延迟
        unsigned short device_type;
        char  Reserved[6];//16 Byte
}__attribute__((packed)) SEND_UPPER_SONAR_STATUS;

/***************************UIO配置结构体**********************/
typedef struct
{
    int               fd;    //文件描述符
    char              * uiod;  //设备驱动名称
    char              * sysfs_path_file; //设备驱动所在路径
    unsigned int      * physical_addr; //物理地址
    int               mem_size;  //大小
    int              * mem_ptr;  //映射回的指针
}UIO_CONFIG_PARAMETER;

/***************************串口配置结构体**********************/
typedef struct
{
    char              * dev;
    int               nSpeed;
    int               nBits;
    char              nEvent;
    int               nStop;
}COM_CONFIG_PARAMETER;

/***********************************crc16_tab******************************/
static unsigned short crc16_tab[]={
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440, 
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, 
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, 
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, 
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, 
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441, 
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840, 
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, 
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841, 
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, 
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

#ifdef DEBUG
#define DBG(...) do { fprintf(stderr, "DBG(%s, %s(), %d): ", __FILE__, __func__, __LINE__); fprintf(stderr, __VA_ARGS__); fflush(stderr); } while (0)
#define Debug(fmt, ...) printf((fmt), ##__VA_ARGS__)
#else
#define DBG(...) do { } while (0)
#define Debug(fmt, ...) do { } while (0)
#endif

#define DEFAULT_PORT_SERVER 8000
#define DEFAULT_PORT_CLIENT 8000

#define FPGA_CLK_FREQUENCY 100000000
#define PWM_LFM_FACTOR 3602879702UL
#define PWM_CW_FACTOR 42.94967296
#define N2_32 4294967296
#define N2_23 8388608
#define ENABLE 1
#define DISABLE 0
#define TRUE 0
#define FALSE -1
#define BAUD 38400

#define Address 0x4C
#define I2C_SLAVE 0x0703
#define REG_LOCAL_HIGH 0x00
#define REG_REMOTE_HIGH 0x01
#define REG_LOCAL_LOW 0x15
#define REG_REMOTE_LOW 0x10
#define STATUS_REG 0x02
#define REG_CONFIG_WRITE 0x09

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64

typedef unsigned char uint8;

extern int Fpga_start_mod;
extern int sensor1_num;
extern int sensor2_num;
extern int sensor3_num;
extern int sensor4_num;
extern int sensor5_num;
extern unsigned int total_len;
extern int flag;
extern int flag_timer;
extern int fd_uio6;
extern int irq_on;
extern char DataHead_S[4];
extern char DataTail_S[4];
extern unsigned short send_crc_S;
extern unsigned short send_crc_tmp_S;

#ifdef TEST_TIME
extern struct timeval tv1;
extern struct timezone tz1;
extern struct timeval tv2;
extern struct timezone tz2;
extern struct timeval tv3;
extern struct timezone tz3;
extern struct timeval tv4;
extern struct timezone tz4;
#endif
extern struct timeval start_time;
extern struct timeval end_time;

extern RECV_UPPER_CONFIG_PARAMETERS recv_upper_package;
extern RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package;
extern RECV_UPPER_CONFIG_PARAMETERS cmd_package;
extern SEND_UPPER_SONAR_STATUS send_upper_status_information;
extern SEND_UPPER_SONAR_STATUS *ptr_send_upper_status_information;
extern SEND_UPPER_PACKAGE_FIRST send_to_upper_package_first;
extern SEND_UPPER_PACKAGE_FIRST *ptr_send_to_upper_package_first;
extern SEND_UPPER_SENSOR_FIRST send_to_upper_sensor;
extern SEND_UPPER_SENSOR_FIRST *ptr_send_to_upper_sensor;

extern int Socket_fd_server;
extern int Connect_fd;
extern struct sockaddr_in Servaddr_server;
extern volatile int netStatus;

extern UIO_CONFIG_PARAMETER uio_fpga_register;
extern UIO_CONFIG_PARAMETER uio_tvg_register;
extern UIO_CONFIG_PARAMETER uio_share_mem_IQ;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_0;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_8;
extern UIO_CONFIG_PARAMETER uio_share_mem_original;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_1;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_2;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_3;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_4;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_9;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_A;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_B;
extern UIO_CONFIG_PARAMETER uio_sensor_mem_C;

extern FPGA_REGISTERS fpga_register_data;
extern FPGA_REGISTERS *ptr_fpga_register_data;
extern FPGA_DDR_FIRST fpga_ddr_frame_first;
extern FPGA_DDR_FIRST *ptr_fpga_frame_first;

extern unsigned char ddr_sonar_data[50000000];

extern int fd_icc;
extern unsigned char local_high_value;
extern unsigned char local_low_value;
extern unsigned char remote_high_value;
extern unsigned char remote_low_value;
extern unsigned char status_register;
extern float temprature1;
extern int tem_num;
extern uint8 i2c_read_reg;

extern int fd_compass;
extern char CompassSendBuf[5];
extern char CompassRecvBuf[14];
extern char sum_calculate;
extern char recv_sum;
extern int nread_compass;
extern COM_CONFIG_PARAMETER com_compass;

extern pthread_t thread[3];
extern pthread_mutex_t mut;
extern int syncstatus;

/*
 * 功能: 上位机到 FPGA 配置线程入口。
 * 参数: 无。
 * 返回值: 线程退出时返回 NULL。
 */
void *thread_upper_to_fpga();

/*
 * 功能: FPGA 数据到上位机发送线程入口。
 * 参数: 无。
 * 返回值: 线程退出时返回 NULL。
 */
void *thread_fpga_to_upper();

/*
 * 功能: 传感器读取线程入口。
 * 参数: 无。
 * 返回值: 线程退出时返回 NULL。
 */
void *thread_sensor();

/*
 * 功能: 创建上位机接收、FPGA 上传和传感器读取三个工作线程。
 * 参数: 无。
 * 返回值: 无。
 */
void thread_create(void);

/*
 * 功能: 等待并回收三个工作线程。
 * 参数: 无。
 * 返回值: 无。
 */
void thread_wait(void);

#endif
