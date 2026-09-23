#include "tdc_gp22.h"
volatile float True_sv = 0.0;
volatile float Pretend_sv = 0.0;
volatile float kalman_sv = 0.0;
volatile float pri_sv = 0.0;
/*暂存时间测量平均值*/
float  echo_times  = 0.00000f;
float PW1STvalue;
uint16_t        G_tdcStatusRegister=0;
float           G_calibrateResult=0.000f;    
/*晶振校准系数*/
volatile float  bytes_Cal = 0.00000f;           
/*飞跃时间差值*/
float  Source_diff = 5.5f; 

/*系数*/
SVCA_T svca;
/*默认系数*/
float Sound_velocity_coe_def[SOUND_COE_SIZE] = {1.0f,0.0f,1.0f,0.0f,1.0,0.0f};
uint8_t sound_pri_flag;
/*应用系数范围*/
SCS_T scs;
/*默认系数应用范围*/
float Sound_coe_scope_def[SOUND_SCOPE_SIZE] = {1350.0f,1400.0f};
/*平滑处理结构体*/
SMOOTH_T smooth;
/*阈值*/
float  outliers_threshold; 
/*距离挡片距离 */
float  Source_distance; 
int32_t first_v;  //阈值电压
float Sound_velocity_temp;

volatile float decimal_part; 
volatile float total_value;
/****************************************************
    读取flash存储的gp22的 距离 系数 阈值 并设置
*****************************************************/
void gp22_parameter_set(void)
{   
    //读取当前换能器挡板高度
    Source_distance = svp_cmd.PROBE_DISTANCE;
    outliers_threshold = svp_cmd.OUTLIERS_THRESHOLD;
    memcpy(&svca,&svp_cmd.SOUND_VELOCITY_COE_A1,sizeof(svca));
    memcpy(&scs,&svp_cmd.COE_2_SPOCE,sizeof(scs));
}

/**********SPI接口定义-开头***************/
//CLK  PA5
//MISO PA6
//MOSI PA7
//SSN  PA4   RSTN PA8  接
/**************TDC接口定义***************/

/*
PA3  EN_STA
PA4  CS
PA0  INT
PA2  FIRE_IN
PA1  RSTN     超声探头红的接sp1 sp2 白线都接地   
*/
/****************************************************
    gp22的引脚初始化
    (引脚配置、外部中断配置、寄存器配置、参数配置、晶振校准)    
    成功返回晶振校准系数  
    失败返回-1
*****************************************************/
void TDC_GP22_Init(void){
    
    /*片选引脚配置*/
    GPIO_InitTypeDef GPIO_Initure;

    __HAL_RCC_GPIOA_CLK_ENABLE();           //使能GPIOA时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();           //使能GPIOC时钟
    //NSS
    GPIO_Initure.Pin = GPIO_PIN_4;        
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP; //推挽输出
    GPIO_Initure.Pull = GPIO_PULLUP;        //上拉
    GPIO_Initure.Speed = GPIO_SPEED_HIGH;   //高速
    HAL_GPIO_Init(GPIOA, &GPIO_Initure);    //初始化
    //reset C5
    GPIO_Initure.Pin = GPIO_PIN_5;        
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP; //推挽输出
    GPIO_Initure.Pull = GPIO_PULLUP;        //上拉
    GPIO_Initure.Speed = GPIO_SPEED_HIGH;   //高速
    HAL_GPIO_Init(GPIOC, &GPIO_Initure);    //初始化
    
    TDC_GP22_CS = 1;
    /*SPI初始化*/
    SPI1_Init();
    /*外部中断初始化 - 待修改引脚*/
    EXTI_Init();
    /*寄存器配置 */   
    TDC_Init_Reg();
    /*参数设置*/
    gp22_parameter_set();
    /*晶振校准*/
    bytes_Cal =  calibrateResonator();
}



/****************************************************
    读取gp22的片内结果寄存器
*****************************************************/
unsigned char ID_Bytes[7];
void readIDbytesTDCGP22(void){ 
	TDC_GP22_CS = 0;
	delay_us(3);
	SPI_ReadWriteByte(&SPI1_Config,READ_IDBIT);
	//读第一个字节
	ID_Bytes[0] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	//读第二个字节
	ID_Bytes[1] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	//读第三个字节
	ID_Bytes[2] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	//读第四个字节
	ID_Bytes[3] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	//读第五个字节
	ID_Bytes[4] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	//读第六个字节
	ID_Bytes[5] = SPI_ReadWriteByte(&SPI1_Config,0xff); 
	//读第七个字节
	ID_Bytes[6] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	delay_us(3);
	TDC_GP22_CS = 1;
	delay_us(3);
}

