#ifndef __TDC_GP22_H
#define __TDC_GP22_H

#ifdef __cplusplus
extern "C" {
#endif 

#include "main.h"
#include "float.h"

#define TDC_GP22_CS            PAout(4)          //TDC_GP22 cs
#define TDC_GP22_REST          PAout(1)          //TDC_GP22 rest
#define TDC_EN_START           PAout(3)          //TDC_GP22 TDC_EN_START
#define TDC_FIRE_IN            PAout(2)          //TDC_GP22 TDC_FIRE_IN

#define	DUMMY_DATA		           0x00

//指令表宏定义
#define START_TOF                  0x01          // Start TOF measure
#define START_TEMP                 0x02          // Start temperature measure
#define START_CAL_OSC              0x03          // Calibrate oscillator
#define START_CAL_TDC              0x04          // Calibrate TDC
#define START_TOF_RESTART          0x05          // Restart TOF measure
#define START_TEMP_RESTART         0x06          // Restart temperature measure

#define POWER_ON_RESET             0x50          // Power on reset
#define INIT_MEASURE               0x70          // Init measure

#define WRITE_REG0                 0x80          // Write register0 operation address
#define WRITE_REG1                 0x81          // Write register1 operation address
#define WRITE_REG2                 0x82          // Write register2 operation address
#define WRITE_REG3                 0x83          // Write register3 operation address
#define WRITE_REG4                 0x84          // Write register4 operation address
#define WRITE_REG5                 0x85          // Write register5 operation address
#define WRITE_REG6                 0x86          // Write register6 operation address

#define READ_RES0                  0xB0          // Read 32bit result register0 address
#define READ_RES1                  0xB1          // Read 32bit result register1 address
#define READ_RES2                  0xB2          // Read 32bit result register2 address
#define READ_RES3                  0xB3          // Read 32bit result register3 address---
#define READ_STAT                  0xB4          // Read 16bit state register address
#define READ_REG1                  0xB5          // Read 8bit what write to reg1 data, use to test communication
#define READ_IDBIT                 0xB7          // Read TDC ID bit(56 bits)
#define READ_PW1ST                 0xB8          // Read TDC PW1ST(8 bits)


extern float  echo_times;       //时间测量返回值
extern volatile float            bytes_Cal;//校准系数
extern uint16_t         G_tdcStatusRegister;
extern float            G_calibrateCorrectionFactor;
extern float            G_calibrateResult;

extern float  Source_diff;      
extern float  Source_distance;            //挡板距离 
extern uint8_t sound_pri_flag;

extern float PW1STvalue;
//系数结构体
typedef struct Sound_velocity_arr{
    float Sound_velocity_coefficient_A1; 
    float Sound_velocity_coefficient_B1; 
    float Sound_velocity_coefficient_A2; 
    float Sound_velocity_coefficient_B2; 
    float Sound_velocity_coefficient_A3; 
    float Sound_velocity_coefficient_B3;
}SVCA_T;
extern SVCA_T svca;
//默认声速系数
#define SOUND_COE_SIZE     6
extern float Sound_velocity_coe_def[SOUND_COE_SIZE];
//系数范围结构体
typedef struct Sound_coe_scope{
    float Sound_coe_scope_A1_B1_HL;
    float Sound_coe_scope_A1_B1_LL; 
    float Sound_coe_scope_A2_B2_HL;
    float Sound_coe_scope_A2_B2_LL;
    float Sound_coe_scope_A3_B3_HL;
    float Sound_coe_scope_A3_B3_LL;
}SCS_T;
extern SCS_T scs;

//默认声速系数范围
#define SOUND_SCOPE_SIZE   6
extern float Sound_coe_scope_def[SOUND_SCOPE_SIZE];
typedef struct Smooth_coe{
    float current_coe_a;      //当前平滑系数
    float current_coe_b;
    float target_coe_a;       //目标系数
    float target_coe_b;
    float alpha_a;            //步长
    float alpha_b;
    float process_coe_a;      //过程系数
    float process_coe_b;
    uint8_t smooth_flag;    //标志位1:需要做平滑处理 0:不需要做平滑处理
}SMOOTH_T;
extern SMOOTH_T smooth;

/*伪值处理相关*/
typedef struct TDC_PSE_T{
    float F_value;
	float T_value;
	float F_value_buffer[50];
	float T_value_buffer[500];
	uint8_t F_value_updata_flag;	//用来更新真实声速
	uint8_t T_value_updata_flag;	//用来更新假伪值
	uint16_t F_value_count;
	uint16_t T_value_count;
	uint8_t error_count;	
}tdc_pse_t;


#define AVE_BUFFER_SIZE 100
typedef struct TDC_AVE_T{
    uint32_t ave_count;
    uint8_t ave_index;
    float ave_buf[AVE_BUFFER_SIZE];
}ave_t;
extern ave_t svave;

extern tdc_pse_t tdc_pse;
extern int16_t first_v;  //阈值电压
extern float    outliers_threshold;
extern float Sound_velocity_temp;
extern float Sound_velocity_temp2;
extern float sound_velocity_end;

//脉冲宽度比
extern volatile float decimal_part; 
extern volatile float total_value;

extern volatile float True_sv;
extern volatile float Pretend_sv;
extern volatile float kalman_sv;
extern volatile float pri_sv;

//初始化环形缓冲区
void initBuffer(CircularBuffer* cb);   
//填充缓冲区 并计算平均值
float addValueAndCalculate(CircularBuffer* cb,float value);
//返回当前缓冲区已有值的个数
int bufferCount(CircularBuffer* cb);

/*TDC 初始化*/
void TDC_GP22_Init(void);
/*SPI 测试*/
u8 Test_spi(void);
/*设置TDC配置寄存器*/
void configureRegisterTDCGP22(u8 opcode_address,uint32_t config_reg_data);
/*读TDC的一个字节的寄存器*/
u8 readByteFromRegister(u8 regAddr);
/*TDC中断分组配置*/
void TDC_GP22_NVICConfiguration(uint8_t pre, uint8_t sub);
/*晶振校准*/
void calIbarate(void);
/*初始化寄存器*/
void initMeasureTDCGP22(void);
/*Start_TOF*/
void timeFlightStartTDCGP22(void);
void timeFlightRestartTDCGP22(void);
/*获取晶振校准系数*/
void calibrateResonator(void);
/*Power_On_Reset*/
void powerOnResetTDCGP22(void);
/*获取脉冲宽度比*/
float getPW1STvalue(void);
/*进行一次声速测量*/
float getTOFValue(void);
/*gp22配置寄存器初始化*/
void TDC_Init_Reg( void );
/*STAT寄存器溢出标志位*/
void gp22_analyse_error_bit(void);
/*Dot HEX to Dot DEC*/
float dotHextoDotDec(unsigned long dotHex);
/*读取TDC四个字节*/
uint32_t readRegisterTDCGP22(unsigned char read_opcode_address);
/*获得一次声速*/
void kalman_sound_velocity_posprocessor(void) ;
/*读取flash存储的gp22的 距离 系数 阈值*/
void gp22_parameter_set(void);
/*gp22复位*/
void resetTDCGP22(void);
/*伪值处理*/
void tdc_pse_init(tdc_pse_t *tdc_pse_p);
void pesudo_true_process(float current_value);
/*kalman滤波数组填充与处理*/
float kalman_func(float filtered_spikes);
/*多系数处理*/
float sound_coe_handle(float sound_after_filter);
/*根据设置配置当前第一波电压阈值*/
uint32_t read_tdc_frist_v(void);
extern void Tdc_Ave_init(ave_t* svave);
extern float Tdc_Ave_func(ave_t* svave,float sv);
#ifdef __cplusplus
}
#endif

#endif
