#include "tdc_gp22.h"
volatile float True_sv = 0.0f;
volatile float Pretend_sv = 0.0f;
volatile float kalman_sv = 0.0f;
float pri_sv = 0.0f;
/*�ݴ�ʱ�����ƽ��ֵ*/
float  echo_times  = 0.00000f;
float PW1STvalue;
uint16_t        G_tdcStatusRegister=0;
float           G_calibrateResult=0.000f;    
/*����У׼ϵ��*/
volatile float  bytes_Cal = 0.00000f;           
/*��Ծʱ���ֵ*/
float  Source_diff = 5.5f; 
/*ϵ��*/
SVCA_T svca;
/*Ĭ��ϵ��*/
float Sound_velocity_coe_def[SOUND_COE_SIZE] = {1.0f,0.0f,1.0f,0.0f,1.0,0.0f};
uint8_t sound_pri_flag;
/*Ӧ��ϵ����Χ*/
SCS_T scs;
/*Ĭ��ϵ��Ӧ�÷�Χ*/
float Sound_coe_scope_def[SOUND_SCOPE_SIZE] = {1350.0f,1400.0f};
/*ƽ������ṹ��*/
SMOOTH_T smooth;
/*��ֵ*/
float  outliers_threshold; 
/*���뵲Ƭ���� */
float  Source_distance; 
int32_t first_v;  //��ֵ��ѹ
float Sound_velocity_temp;

volatile float decimal_part; 
volatile float total_value;
/****************************************************
    ��ȡflash�洢��gp22�� ���� ϵ�� ��ֵ ������
*****************************************************/
void gp22_parameter_set(void)
{   
    //��ȡ��ǰ����������߶�
    Source_distance = svp_cmd.PROBE_DISTANCE;
    outliers_threshold = svp_cmd.OUTLIERS_THRESHOLD;
    memcpy(&svca,&svp_cmd.SOUND_VELOCITY_COE_A1,sizeof(svca));
    memcpy(&scs,&svp_cmd.COE_2_SPOCE,sizeof(scs));
}

/**********SPI�ӿڶ���-��ͷ***************/
//CLK  PA5
//MISO PA6
//MOSI PA7
//SSN  PA4   RSTN PA8  ��
/**************TDC�ӿڶ���***************/

/*
PA3  EN_STA
PA4  CS
PA0  INT
PA2  FIRE_IN
PA1  RSTN     ����̽ͷ��Ľ�sp1 sp2 ���߶��ӵ�   
*/
/****************************************************
    gp22�����ų�ʼ��
    (�������á��ⲿ�ж����á��Ĵ������á��������á�����У׼)    
    �ɹ����ؾ���У׼ϵ��  
    ʧ�ܷ���-1
*****************************************************/
void TDC_GP22_Init(void){
    /*Ƭѡ��������*/
    GPIO_InitTypeDef GPIO_Initure;

    __HAL_RCC_GPIOA_CLK_ENABLE();           //ʹ��GPIOAʱ��
    __HAL_RCC_GPIOC_CLK_ENABLE();           //ʹ��GPIOCʱ��
    //NSS
    GPIO_Initure.Pin = GPIO_PIN_4;        
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP; //�������
    GPIO_Initure.Pull = GPIO_PULLUP;        //����
    GPIO_Initure.Speed = GPIO_SPEED_HIGH;   //����
    HAL_GPIO_Init(GPIOA, &GPIO_Initure);    //��ʼ��
    //reset C5
    GPIO_Initure.Pin = GPIO_PIN_5;        
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP; //�������
    GPIO_Initure.Pull = GPIO_PULLUP;        //����
    GPIO_Initure.Speed = GPIO_SPEED_HIGH;   //����
    HAL_GPIO_Init(GPIOC, &GPIO_Initure);    //��ʼ��
    
    TDC_GP22_CS = 1;
    /*�Ĵ������� */   
    TDC_Init_Reg();
    /*��������*/
    gp22_parameter_set();
    /*����У׼*/
    bytes_Cal =  calibrateResonator();
}



/****************************************************
    ��ȡgp22��Ƭ�ڽ���Ĵ���
*****************************************************/
unsigned char ID_Bytes[7];
void readIDbytesTDCGP22(void){ 
	TDC_GP22_CS = 0;
	delay_us(3);
	SPI_ReadWriteByte(&SPI1_Config,READ_IDBIT);
	//����һ���ֽ�
	ID_Bytes[0] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	//���ڶ����ֽ�
	ID_Bytes[1] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	//���������ֽ�
	ID_Bytes[2] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	//�����ĸ��ֽ�
	ID_Bytes[3] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	//��������ֽ�
	ID_Bytes[4] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	//���������ֽ�
	ID_Bytes[5] = SPI_ReadWriteByte(&SPI1_Config,0xff); 
	//�����߸��ֽ�
	ID_Bytes[6] = SPI_ReadWriteByte(&SPI1_Config,0xff);
	delay_us(3);
	TDC_GP22_CS = 1;
	delay_us(3);
}