/****************************************************
    gp22复位
*****************************************************/
void resetTDCGP22(void){
	//RSTN 置高再拉低，延时后再置高
	TDC_GP22_REST = 1;
	delay_ms(100);
	TDC_GP22_REST = 0;
	delay_ms(200);
	TDC_GP22_REST = 1; 
	delay_ms(100); 
}

//tdc寄存器配置

//0.04/1500=27us(25us~29us)
/*
ClkHSDiv = 0;
DIV_FIRE = 1;

*/
/****************************************************
    gp22配置寄存器初始化
*****************************************************/

uint32_t reg_value[32] = {0x20005004,
                            0x20005104,
                            
};
void TDC_Init_Reg( void ){
    resetTDCGP22();
    initMeasureTDCGP22();
//    configureRegisterTDCGP22( WRITE_REG0, 0xA147E800 );  
    configureRegisterTDCGP22( WRITE_REG0, 0xa147E800 );    
	configureRegisterTDCGP22( WRITE_REG1, 0x21444701 ); 
    configureRegisterTDCGP22( WRITE_REG2, 0XA009C002 );
    /*计算公式*/
    //A009C002  39us  / 500 * 32 * 1000
    //A009A002  38.5us 500
    configureRegisterTDCGP22( WRITE_REG3, 0xE8510303 ); 
        // 1110 1000 0101 0001 0000 0011 0000 0003 0xE8510303   3   4   5
        // 1110 1000 0111 0001 1000 0101 0000 0003  0xE8718503  5   6   7
        // 1110 1000 1000 0001 1100 0110 0000 0003  0XE881C603  6   7  8
        // 1110 1000 1001 0010 0000 0111 0000 0003  0xE8920703  7   8   9
        // 1110 1000 1010 0010 0100 1000 0000 0003` 0xE8A24803  8 1  9   10
        // 1110 1000 1011 0010 1000 1001 0000 0003  0XE8B28903  9   10  11
        // 1110 1000 1100 0010 1100 1010 0000 0003  0XE8C2CA03  10  11  12
        // 1110 1000 1111 0011 1000 1101 0000 0003  0xE8F38D03  13 14 15

    //生成一个32位 指令 配置
    uint32_t reg4_value = read_tdc_frist_v();   //根据当前阈值电压配置 如果为0mV 设置默认10mV
    configureRegisterTDCGP22( WRITE_REG4, reg4_value);     
    configureRegisterTDCGP22( WRITE_REG5, 0x50000005 );
    configureRegisterTDCGP22( WRITE_REG6, /*0xC0416106*/0xC0416006 ); 
    readIDbytesTDCGP22(); 
    
    
    initBuffer(&circular_buffer);        //初始化环形缓冲区
    tdc_pse_init(&tdc_pse);             //初始化伪值处理
}						

/*
    20 1-15
*/
uint32_t read_tdc_frist_v(void){
    int16_t max_v = 35,temp;
    uint32_t reg4 = 0x20000004;
    first_v = svp_cmd.FRIST_WAVE_V;
    if(first_v > 35 || first_v <= 0){    //def
        first_v = 0x05;
        //STMFLASH_Write(FRIST_WAVE_VOLTAGE,(u16*)&first_v,1);
        return reg4 |= (first_v << 8);
    }
    
    if(first_v >= 20){                      //20+mv
        reg4 |= (0x01<<14);
        temp = max_v - first_v;
        if( temp == 15) //20mv
            return reg4;        
        reg4 |= ((0xf-temp)<<8);
        return reg4;
    }else if(first_v > 15 && first_v < 20){  //15-20mv
            //16 20-4 0101 1100
            //17 20-3 0101 1101
            //18 20-2 0101 1110
            //19 20-1 0101 1111
        switch(first_v){
            case 16:
                reg4 |= 0x5C << 8;
                break;
            case 17:
                reg4 |= 0x5D << 8;
                break;
            case 18:
                reg4 |= 0x5E << 8;
                break;
            case 19:
                reg4 |= 0x5F << 8;
                break;
        }
        return reg4;
    }else if(first_v >= 0 && first_v <= 15){    //0-15mv
        reg4 |= (first_v << 8);
        return reg4;
    }
    
    return reg4;
}

