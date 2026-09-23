
#ifndef _FPGA_INIT_H_
#define _FPGA_INIT_H_

/********************************FPGA UIO0******************************/
#define FPGA_REGISTER_BASEADDR 0x43C00000 // ARM给FPGA进行参数配置的寄存器基地址   size:0x1000	4k

/********************************FPGA DMA UIO1******************************/
#define MEMS_REGISTER_BASEADDR 0x40000000	// DMA预留地址空间	size:0x8000		32k

/********************************FPGA TVG UIO2******************************/
#define TVG_REGISTER_BASEADDR 0x43C20000 // ARM给FPGA进行TVG下发的基地址		size:0x10000	64k

/*********************************DDR UIO3*********************/ 
#define DDR_REGISTER_BASEADDR 0x30000000			// DDR buffer0 		size:10000	64k

/*********************************DDR UIO5*********************/
#define DDR_ORIGINAL_BASEADDR 0x31000000			//原始数据			size:1000000	16M

/*********************************DDR UIO6*********************/	
#define DDR_IQ_BASEADDR 0x33000000					//IQ数据			size:1000000	16M

/*********************************DDR UIO7*********************/
#define DDR_SENSOR_BASEADDR 0x35000000				//传感器数据		size:1000000	16M

/*********************************DDR UIO8*********************/
#define DDR_REGISTER_BASEADDR_1 0x30010000			//DDR buffer1

/*********************************DDR UIO9*********************/
#define DDR_ORIGINAL_BASEADDR_1 0x32000000			//原始数据 buffer1

/*********************************DDR UIO10*********************/
#define DDR_IQ_BASEADDR_1 0x34000000				//IQ数据 buffer1

/*********************************DDR UIO11*********************/
#define DDR_SENSOR_BASEADDR_1 0x36000000			//传感器数据 buffer1

/***************************UIO配置结构体**********************/
typedef struct
{
	int fd;						 // 文件描述符
	char *uiod;					 // 设备驱动名称
	char *sysfs_path_file;		 // 设备驱动所在路径
	unsigned int *physical_addr; // 物理地址
	int mem_size;				 // 大小
	int *mem_ptr;				 // 映射回的指针
} UIO_CONFIG_PARAMETER;

/************************工作状态机WSM_SS_REGISTERS 8 Byte******************/
typedef struct
{
	unsigned int wsm_mod; // 0x08 工作模式寄存器 Bit0，0：CW，1：LFM Bit1，0：升频，1：降频 Bit2，0：原始数据，1：IQ数据
	unsigned int wsm_ct;  // 0x0c 工作周期寄存器
} WSM_REGISTERS;

/***********************************ADC_REGISTERS 8 Byte******************************/
typedef struct
{
	unsigned int adc_sct; // 0x10 adc采样周期寄存器
	unsigned int adc_sn;  // 0x14 adc采样次数寄存器
} ADC_REGISTERS;

/***********************************DAC_REGISTERS******************************/
typedef struct
{
	unsigned int dac_sct; // 0x18 dac采样周期寄存器
	unsigned int dac_sn;  // 0x1c dac采样次数寄存器
} DAC_REGISTERS;

/**************************PWM寄存器PWM_SS_REGISTERS 40 Byte***********************/
typedef struct
{
	unsigned int pwm_bf;		// 0x20 PWM基频寄存器
	unsigned int pwm_lfm;		// 0x24 PWM调频寄存器
	unsigned int pwm_pulse;		// 0x28 PWM脉冲时宽寄存器
	unsigned int pwm_frequency; // 0x2c PWM中心频率
	unsigned int sample_rate;	// 0x30 AD采样率
	unsigned int range;			// 0x34 量程
	unsigned int band_width;	// 0x38 带宽
	unsigned int ping_rate;		// 0x3c PING率
	unsigned int pwm_start;		// 0x40 PWM开始标志位
	unsigned int ADC_SN_af_iq;	// 0x44 IQ抽样后采样次数=ADC_SN/IQ数据抽样因子
	unsigned int ADC_SN_af_ad;	// 0x48 AD抽样后采样次数=ADC_SN/原始数据抽样因子
	unsigned int uart_cfg;			// 0x4c mems配置标志位
	unsigned int uart_cfg_length;	// 0x50 mems配置长度
	unsigned int uart_cfg_baud;		// 0x54 波特率
	unsigned int mems_up_cfg;	 	// 0x58 mems更新标志位
	unsigned int mems_up_send_cfg;	// 0x5c mems更新发送标志位
	// unsigned int mems_up_length;	//mems更新长度
} PWM_REGISTERS;