/****************************************************
    gp22��λ
*****************************************************/
void resetTDCGP22(void){
	//RSTN �ø������ͣ���ʱ�����ø�
	TDC_GP22_REST = 1;
	delay_ms(100);
	TDC_GP22_REST = 0;
	delay_ms(200);
	TDC_GP22_REST = 1; 
	delay_ms(100); 
}

//tdc�Ĵ�������

//0.04/1500=27us(25us~29us)
/*
ClkHSDiv = 0;
DIV_FIRE = 1;

*/
/****************************************************
    gp22���üĴ�����ʼ��
*****************************************************/

uint32_t reg_value[32] = {0x20005004,
                            0x20005104,
                            
};
void TDC_Init_Reg( void ){
    resetTDCGP22();
    initMeasureTDCGP22();
//    configureRegisterTDCGP22( WRITE_REG0, 0xA147E800 );  
    configureRegisterTDCGP22( WRITE_REG0, 0xa147E800 );         //У׼�մɾ���� 32k ʱ�������� 4��ʱ������122.07us
	configureRegisterTDCGP22( WRITE_REG1, 0x21444701 ); 
    configureRegisterTDCGP22( WRITE_REG2, 0XA008C002 );
    /*���㹫ʽ*/
    //A008C002  35us  / 500 * 32 * 1000
    //X/32 * 500 = 35000000
    //A009A002  38.5us 500
    configureRegisterTDCGP22( WRITE_REG3, 0xE8510303 ); 
        // 1110 1000 0101 0001 0000 0011 0000 0003  0xE8510303   3   4   5
        // 1110 1000 0111 0001 1000 0101 0000 0003  0xE8718503  5   6   7
        // 1110 1000 1000 0001 1100 0110 0000 0003  0XE881C603  6   7   8
        // 1110 1000 1001 0010 0000 0111 0000 0003  0xE8920703  7   8   9
        // 1110 1000 1010 0010 0100 1000 0000 0003` 0xE8A24803  8   9   10
        // 1110 1000 1011 0010 1000 1001 0000 0003  0XE8B28903  9   10  11
        // 1110 1000 1100 0010 1100 1010 0000 0003  0XE8C2CA03  10  11  12
        // 1110 1000 1111 0011 1000 1101 0000 0003  0xE8F38D03  13  14  15

    //����һ��32λ ָ�� ����
    uint32_t reg4_value = read_tdc_frist_v();   //���ݵ�ǰ��ֵ��ѹ���� ���Ϊ0mV ����Ĭ��5mV
    configureRegisterTDCGP22( WRITE_REG4, reg4_value); 
    
    configureRegisterTDCGP22( WRITE_REG5, 0x50000005 );
    configureRegisterTDCGP22( WRITE_REG6, /*0xC0416106*/0xC0416006 ); 
    readIDbytesTDCGP22(); 
    initBuffer(&circular_buffer);        //��ʼ�����λ�����
    tdc_pse_init(&tdc_pse);              //��ʼ��αֵ����
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

//����У׼
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

/*��ȡ�����ȱ�*/
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
	//����һ���ֽ�
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
	//����һ���ֽ�
	result_read |= SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	result_read <<= 8;
	//���ڶ����ֽ�
	result_read |= SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	result_read <<= 8;
	//���������ֽ�
	result_read |= SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	result_read <<= 8;
	//�����ĸ��ֽ�
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
	//����һ���ֽ�
	result |= SPI_ReadWriteByte(&SPI1_Config,DUMMY_DATA);
	result <<= 8;
	//���ڶ����ֽ�
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
    ��ʼ�����ڴ洢ʱ������Ļ�����
    ���������λ������ṹ��
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
    ���ֵ���洢ʱ������Ļ������� ���δ�����ص�ǰֵ   --- ���ƶ�ƽ��
    ���������λ������ṹ�壬ʱ�������ֵ
*****************************************************/
float addValueAndCalculate(CircularBuffer* cb,float value){ 
#if 1
    int i;
    if(value == 0){ //�����ǰʱ�����Ϊ0������0
        return 0.0;
    }
    cb->buffer[cb->index] = value;             
    cb->index = (cb->index + 1) % CIR_BUFFER_SIZE; 
    if(cb->count < CIR_BUFFER_SIZE){    //����ڻ�����δ����ʱ�� ���ص�ǰֵ
        cb->count++;
        return value;                                                                                                                                                                                                                                                                                                                                                     
    }else{                              //���������� ���ص�ǰ������ǰ9��������ƽ��
        float sum = 0.00f;
        for(i=0;i<CIR_BUFFER_SIZE;i++){
            sum += cb->buffer[i];
        }
        return sum/CIR_BUFFER_SIZE;
    }
#endif 
} 

/*αֵ�ṹ���ʼ��*/
void tdc_pse_init(tdc_pse_t *tdc_pse_p){
	tdc_pse_p->F_value = 0.0f;	//��αֵΪ0
	tdc_pse_p->T_value = 0.0f;	//��ˮ�������һ�ּ�¼�Ŀ��������׼
	memset(tdc_pse_p->F_value_buffer,0,sizeof(tdc_pse_p->F_value_buffer));	//
	memset(tdc_pse_p->T_value_buffer,0,sizeof(tdc_pse_p->T_value_buffer));	//��ֵ��ʷ��������0
	tdc_pse_p->F_value_updata_flag = 0; 
	tdc_pse_p->T_value_updata_flag = 0;
	tdc_pse_p->F_value_count = 0.0;
	tdc_pse_p->T_value_count = 0.0;
	tdc_pse_p->error_count	 = 0;		//���αֵ������쳣����
}

float check(float *buffer,uint16_t size){
	uint16_t i = 0;
	float sum = 0.0f;
	for(i=1;i<size;i++){
		if(fabs(buffer[i] - buffer[i-1]) >= 5.0){
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
uint8_t fvstart_flag = 0;
volatile uint8_t wite_file_flag = 0;	//0�ر��ļ� 1���ڼ�¼�ļ�

/*��ȡ��ֵ ������ֵ�滻ֵ*/
float Pretend_Sv_Value(float Sv){
	float Pretend_temp,avg_value;
	if(Sv == 0.00f && zero_flag == 1){	//������ˮ���滻
		return 0.0;
	}
	uint8_t fcount;
	//�ж���αֵ�����Ƿ������� - �Ƿ��ȡ����αֵ
	if(tdc_pse.F_value == 0.0f){
		fcount = 100;
	}else{
		fcount = 5;
	}
	
	if(tdc_pse.F_value_count == fcount){
		/*F_VALUE*/
		//�жϵ�ǰ�����е�ÿ������֮�����Ƿ�С10
		avg_value = check(tdc_pse.F_value_buffer,FBUF_SIZE);
		if(avg_value != 2000.0f){	//ȫ��С��10  -> ȡƽ��->����αֵ->�������
			//ƽ��
			tdc_pse.F_value = avg_value;//����αֵΪƽ��ֵ
			/*��� - ��αֵ�������ֱ����һ�λ������� �滻αֵ*/
            fvstart_flag = 1;
			tdc_pse.F_value_count = 0;
			memset(tdc_pse.F_value_buffer,0,sizeof(tdc_pse.F_value_buffer));
            
		}else{	//�д���1m���������	��ջ����� ������䵱ǰ����
			/*��� - tdc_pse.F_value����*/
			tdc_pse.F_value_count = 0;
			memset(tdc_pse.F_value_buffer,0,sizeof(tdc_pse.F_value_buffer));
		}		
	}else{	//û�������� tdc_pse.F_value���� ������αֵ�������0
        /* �״ν׶����ۼ�100�Σ���ֻ�������5������������д���������� */
        tdc_pse.F_value_buffer[tdc_pse.F_value_count % FBUF_SIZE] = Sv;
        tdc_pse.F_value_count++;
	}
    /*
     * �ȶ���ˮ�о�ͬʱ���������ֹ���ģʽ��
     * 1. ������Ч��ˮ�����ﵽ��ֵ��
     * 2. ��ǰ������Ч��
     * 3. fvstart_flag ��ʾαֵ�������ȶ���
     *
     * ֱ��ģʽ�����оݺ�ֻ���빤���Ƴ���״̬�����������ļ���
     * ����ģʽ��ͬһ�о��£��������ύ��ʼ��¼�ļ�����
     */
    pri_file.leave_count++;
    if((pri_file.leave_count >= outliers_threshold) &&
       (Sv != 0.0f) &&
       (fvstart_flag)){
        enter_workmode_led();

        if((svp_cmd.WORK_MODE_FLAG == 0x00) && (workmode_flag == 0U) && (wite_file_flag == 0)){
            pri_start_record();
            wite_file_flag = 1;
        }
    }        
    if(tdc_pse.F_value != 0.0f){ //�����ȡ��Fαֵ ��ֵ��Tαֵ������Tαֵ������0
        /* �������һ���Ѿ����������õļ�ֵ����Ϊ�����쳣ͻ��ʱ�Ŀ��������׼�� */
        tdc_pse.T_value = tdc_pse.F_value;
        //ledflag = work_ledmode;    
    }
	
	//    
    if(tdc_pse.F_value == 0.0f){   
        if((fabs(Sv - tdc_pse.T_value) > 10.0f) && tdc_pse.T_value != 0.0f){
            Pretend_temp = tdc_pse.T_value;
        }else{
            Pretend_temp = Sv;  
        }
    }else{
        if((fabs(Sv - tdc_pse.F_value) > 10.0f)){
            Pretend_temp = tdc_pse.F_value;	//
        }else{			//fabs(Sv - tdc_pse.F_value) < 1.0f
            Pretend_temp = Sv;			//1m 
        }
    }
	return Pretend_temp;
}

/****************************************************
    ��ȡһ����ʵ������ֵ
*****************************************************/
float getTOFValue(void)
{
	float T_sv = 0.0f;
	//����Ϊ0��־λ
	float times_value = 0.00f;
		/*�ݴ�ʱ�����*/
	float  echo_times0 = 0.00000f;         
    initMeasureTDCGP22();                    
    timeFlightRestartTDCGP22();	
    while(INTSign == 0){} 
    INTSign = 0;
    G_tdcStatusRegister = (readRegisterTDCGP22(READ_STAT) >> 16);
    if((G_tdcStatusRegister&0x0600) != 0){   
        echo_times = 0.000f;
		zero_count++;
		if(zero_count >= outliers_threshold){	//�Ѿ���ȫ��ˮ
			zero_flag = 1;
			/*αֵ���ȫ������*/
			tdc_pse_init(&tdc_pse);
			/*����ģʽֹͣ�رմ򿪵��ļ�*/
			if(wite_file_flag == 1){
				pri_stop_record();
				//printf("wite_file_flag = 0");
				wite_file_flag = 0;
			}
			enter_errormode_led(); 		//״̬�� 
			fvstart_flag = 0;           //��ȡ��αֵ��־λ����
		}else{									//��δ��ˮ
			zero_flag =  0;
		}
		T_sv = 0.0f;		//��ǰ����Ϊ0
        pri_file.leave_count = 0;   //��ˮ��������
    }else{
		enter_surveymode_led();	//״̬��
        echo_times0 = dotHextoDotDec(readRegisterTDCGP22(READ_RES0));   
        //����ȡ�Ľ�� * ����У׼->�������� = У׼��Ľ��
        times_value = echo_times0 * bytes_Cal;        
        echo_times = times_value;
        T_sv = Source_distance * 0.010f /(echo_times/2.000f)*1000*1000;	//��ȡ��ʵ���� 42,062,531,544=x*0.01
        zero_count = 0;		//����0��־λ����
        zero_flag = 0;		//αֵ�滻��־λ
    }
	return T_sv;
}

//���㵱ǰ�������������ٷ�Χ�е���һ��
/*
    ��������ǰ���٣���ʼ��Χ��������Χ������
*/
static int find_segment(float value,float start,float end,int segments){
    //����ÿһ�ݵĳ���
    float segment_length =  (end - start) / segments;
    int segment_index = (int)((value-start)/segment_length);
    //���ڸ�����������ܵ�������������Χ����Ҫ��������ֵ
    if(segment_index > segments)
        segment_index = segments;
    return segment_index;   //���ص�ǰ�������ڵķ���
}


//�ж����ٷ�Χ ȷ������ϵ�� �ó����ϵ������������
float sound_coe_handle(float sound_after_filter){
    float temp,temp2,sv;
    float CRITICAL = 1.0f;  //�������ٷ�Χ�߽��ٽ�ֵ
    uint32_t SEGMENT = 1000;  //�ٽ�㵽��Χ�߽�Ķ���
    uint8_t error_flag;
    float segment_temp;
    error_flag = 0;
    
    if((svca.Sound_velocity_coefficient_A3==0) && (svca.Sound_velocity_coefficient_B3==0)){ 
        if(svca.Sound_velocity_coefficient_B2 == 0){
            //�ж��Ƿ�ΪĬ��ϵ��
            if((svca.Sound_velocity_coefficient_A1 == 1) && (svca.Sound_velocity_coefficient_B1 == 1) && (svca.Sound_velocity_coefficient_A2  == 0)){
                temp = sound_after_filter;
                sv = sound_after_filter;
            }else{
                temp = sound_after_filter * sound_after_filter;
                sv = (float)((svca.Sound_velocity_coefficient_A1 * temp)+(svca.Sound_velocity_coefficient_B1 * sound_after_filter) + svca.Sound_velocity_coefficient_A2);
                if((sv <= svp_cmd.COE_2_SPOCE || sv >= svp_cmd.COE_3_SPOCE)){
                    return 0.0f;
                }
            }
        }else{
            //ax^3+bx^2+cx+d
            if((svca.Sound_velocity_coefficient_A1 == 1) && (svca.Sound_velocity_coefficient_B1 == 1) && (svca.Sound_velocity_coefficient_A2  == 1) && (svca.Sound_velocity_coefficient_B2 == 0)){
                temp = sound_after_filter;
                sv = sound_after_filter;
            }else{
                temp = sound_after_filter * sound_after_filter * sound_after_filter;
                temp2 = sound_after_filter * sound_after_filter;
                sv = (float)(svca.Sound_velocity_coefficient_A1 * temp)+(svca.Sound_velocity_coefficient_B1 * temp2)+(svca.Sound_velocity_coefficient_A2 * sound_after_filter)+(svca.Sound_velocity_coefficient_B2);
                if((sv <= svp_cmd.COE_2_SPOCE || sv >= svp_cmd.COE_3_SPOCE)){
                    return 0.0f;
                }
            }
        }
    }else{
        if((sound_after_filter > 0) && (sound_after_filter <= scs.Sound_coe_scope_1_2)){   //ϵ��1Ӧ��
            if(sound_after_filter >= scs.Sound_coe_scope_1_2 - CRITICAL){  //�����ǰ������Ҫ�л����ٷ�Χ����ǰ1�� ����Ҫ׼����ʼ�л�����ϵ��   
                segment_temp = find_segment(sound_after_filter/*value*/,scs.Sound_coe_scope_1_2 - CRITICAL/*start*/,scs.Sound_coe_scope_1_2/*end*/,SEGMENT/*segment*/);   //eg 1349-1350
                smooth.target_coe_a = svca.Sound_velocity_coefficient_A2;       //Ŀ��ϵ��
                smooth.target_coe_b = svca.Sound_velocity_coefficient_B2;
                smooth.alpha_a = (smooth.target_coe_a - svca.Sound_velocity_coefficient_A1)/2;  //��Ҫ�л���ϵ����ֵ
                smooth.alpha_b = (smooth.target_coe_b - svca.Sound_velocity_coefficient_B1)/2;
                smooth.current_coe_a = svca.Sound_velocity_coefficient_A1 + ( segment_temp * smooth.alpha_a / SEGMENT);
                smooth.current_coe_b = svca.Sound_velocity_coefficient_B1 + ( segment_temp * smooth.alpha_b / SEGMENT);
            }else{  //�������
                smooth.current_coe_a = svca.Sound_velocity_coefficient_A1;
                smooth.current_coe_b = svca.Sound_velocity_coefficient_B1;
            }
            
        }else if((sound_after_filter > scs.Sound_coe_scope_1_2) && (sound_after_filter <= scs.Sound_coe_scope_2_3)){      //ϵ��2Ӧ��
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
        }else if((sound_after_filter > scs.Sound_coe_scope_2_3) && (sound_after_filter <= 2000)){      //ϵ��3Ӧ��
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
            error_flag = 1;
        }
        sv = smooth.current_coe_a * sound_after_filter + smooth.current_coe_b;    //ax+b
        if((sv <= 1350 || sv >= 1600) && error_flag)
            return 0.0f;
    }
    return sv;
}




/**************************************************************/
#define MAX_OBSERVATIONS 10    /*�˲������С*/
float observations[MAX_OBSERVATIONS];
int num_observations = 0;
int t = 0;
float predicted_state = 1500.0;
float predicted_covariance = 1500.0;
float filtered_state;
float filtered_value;
float filtered_covariance;
int kalman_flag = 0;
/*kalman�˲��㷨*/
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
    
/*kalman �˲��������*/
float kalman_func(float filtered_spikes){
	float kalman_value=0.0f;
    int i = 1;
    if(filtered_spikes == 0.000f){  //�����ǰΪ0 ������kalman�˲�������������ݣ���ֹkalman�˲�����������
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
    /*��ȡ��������*/
	True_sv = getTOFValue();
	Pretend_sv = Pretend_Sv_Value(True_sv);
	kalman_sv = kalman_func(Pretend_sv);
	pri_sv = sound_coe_handle(Pretend_sv);
	PW1STvalue = getPW1STvalue()*100;
}