void powerOnResetTDCGP22(void){
	TDC_GP22_CS = 0;
	SPI_ReadWriteByte(&SPI1_Config,POWER_ON_RESET);
	TDC_GP22_CS = 1;
}

//晶振校准
void calIbarate(void){
    TDC_GP22_CS = 0;
	SPI_ReadWriteByte(&SPI1_Config,START_CAL_TDC);
	TDC_GP22_CS = 1;
}

void initMeasureTDCGP22(void){
	TDC_GP22_CS = 0;
	SPI_ReadWriteByte(&SPI1_Config,INIT_MEASURE);   //0x70 
	TDC_GP22_CS = 1;
}

void timeFlightStartTDCGP22(void){
	TDC_GP22_CS = 0;
	SPI_ReadWriteByte(&SPI1_Config,START_TOF);
	TDC_GP22_CS = 1;
}

void timeFlightRestartTDCGP22(void){
	TDC_GP22_CS = 0;
	//SPIx_ReadWriteByte(START_TOF_RESTART);
    SPI_ReadWriteByte(&SPI1_Config,START_TOF);  //0x01
	delay_us(1);
	TDC_GP22_CS = 1;
}

/*获取脉冲宽度比*/
float getPW1STvalue(void){
    uint8_t res;
    res = readByteFromRegister(READ_PW1ST);
    //printf("0x%x\r\n",res);
    int integer_part = (res & 0x80) ? 1:0;
    int fraction_part = res & 0x7f;
    float decimal_part = (float)fraction_part/128.0f;
    float total_value = integer_part + decimal_part;
    return total_value;
}

void configureRegisterTDCGP22(u8 opcode_address,uint32_t config_reg_data){
	unsigned char data_byte_lo;
	unsigned char data_byte_mid1;
	unsigned char data_byte_mid2;
	unsigned char data_byte_hi;
  
	data_byte_lo = (u8)(config_reg_data & 0xff);
	data_byte_mid1 = (u8)(config_reg_data>>8 & 0xff);
	data_byte_mid2 = (u8)(config_reg_data>>16 & 0xff);
	data_byte_hi = (u8)(config_reg_data>>24 & 0xff);
  
	TDC_GP22_CS = 0; 
	SPI_ReadWriteByte(&SPI1_Config,opcode_address);
	SPI_ReadWriteByte(&SPI1_Config,data_byte_hi);
	SPI_ReadWriteByte(&SPI1_Config,data_byte_mid2);
	SPI_ReadWriteByte(&SPI1_Config,data_byte_mid1);
	SPI_ReadWriteByte(&SPI1_Config,data_byte_lo); 
	TDC_GP22_CS = 1; 
}

u8 readByteFromRegister(u8 regAddr){
	u8 res = 0;

	TDC_GP22_CS = 0; 
	SPI_ReadWriteByte(&SPI1_Config,regAddr);
	res = SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	TDC_GP22_CS = 1; 

	return res;
}


float readPW1STRegisterTDCGP22(void){
	float result = 0;
	unsigned char resultByte;

	TDC_GP22_CS = 0; 
	delay_us(3);
	SPI_ReadWriteByte(&SPI1_Config,READ_PW1ST);
	//读第一个字节
	resultByte = SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	delay_us(3);
	TDC_GP22_CS = 1;
	delay_us(3);
	if( (resultByte & 0x80) != 0x00 )
	{
		result += 1;
	}
	resultByte = resultByte & 0x7F;
	result = result + (float)(resultByte >> 3)/16 + (float)((resultByte << 1) & 0x0F)/256; 

	return result;
}

uint32_t readRegisterTDCGP22(unsigned char read_opcode_address){
	unsigned long result_read = 0;

	TDC_GP22_CS = 0; 
	SPI_ReadWriteByte(&SPI1_Config,read_opcode_address);
    delay_ms(3);
	//读第一个字节
	result_read |= SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	result_read <<= 8;
	//读第二个字节
	result_read |= SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	result_read <<= 8;
	//读第三个字节
	result_read |= SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	result_read <<= 8;
	//读第四个字节
	result_read |= SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	TDC_GP22_CS = 1;
	delay_us(3);
	return result_read;
}

