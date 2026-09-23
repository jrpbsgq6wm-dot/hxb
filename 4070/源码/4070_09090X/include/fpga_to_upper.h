/*******************************FPGA->ARM->UPPER头文件**********************/

#ifndef _FPGE_TO_UPPER_H_ // 判断该文件内有没有定义过这个宏

#define _FPGE_TO_UPPER_H_ // 若没有则定义

/**************************************XADC片上温度传感器相关**************************************/
// #define SLEEP_TIME 2
// #define TEMP_DATA_SIZE 5
// #define SYS_PATH_TEMP_IIO_VALUE "/sys/bus/iio/devices/iio:device0/in_temp0_raw"
// #define VOLTAGE_REF 503.975
// #define ADC_RESOLUTION 4096
// #define TEMP_OFFSET -273.15
/**************************************PT100温度传感器相关**************************************/
#define PT100_SYS_PATH_TEMP_VALUE "/sys/bus/iio/devices/iio:device0/in_voltage8_vpvn_raw"
#define TEMP_DATA_SIZE 5
#define A	3.9083*pow(10,-3)
#define B	(-5.775)*pow(10,-7)

#define coef2_1	0.213791246028241
#define coef2_2	(-8.49485958686718)
#define coef2_3	1577.75287036821

/****************************FPGA给ARM的DDR数据头的共256字节的参数数据**********************/
typedef struct
{
	unsigned int FrameHead;		  // 报头0xaaaa_aaaa
	unsigned int Work_Mode;		  // 工作模式	0-cw       1-LFM
	unsigned int Work_Period;	  // 工作周期
	unsigned int ADC_sp;		  // AD采样周期
	unsigned int ADC_sn;		  // AD采样次数
	unsigned int DAC_sp;		  // DA采样周期
	unsigned int DAC_sn;		  // DA采样次数
	unsigned int PWM_FREQUENCY;	  // PWM频率	
	unsigned int PWM_LFM;		  // PWM调频	0-升频     1降频
	unsigned int PWM_PULSE_WIDTH; // PWM脉宽
	unsigned int PWM_CF;		  // PWM中心频率
	unsigned int ADC_sf;		  // 采样率
	unsigned int RANGE;			  // 量程
	unsigned int PWM_BAND_WIDTH;  // PWM带宽
	unsigned int PING_rate;		  // PING率
	unsigned int PPS_10ns;		  // PPS纳秒计数_单位10ns，声学数据时间戳
	unsigned int PPS_s;		  	  // PPS计数
	unsigned int FrameNumber;	  // 帧号---帧计数
	unsigned int PWM_START;		  // PWM开关
	unsigned int SAMPL_sf_IQ;	  // IQ抽样后采样次数
	unsigned int SAMPL_sf_AD;	  // AD抽样后采样次数
	unsigned int RSV[42];		  // 预留4*42=168   
	unsigned int SmallFrameHead;  // 小帧头：0xcccc_cccc为原始数据 0xbbbb_bbbb为IQ数据
} FPGA_DDR_FIRST;

/*********************FPGA给ARM的DDR_SENSOR数据头的共16字节的参数数据*******************/
typedef struct
{
	unsigned int Sensor_FrameHead; // 传感器帧头
	unsigned int Sensor_FrameNum;  // 传感器帧计数
	unsigned int Sensor_PPS_s;	   // PPS计数
	unsigned int Sensor_PPS_10ns;  // PPS纳秒计数_单位10ns，声学数据时间戳
} FPGA_DDR_SENSOR_FIRST;

/********************ARM回复给显控的参数数据包 ******************/
typedef struct
{
#if 1
	/*260119新增 116byte*/
	char Probe_mode;				//探头模式 0：单探头  1：双探头
	char Probe_Logo;				//探头标识0：主探头  1：从探头
	float Install_angle;			//安装角度
	char Ping_mode;					//0：单ping交替模式 	1：全ping模式
	char INS_mode;					//内外惯导选择	0：内置惯导
	unsigned int UP_DataType;		// 数据类型 0-原始数据，1-IQ解调数据 4 Byte
	unsigned int UP_Work_Mode;		// 工作模式0-cw 
	unsigned int UP_PWM_FREQUENCY;	// PWM频率
	unsigned int UP_PWM_LFM;		// PWM调频	0-升频     1降频
	float UP_PWM_PULSE_WIDTH;		// PWM脉宽  float
	unsigned int UP_PWM_CF;			// PWM中心频率
	unsigned int UP_ADC_sf;			// 采样率
	unsigned int UP_PWM_START;		// pwm开关
	unsigned int UP_RANGE;			// 量程
	unsigned int UP_PWM_BAND_WIDTH; // PWM带宽
	float UP_PING_rate;				// PING率  float
	unsigned int UP_PPS_10ns;		// PPS纳秒计数_单位10ns，声学数据时间戳
	unsigned int UP_PPS_s;			// PPS计数
	unsigned int UP_FrameNumber;	// 帧号--帧计数
	float UP_PT100;					// 声速 温度，单位摄氏度℃ -舱外温度
	unsigned int UP_SAMPL_sf_AD;	//AD抽样后采样次数
	float UP_TMP451;				//舱内温度
	unsigned int UP_RSV[9];		 // 预留4*9
	unsigned int UP_SonarDataLength; // 声学数据的总长度
#else
	
#endif
} __attribute__((packed)) SEND_UPPER_SONAR_FIRST;


#define printf_time(fmt, ...) do { \
    struct timeval _tv; \
    gettimeofday(&_tv, NULL); \
    struct tm *_tm = localtime(&_tv.tv_sec); \
    char _buf[32]; \
    strftime(_buf, sizeof(_buf), "%H:%M:%S", _tm); \
    printf("[%s.%03ld] " fmt, _buf, _tv.tv_usec / 1000, ##__VA_ARGS__); \
} while(0)

/********************回复给干端的传感器数据包前16字节******************/
typedef struct
{
	unsigned int UP_SensorData_len; // 传感器数据总长度
	unsigned int UP_SynchState;		// 同步状态 0：同步出现问题，1：同步正常
	char UP_Sensor_RSV[4];			// 预留字节
} __attribute__((packed)) SEND_UPPER_SENSOR_FIRST;

extern void Copy_Fpga_SendTo_Upper(void);

#endif //!_FPGE_TO_UPPER_H_