/****************************FPGA参数配置的寄存器总和 88 Byte**********************/
typedef struct
{
	WSM_REGISTERS wsm_registers_value; // 8		0x0c
	ADC_REGISTERS adc_registers_value; // 8		0x14
	DAC_REGISTERS dac_registers_value; // 8		0x1c
	PWM_REGISTERS pwm_registers_value; // 64	0x5c
} FPGA_CONFIG_PARAMETERS;

/****************************FPGA预留字节**********************/
typedef struct
{
	unsigned int rsv[101]; // 预留字节-1，加了更新标志和波特率后改为102
} FPGA_RSV;

/***************FPGA状态寄存器(当fpga完成本帧的数据传输时此寄存器为0) 12 Byte**************/
typedef struct
{
	unsigned int wstatus; // 0x1F4 FPGA工作状态寄存器
	unsigned int date;	  // 0x1F8 FPGA版本日期
	unsigned int version; // 0x1FC FPGA版本信息
} FPGA_STATUS;

/****************************FPGA所有寄存器 共256字节**********************/
typedef struct
{
	unsigned int wsm_con;						// 0x00 4字节---工作控制寄存器
	unsigned int set_pr;						// 0x04 4字节---参数更新中断
	FPGA_CONFIG_PARAMETERS fpga_registers_200k; // 0x5c 64字节---寄存器结构体变量
	FPGA_RSV rsv_data;							// 0x1F0 424字节---预留
	FPGA_STATUS fpga_sta;						// 0X1FC 12字节---fpga状态寄存器
} FPGA_REGISTERS;

/****************************ARM下发给FPGA的传感器配置**********************/
// typedef struct
// {
// 	unsigned int uart_lenth;	//串口配置命令长度
// 	unsigned int uart_flag;		//串口配置开始
// 	unsigned int GGA_ZDA_BAUD; // GGA和ZDA波特率
// 	unsigned int HEADING_BAUD; // Heading波特率
// 	unsigned int AT_BAUD;	   // AT波特率
// 	unsigned int SVT_BAUD;	   // SVT波特率
// 	unsigned int GGA_ZDA_FH;   // GGA和ZDA帧头
// 	unsigned int HEADING_FH;   // Heading帧头
// 	unsigned int AT_FH;		   // AT帧头
// 	unsigned int SVT_FH;	   // SVT帧头
// 	unsigned int GGA_ZDA_FT;   // GGA和ZDA帧尾
// 	unsigned int HEADING_FT;   // Heading帧尾
// 	unsigned int AT_FT;		   // AT帧尾
// 	unsigned int SVT_FT;	   // SVT帧尾
// }FPGA_CONFIG_SENSER_PARAMETERS;

/**************************************UIO结构体--声明**************************************/
extern UIO_CONFIG_PARAMETER uio_fpga_register;
extern UIO_CONFIG_PARAMETER uio_mems_register;
extern UIO_CONFIG_PARAMETER uio_tvg_register;

extern UIO_CONFIG_PARAMETER uio_baseddr_head;
extern UIO_CONFIG_PARAMETER uio_baseddr_original;
extern UIO_CONFIG_PARAMETER uio_baseddr_iq;
extern UIO_CONFIG_PARAMETER uio_baseddr_sensor;

extern UIO_CONFIG_PARAMETER uio_baseddr_head_1;
extern UIO_CONFIG_PARAMETER uio_baseddr_original_1;
extern UIO_CONFIG_PARAMETER uio_baseddr_iq_1;
extern UIO_CONFIG_PARAMETER uio_baseddr_sensor_1;

extern UIO_CONFIG_PARAMETER uio_baseddr_iq_2;
extern UIO_CONFIG_PARAMETER uio_baseddr_iq_3;

/**************************************与FPGA协议相关结构体**************************************/
extern FPGA_REGISTERS fpga_register_data;	   // 下发给FPGA的参数配置寄存器；结构体变量
extern FPGA_REGISTERS *ptr_fpga_register_data; // 结构体指针
extern FPGA_REGISTERS *ptr_fpga_version_data;
// extern FPGA_CONFIG_SENSER_PARAMETERS *ptr_fpga_mems_config; //mems配置结构体指针

extern void fpga_interface_init(void);

#endif /* _FPGA_INIT_H_ */