u8 Test_spi(void){
	u8 rcv0 = 0;
  
    initMeasureTDCGP22();
    configureRegisterTDCGP22(0x81, 0X55323456); 
    rcv0 = readByteFromRegister(0XB5); 
    
	return rcv0;
}
uint16_t readStatusRegisterTDCGP22(void){
	unsigned int result = 0;

	TDC_GP22_CS = 0; 
	//delay_us(3);
	SPI_ReadWriteByte(&SPI1_Config,READ_STAT);
	//读第一个字节
	result |= SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	result <<= 8;
	//读第二个字节
	result |= SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	//delay_us(3);
	TDC_GP22_CS = 1;
	//delay_us(3);
	return result;
}
void gp22_analyse_error_bit(void){
  uint16_t STAT_REG = 0x0000;

  STAT_REG = readStatusRegisterTDCGP22();
  
  //Bit9: Timeout_TDC
  if ((STAT_REG&0x0200)==0x0200) 
    printf("\n-Indicates an overflow of the TDC unit\n");
  //Bit10: Timeout_Precounter
  if ((STAT_REG&0x0400)==0x0400) 
    printf("\n-Indicates an overflow of the 14 bit precounter in MR 2\n");
  //Bit11: Error_open
  if ((STAT_REG&0x0800)==0x0800) 
    printf("\n-Indicates an open sensor at temperature measurement\n");
  //Bit12: Error_short
  if ((STAT_REG&0x1000)==0x1000) 
    printf("\n-Indicates a shorted sensor at temperature measurement\n");
  //Bit13: EEPROM_eq_CREG
  if ((STAT_REG&0x2000)==0x2000) 
    printf("\n-Indicates whether the content of the configuration registers equals the EEPROM\n");
  //Bit14: EEPROM_DED
  if ((STAT_REG&0x4000)==0x4000) 
    printf("\n-Double error detection. A multiple error has been detected whcich can not be corrected.\n");
  //Bit15: EEPROM_Error
  if ((STAT_REG&0x8000)==0x8000) 
    printf("\n-Single error in EEPROM which has been corrected\n");
}
/***************************************************************************
 * DotHextoDotDec function.
 * @brief	Convert Dot HEX to Dot DEC of the char.
 **************************************************************************/
float dotHextoDotDec(unsigned long dotHex){
	float dotDec = 0;
	
	dotDec = (float)((dotHex >> 16) & 0xFFFF) + (float)((dotHex & 0xFFFF) / 65536.0);
	
	return (dotDec*0.25f);
}

float calibrateResonator(void){
	unsigned long temp;
    float temp_Cal;
        
	TDC_GP22_CS = 0;
	SPI_ReadWriteByte(&SPI1_Config,START_CAL_OSC);
	TDC_GP22_CS = 1;
    
	while( INTSign == 0){
    }
	INTSign = 0;
	G_tdcStatusRegister = readStatusRegisterTDCGP22();//0x0011    0XB4
	temp = readRegisterTDCGP22(READ_RES0+(G_tdcStatusRegister&0x0007)-1);
	G_calibrateResult = dotHextoDotDec(temp);
    temp_Cal=122.07f/G_calibrateResult;
    if(temp_Cal == 0.0f){
        return (float)1.0;
    }
    return temp_Cal;
}

/****************************************************
    初始化用于存储时间测量的缓冲区
    参数：环形缓冲区结构体
*****************************************************/
void initBuffer(CircularBuffer* cb){
    int i;
    cb->count = 0;
    cb->index = 0;
    for(i=0;i<CIR_BUFFER_SIZE;i++){
        cb->buffer[i] = 0.00f;
    }
}  

/****************************************************
    添加值到存储时间测量的缓冲区中 如果未满返回当前值   --- 简单移动平均
    参数：环形缓冲区结构体，时间测量的值
*****************************************************/
float addValueAndCalculate(CircularBuffer* cb,float value){ 
#if 1
    int i;
    if(value == 0){ //如果当前时间测量为0，返回0
        return 0.0;
    }
    cb->buffer[cb->index] = value;             
    cb->index = (cb->index + 1) % CIR_BUFFER_SIZE; 
    if(cb->count < CIR_BUFFER_SIZE){    //如果在缓冲区未满的时候 返回当前值
        cb->count++;
        return value;                                                                                                                                                                                                                                                                                                                                                     
    }else{                              //缓冲区满了 返回当前数据与前9个数据做平均
        float sum = 0.00f;
        for(i=0;i<CIR_BUFFER_SIZE;i++){
            sum += cb->buffer[i];
        }
        return sum/CIR_BUFFER_SIZE;
    }
#endif 
} 
/*伪值结构体初始化*/
void tdc_pse_init(tdc_pse_t *tdc_pse_p){
	tdc_pse_p->F_value = 0.0f;	//假伪值为0
	tdc_pse_p->T_value = 0.0f;	//真伪值为0
	memset(tdc_pse_p->F_value_buffer,0,sizeof(tdc_pse_p->F_value_buffer));	//
	memset(tdc_pse_p->F_value_buffer,0,sizeof(tdc_pse_p->F_value_buffer));	//伪值缓冲区清0
	tdc_pse_p->F_value_updata_flag = 0; 
	tdc_pse_p->T_value_updata_flag = 0;
	tdc_pse_p->F_value_count = 0.0;
	tdc_pse_p->T_value_count = 0.0;
	tdc_pse_p->error_count	 = 0;		//真假伪值差距大的异常计数
}

float check(float *buffer,uint16_t size){
	uint16_t i = 0;
	float sum = 0.0f;
	for(i=1;i<size;i++){
		if(fabs(buffer[i] - buffer[i-1]) >= 1.0){
			return 2000.0;
		}
	}
	for(i=0;i<size;i++){
		sum += buffer[i];
	}
	return sum/(float)size;
}
uint8_t zero_flag=0;
uint32_t zero_count = 0;
/*获取真值 返回真值替换值*/
float Pretend_Sv_Value(float Sv){
	float Pretend_temp,avg_value;
	if(Sv == 0.00f && zero_flag == 1){	//连续离水不替换
		return 0.0;
	}
	
	//判断真假伪值数组是否填充完成
	if(tdc_pse.F_value_count == 50){
		/*F_VALUE*/
		//判断当前数据中的每个数据之间差距是否小于1m
		avg_value = check(tdc_pse.F_value_buffer,50);
		if(avg_value != 2000.0f){	//全部小于1m  -> 取平均->更新伪值->计数清空
			//平均
			tdc_pse.F_value = avg_value;//更新伪值为平均值
			/*清空 - 但伪值不会清空直至下一次缓冲区满 替换伪值*/
			tdc_pse.F_value_count = 0;
			memset(tdc_pse.F_value_buffer,0,sizeof(tdc_pse.F_value_buffer));
		}else{	//有大于1m的情况产生	清空缓冲区 重新填充当前数组
			/*清空 - tdc_pse.F_value不变*/
			tdc_pse.F_value_count = 0;
			memset(tdc_pse.F_value_buffer,0,sizeof(tdc_pse.F_value_buffer));
		}		
	}else{	//没有填充完毕 tdc_pse.F_value不变 不会向伪值数组填充0
		if(Sv != 0.0f){
			tdc_pse.F_value_buffer[tdc_pse.F_value_count] = Sv;
			tdc_pse.F_value_count++;
		}
	}
	
//	if(tdc_pse.T_value_count == 500){
//		/*T_VALUE*/
//		avg_value = 0.0f;
//		avg_value = check(tdc_pse.T_value_buffer,500);
//		/*判断真伪值数组是否填充完毕*/
//		if(avg_value){	//全部小于1m  -> 取平均->更新伪值->得出替换后伪值声速->计数清空 ->判断与假伪值的差距<1 替换真伪值为假伪值 >1判断当前声速与真伪值差距差距
//			tdc_pse.T_value = avg_value;
//			/*清空 - 但伪值不会清空直至下一次缓冲区满 替换伪值*/
//			tdc_pse.T_value_count = 0;
//			memset(tdc_pse.T_value_buffer,0,sizeof(tdc_pse.T_value_buffer));
//		}else{
//			/*清空 - tdc_pse.T_value_buffer*/
//			tdc_pse.T_value_count = 0;
//			memset(tdc_pse.T_value_buffer,0,sizeof(tdc_pse.T_value_buffer));
//		}
//	}else{	//没有填充完毕 tdc_pse.T_value不变 不会向伪值数组填充0
//		if(Sv != 0.0){
//			tdc_pse.T_value_buffer[tdc_pse.T_value_count] = Sv;
//			tdc_pse.T_value_count++;
//		}
//	}
	

	if((fabs(Sv - tdc_pse.F_value) > 1.0f) && (tdc_pse.F_value != 0.0f)){
		Pretend_temp = tdc_pse.F_value;	//将返回数据替换为伪值数据
	}else{			//fabs(Sv - tdc_pse.F_value) < 1.0f
		Pretend_temp = Sv;			//小于1m 返回当前真实声速数据
	}
	return Pretend_temp;
}

/****************************************************
    获取一次真实的声速值 /20ms
*****************************************************/
float getTOFValue(void)
{
	float T_sv = 0.0f;
		
	//连续为0标志位
	float times_value = 0.00f;
		/*暂存时间测量*/
	float  echo_times0 = 0.00000f;         

	
    initMeasureTDCGP22();                    
    timeFlightRestartTDCGP22();	
    
    while(INTSign == 0){}
    INTSign = 0;
    G_tdcStatusRegister = (readRegisterTDCGP22(READ_STAT) >> 16);
     
    if((G_tdcStatusRegister&0x0600) != 0){   
        echo_times = 0.000f;
		zero_count++;
		if(zero_count >= outliers_threshold){	//已经完全离水
			zero_flag = 1;
			/*伪值相关全部清零*/
			tdc_pse_init(&tdc_pse);
            /*
                自容模式停止
            */
//            if(nowData.pressure<=0.1f && nowData.pressure>=(-0.1f)){    //压力0+-0.1
//                pri_file.start_flag = 0;    //停止当前自容模式记录的文件
//                pri_file.updata_filename_flag =   0;    //更新标志位置位，为下一次采集做准备
//                pri_file.leave_count++;
//                auto_thrshold.last_pa = 1000.f;
//                auto_thrshold.now_pa = 0.0f;
//            }
            
		}else{									//暂未离水
			zero_flag =  0;
		}
		T_sv = 0.0f;		//当前声速为0
        //printf("okl\r\n");
    }else{
        echo_times0 = dotHextoDotDec(readRegisterTDCGP22(READ_RES0));   
        //times_value = ((echo_times0+0.5005f)*bytes_Cal-5.5f)*1.0000; 
        times_value = echo_times0 * bytes_Cal;        
		/*飞跃时间平均*/
		echo_times = addValueAndCalculate(&circular_buffer,times_value);
		T_sv = Source_distance * 0.010f /(echo_times/2.000f)*1000*1000;	//获取真实声速
		zero_count = 0;		//连续0标志位计数
		zero_flag = 0;		//伪值替换标志位
        
        
//        /* 自容入水计数 */
//        pri_file.leave_count++;
//        if((pri_file.leave_count >= 50) && (nowData.pressure>0.0f)){   //20ms 触发一次 50次为1s
//            pri_file.start_flag = 1;
//            auto_thrshold.now_pa = nowData.pressure;
//            
//        }
    }
	
	return T_sv;
}




//计算当前声速落在了声速范围中的哪一份
/*
    参数：当前声速，开始范围，结束范围，份数
*/
static int find_segment(float value,float start,float end,int segments){
    //计算每一份的长度
    float segment_length =  (end - start) / segments;
    int segment_index = (int)((value-start)/segment_length);
    //由于浮点数运算可能导致索引超出范围，需要限制索引值
    if(segment_index > segments)
        segment_index = segments;
    return segment_index;   //返回当前声速所在的份数
}

//判断声速范围 确认声速系数 得出添加系数后声速数据
float sound_coe_handle(float sound_after_filter){
    float CRITICAL = 1.0f;  //距离声速范围边界临界值
    uint8_t SEGMENT = 100;  //临界点到范围边界的段数
    float temp,segment_temp;
    //printf("%f %f %f \r\n",sound_after_filter,scs.Sound_coe_scope_A1_B1_LL,scs.Sound_coe_scope_A1_B1_HL);
    if((sound_after_filter >= 0) && (sound_after_filter <= scs.Sound_coe_scope_1_2)){   //系数1应用
        if(sound_after_filter >= scs.Sound_coe_scope_1_2 - CRITICAL){  //如果当前声速需要切换声速范围上限前1米 就需要准备开始切换声速系数   
            segment_temp = find_segment(sound_after_filter/*value*/,scs.Sound_coe_scope_1_2 - CRITICAL/*start*/,scs.Sound_coe_scope_1_2/*end*/,SEGMENT/*segment*/);   //eg 1349-1350
            smooth.target_coe_a = svca.Sound_velocity_coefficient_A2;       //目标系数
            smooth.target_coe_b = svca.Sound_velocity_coefficient_B2;
            smooth.alpha_a = (smooth.target_coe_a - svca.Sound_velocity_coefficient_A1)/2;  //需要切换的系数差值
            smooth.alpha_b = (smooth.target_coe_b - svca.Sound_velocity_coefficient_B1)/2;
            smooth.current_coe_a = svca.Sound_velocity_coefficient_A1 + ( segment_temp * smooth.alpha_a / SEGMENT);
            smooth.current_coe_b = svca.Sound_velocity_coefficient_B1 + ( segment_temp * smooth.alpha_b / SEGMENT);
        }else{  //其他情况
            smooth.current_coe_a = svca.Sound_velocity_coefficient_A1;
            smooth.current_coe_b = svca.Sound_velocity_coefficient_B1;
        }
    }else if((sound_after_filter > scs.Sound_coe_scope_1_2) && (sound_after_filter <= scs.Sound_coe_scope_2_3)){      //系数2应用
        if(sound_after_filter <= scs.Sound_coe_scope_1_2 + CRITICAL){      
            segment_temp = find_segment(sound_after_filter/*value*/,scs.Sound_coe_scope_1_2/*start*/,scs.Sound_coe_scope_1_2 + CRITICAL/*end*/,SEGMENT/*segment*/);   //eg 1350-1351
            smooth.target_coe_a = svca.Sound_velocity_coefficient_A2;
            smooth.target_coe_b = svca.Sound_velocity_coefficient_B2;
            smooth.alpha_a = (smooth.target_coe_a - svca.Sound_velocity_coefficient_A1)/2; 
            smooth.alpha_b = (smooth.target_coe_b - svca.Sound_velocity_coefficient_B1)/2;
            smooth.current_coe_a = (svca.Sound_velocity_coefficient_A2 - smooth.alpha_a ) + (segment_temp * smooth.alpha_a / SEGMENT);
            smooth.current_coe_b = (svca.Sound_velocity_coefficient_B2 - smooth.alpha_b ) + (segment_temp * smooth.alpha_b / SEGMENT);
        }else if(sound_after_filter >= scs.Sound_coe_scope_2_3 - CRITICAL){    
            segment_temp = find_segment(sound_after_filter/*value*/,scs.Sound_coe_scope_2_3 - CRITICAL - CRITICAL/*start*/,scs.Sound_coe_scope_2_3/*end*/,SEGMENT/*segment*/);   //eg 1350-1351
            smooth.target_coe_a = svca.Sound_velocity_coefficient_A3;
            smooth.target_coe_b = svca.Sound_velocity_coefficient_B3;
            smooth.alpha_a = (smooth.target_coe_a - svca.Sound_velocity_coefficient_A2)/2; 
            smooth.alpha_b = (smooth.target_coe_b - svca.Sound_velocity_coefficient_B2)/2;
            smooth.current_coe_a = svca.Sound_velocity_coefficient_A2 + ( segment_temp * smooth.alpha_a / SEGMENT);
            smooth.current_coe_b = svca.Sound_velocity_coefficient_B2 + ( segment_temp * smooth.alpha_b / SEGMENT);
        }else{
            smooth.current_coe_a = svca.Sound_velocity_coefficient_A2;
            smooth.current_coe_b = svca.Sound_velocity_coefficient_B2;
        }
    }else if((sound_after_filter > scs.Sound_coe_scope_2_3) && (sound_after_filter <= 1600)){      //系数3应用
        if(sound_after_filter <= scs.Sound_coe_scope_2_3 + CRITICAL){  
            segment_temp = find_segment(sound_after_filter/*value*/,scs.Sound_coe_scope_2_3/*start*/,scs.Sound_coe_scope_2_3 + CRITICAL/*end*/,SEGMENT/*segment*/);   //eg 1350-1351
            smooth.target_coe_a = svca.Sound_velocity_coefficient_A3;
            smooth.target_coe_b = svca.Sound_velocity_coefficient_B3;
            smooth.alpha_a = (smooth.target_coe_a - svca.Sound_velocity_coefficient_A2)/2; 
            smooth.alpha_b = (smooth.target_coe_b - svca.Sound_velocity_coefficient_B2)/2;
            smooth.current_coe_a = (svca.Sound_velocity_coefficient_A3 - smooth.alpha_a ) + ( segment_temp * smooth.alpha_a / SEGMENT);
            smooth.current_coe_b = (svca.Sound_velocity_coefficient_B3 - smooth.alpha_b ) + ( segment_temp * smooth.alpha_b / SEGMENT);
        }else{
            smooth.current_coe_a = svca.Sound_velocity_coefficient_A3;
            smooth.current_coe_b = svca.Sound_velocity_coefficient_B3;
        }
    }else{
        smooth.current_coe_a = 1;
        smooth.current_coe_b = 0;
    }
    /*更换系数后的声速值*/
    temp = 1 * sound_after_filter + 0;    //ax+b
    //printf("%f %f %f %f\r\n",sound_after_filter,smooth.current_coe_a,smooth.current_coe_b,temp);
    return temp;
}




/**************************************************************/
#define MAX_OBSERVATIONS 100    /*滤波数组大小*/
float observations[MAX_OBSERVATIONS];
int num_observations = 0;
int t = 0;
float predicted_state = 1500.0;
float predicted_covariance = 1500.0;
float filtered_state;
float filtered_value;
float filtered_covariance;
int kalman_flag = 0;
/*kalman滤波算法*/
void kalmanFilter(float observation, float* predicted_state, float* predicted_covariance,
    float* filtered_state, float* filtered_covariance){
    float A = 1.0;
    float H = 1.0;
//    double Q = 1e-5;
//    double R = 0.1;
    float Q = 1e-4;
    float R = 0.05;

    float innovation = observation - H * (*predicted_state);
    float innovation_covariance = H * (*predicted_covariance) * H + R;

    float kalman_gain = (*predicted_covariance) * H / innovation_covariance;
    *filtered_state = *predicted_state + kalman_gain * innovation;
    *filtered_covariance = (1 - kalman_gain * H) * (*predicted_covariance);
    
    *predicted_state = A * (*filtered_state);
    *predicted_covariance = A * (*filtered_covariance) * A + Q;
}
    
/*kalman 滤波处理函数*/
float kalman_func(float filtered_spikes){
	float kalman_value=0.0f;
    int i = 1;
    if(filtered_spikes == 0.000f){  //如果当前为0 不会向kalman滤波数组中添加数据，防止kalman滤波数据收敛慢
        kalman_value = 0.000f;
    }else{   
        if (num_observations < MAX_OBSERVATIONS){
            observations[num_observations] = filtered_spikes;
            num_observations++;
        }else{
            for(i = 1; i < MAX_OBSERVATIONS; i++){
                observations[i - 1] = observations[i];
                //observations[MAX_OBSERVATIONS - 1] = sound_velocity;
            }
            observations[MAX_OBSERVATIONS - 1] = filtered_spikes;
            kalman_flag = 1;
        }
        int size_observations = sizeof(observations) / sizeof(observations[0]);  
		
        for (t = 0; t < size_observations; t++){
            kalmanFilter(observations[t], &predicted_state, &predicted_covariance, &filtered_state, &filtered_covariance);
        }
        
        if(kalman_flag == 1){
            kalman_value = filtered_state;
        }else{
            kalman_value = filtered_spikes;
        }
    }
	return kalman_value;
}


void kalman_sound_velocity_posprocessor(void){
    /*获取声速数据*/
    True_sv = getTOFValue();
	Pretend_sv = Pretend_Sv_Value(True_sv);
	kalman_sv = kalman_func(Pretend_sv);
	pri_sv = sound_coe_handle(kalman_sv);
    PW1STvalue = getPW1STvalue()*100;
}





