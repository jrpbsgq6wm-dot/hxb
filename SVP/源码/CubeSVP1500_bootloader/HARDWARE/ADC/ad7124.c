#include "ad7124.h"
#include "delay.h"

uint8_t AD_ID1_REG;		//复位值为0x12或0x14
volatile uint8_t DATA_STATUS;	//获取的通道号
uint8_t AD_Gain=1;

volatile float PT100_TEMP;
volatile float PA;
volatile float pa_refv=0.0,pa_refi=0.0,pa_v=0.0;
    
  
    
void AD7124_PT100_Init(void)
{   
    //复位
    AD7124_Reset();  
     /*ADC_CONTROL寄存器 全功耗 连续工作模式 内部时钟*/
    AD7124_Write_Reg(AD7124_ADC_CTRL_REG ,2,
        AD7124_ADC_CTRL_REG_DATA_STATUS |        //每次数据寄存器读操作之后，状态寄存器内容传输的使能位
        AD7124_ADC_CTRL_REG_POWER_MODE(3)  |     //全功率
        //AD7124_ADC_CTRL_REG_REF_EN          |    //内部基准电压使能   
        AD7124_ADC_CTRL_REG_MODE(0)         |    //0000 连续转换模式
        AD7124_ADC_CTRL_REG_CLK_SEL(0)           //内部时钟
    );    
     /*IO_Control_1 */ 
    AD7124_Write_Reg(AD7124_IO_CTRL1_REG ,3,
        AD7124_IO_CTRL1_REG_PDSW    |       //电桥关断开关控制位
        AD7124_IO_CTRL1_REG_IOUT1(4)   |    //IOUT1激励电流的值   500
        AD7124_IO_CTRL1_REG_IOUT0(4)    |   //IOUT0激励电流的值   500UA
        AD7124_IO_CTRL1_REG_IOUT_CH1(1) |   //IOUT1激励电流的通道选择位 AIN1
        AD7124_IO_CTRL1_REG_IOUT_CH0(0)     //IOUT0激励电流的通道选择位 AIN0  
    );
    AD7124_Write_Reg(AD7124_CH0_MAP_REG,2,0X0000);
//    //PT100   
        /*ch3 reg*/
        AD7124_Write_Reg(AD7124_CH3_MAP_REG  ,2,
            AD7124_CH_MAP_REG_CH_ENABLE |   //通道3使能
            AD7124_CH_MAP_REG_SETUP(3)  |   //SETUP3
            AD7124_CH_MAP_REG_AINP(2)   |   //AIN2
            AD7124_CH_MAP_REG_AINM(3)       //AIN3   
        );
        AD7124_Write_Reg(AD7124_CFG3_REG,2,
            AD7124_CFG_REG_UNIPOLAR     |   //单极性
            AD7124_CFG_REG_BURNOUT(0)   |   //这些位选择传感器开路检测电流源的幅度|
            AD7124_CFG_REG_REF_BUFP     |
            AD7124_CFG_REG_REF_BUFM     |
            AD7124_CFG_REG_AIN_BUFP     |
            AD7124_CFG_REG_AINN_BUFM    |
            AD7124_CFG_REG_REF_SEL(0)   |   // 基准电压选择00 REFV1+/REFV1-    01REFV2+/REFV2-  10 2.5  11 3.3
            AD7124_CFG_REG_PGA(0)           //增益选择位 32
        );
        AD7124_Write_Reg(AD7124_FILT3_REG,3,    
            AD7124_FILT_REG_FILTER(0)   |       //滤波器类型选择位
            AD7124_FILT_REG_POST_FILTER(3)  |   //后置滤波器类型选择位
            AD7124_FILT_REG_FS(384)              //滤波器输出数据速率选择位
        ); 
        
//PA        
        //REFV2+/REFV2-
        AD7124_Write_Reg(AD7124_CH2_MAP_REG  ,2,
            AD7124_CH_MAP_REG_CH_ENABLE |   //通道4使能
            AD7124_CH_MAP_REG_SETUP(2)  |   //SETUP2
            AD7124_CH_MAP_REG_AINP(6)   |   //AIN6
            AD7124_CH_MAP_REG_AINM(7)       //AIN7
        );

        AD7124_Write_Reg(AD7124_CFG2_REG,2,
            AD7124_CFG_REG_UNIPOLAR     |       //单极性
            AD7124_CFG_REG_BURNOUT(0)   |   //这些位选择传感器开路检测电流源的幅度
            AD7124_CFG_REG_REF_SEL(3)   |   //AVDD 基准电压源。
            AD7124_CFG_REG_PGA(0)           //增益选择位 1
        );

        AD7124_Write_Reg(AD7124_FILT2_REG,3,
            AD7124_FILT_REG_FILTER(0)   |
            AD7124_FILT_REG_POST_FILTER(6)  |
            AD7124_FILT_REG_FS(384)
        );
        /*ch1 reg*/
        AD7124_Write_Reg(AD7124_CH1_MAP_REG  ,2,
            AD7124_CH_MAP_REG_CH_ENABLE |   //通道使能
            AD7124_CH_MAP_REG_SETUP(1)  |   //SETUP3
            AD7124_CH_MAP_REG_AINP(4)   |   //AIN4
            AD7124_CH_MAP_REG_AINM(5)       //AIN5   
        );
        AD7124_Write_Reg(AD7124_CFG1_REG,2,
            AD7124_CFG_REG_UNIPOLAR     |   //单极性
            AD7124_CFG_REG_BURNOUT(0)   |   //这些位选择传感器开路检测电流源的幅度|
            AD7124_CFG_REG_AIN_BUFP     |
            AD7124_CFG_REG_AINN_BUFM    |
            AD7124_CFG_REG_REF_SEL(1)   |   // 基准电压选择11 3.3
            AD7124_CFG_REG_PGA(0)           //增益选择位 32
        );
        AD7124_Write_Reg(AD7124_FILT1_REG,3,    
            AD7124_FILT_REG_FILTER(0)   |       //滤波器类型选择位
            AD7124_FILT_REG_POST_FILTER(3)  |   //后置滤波器类型选择位
            AD7124_FILT_REG_FS(384)              //滤波器输出数据速率选择位
        );
         
    
}


    

/**
  * @brief  AD7124的读写SPI操作
  * @param  Data : 需要传输的数据
  * @retval SPI读到的数据
  */


uint32_t DATA=0;
uint32_t Rd_Ary[3];

uint32_t AD7124_Read_Data(uint8_t byte)
{
	for(uint8_t i=0; i<byte; i++)
	{
		Rd_Ary[i] = SPI_ReadWriteByte(&SPI2_Config,0xFF);
	}
	
	DATA = (Rd_Ary[0]<<16) + (Rd_Ary[1]<<8) + Rd_Ary[2];
	
	return DATA;
}

void AD7124_Reset(void)
{
	AD7124_CS = 0;
	//提供大于64个写操作，复位AD7124
	for(uint8_t a=0; a<9; a++)
	{
		SPI_ReadWriteByte(&SPI2_Config,0xFF);
	}
	delay_us(60);
	AD7124_CS = 1;
}

//函数:	读AD7124寄存器
//变量:	addr:寄存器地址 byte:要写入的字节数
//返回值: reg  Ye
uint32_t AD7124_Read_Reg(uint8_t addr,uint8_t byte)
{
	uint32_t reg=0;
	AD7124_CS = 0;
	SPI_ReadWriteByte(&SPI2_Config,0x40 | addr);
	
	switch(byte)
	{
		case 1:
			reg = SPI_ReadWriteByte(&SPI2_Config,0XFF);
			break;
		case 2:
			reg = SPI_ReadWriteByte(&SPI2_Config,0XFF);
			reg <<= 8;
			reg |= SPI_ReadWriteByte(&SPI2_Config,0XFF);
			break;
		case 3:
			reg = SPI_ReadWriteByte(&SPI2_Config,0XFF);
			reg <<= 16;
			reg |= SPI_ReadWriteByte(&SPI2_Config,0XFF);
			reg <<= 8;
			reg |= SPI_ReadWriteByte(&SPI2_Config,0XFF);
			break;
	}
	delay_us(60);
	AD7124_CS = 1;
	return reg;
}

//函数:	写AD7124寄存器
//变量:	addr:寄存器地址 byte:要写入的字节数 data 写入值
//返回值:无
uint16_t AD7124_Write_Reg(uint8_t addr,uint8_t byte,uint32_t data)
{
	uint32_t reg=0;
	AD7124_CS = 0;
	SPI_ReadWriteByte(&SPI2_Config,addr);
	
	switch(byte)
	{
		case 1:
			SPI_ReadWriteByte(&SPI2_Config,data >> 0);
			break;
		case 2:
			SPI_ReadWriteByte(&SPI2_Config,data >> 8);
			SPI_ReadWriteByte(&SPI2_Config,data >> 0);
			break;
		case 3:
			SPI_ReadWriteByte(&SPI2_Config,data >> 16);
			SPI_ReadWriteByte(&SPI2_Config,data >> 8);
			SPI_ReadWriteByte(&SPI2_Config,data >> 0);
			break;
	}
	delay_us(60);
	AD7124_CS = 1;
	return reg;
}


//函数:	读取ID
uint16_t Get_AD7124_ID(void)
{
	uint16_t ID;
	ID=0;
	ID=AD7124_Read_Reg(AD7124_ID_REG,1);
	return ID;
}


float AD7124_READ_DATAREG(void){
    uint32_t Data;
    
    DATA_STATUS = AD7124_Read_Reg(AD7124_STATUS_REG,1);
    
    AD7124_CS = 0;
    SPI_ReadWriteByte(&SPI2_Config,0x42);			//读操作
    Data = AD7124_Read_Data(3);			//Data采集结果
    AD7124_CS= 1;
    return (float)Data;
}

float calculate_value(float temp){
    const float R0 =100.0f;
    const float A = 3.9083e-3f;
    const float B = -5.775e-7f;
    float temperature;
    if(temp >= R0){
        temperature = (-A + __sqrtf(A * A - 4.0f * B * (1.0f - temp / R0))) / (2.0f*B);
    }
    return temperature;
}

void AD7124_DATA(void){
    float data;
    float data_reg = AD7124_READ_DATAREG();
    if(DATA_STATUS == 3){
        data = data_reg * 3900 / (2*0x800000);
        PT100_TEMP = calculate_value(data);
        //printf("----------------------------------------------------------------------- PT100_TEMP %f  R%f datareg%x\r\n",PT100_TEMP,data,DATA_STATUS); 
        if((PT100_TEMP > 50)||(PT100_TEMP < 0)){
             
        }
        //printf("PT100_TEMP %f  R%f datareg%x\r\n",PT100_TEMP,data,DATA_STATUS);
    }
    if(DATA_STATUS == 2){
        pa_refv = (float)3300/1 * (float)data_reg/(2*0X800000);  //uV
        pa_refi = pa_refv/2000;
    }
    if(DATA_STATUS == 1){
        pa_v = (float)pa_refv*(float)data_reg/(2*0X800000);//uV
        PA = (float)(((pa_v)-(0))/23.33f)*25;
#if 0
        printf("pa_refv%fmV  pa_refi%fmA  pa_v%fmV  pa%fBar\r\n",pa_refv,pa_refi,pa_v,PA);
        
        if((PA > 5)){
            
            printf("DATA_STATUS :%x\r\n",DATA_STATUS);
            reg = AD7124_READ_DATAREG();
            if(DATA_STATUS == 2){
                pa_refv = (float)3300/1 * (float)data_reg/(2*0X800000);
                pa_refi = pa_refv/2000;
                printf("pa_refv%fmV pa_refi%fmA pa_v%fmV\t\n",pa_refv,pa_refi,pa_v);
                printf("DATA_STATUS :%x\r\n\r\n\r\n\r\n\r\n\r\n\r\n",DATA_STATUS);
            }
        }
#endif
    }
    
}
SensorData_t lastData;
SensorData_t nowData;    //当前温度压力数据
/*
    限幅滤波
*/
SensorData_t limitFilter(SensorData_t newData){
    SensorData_t filtered;
    
    //判断当前的值是否是0
    if(newData.temperature == 0.0f){
        if(lastData.temperature == 0.0f){
            filtered.temperature = 0;
        }else{
            filtered.temperature = lastData.temperature;
        }
    }else{
        //温度限幅 - 20℃
        if(fabs(newData.temperature - lastData.temperature) > MAX_DEVIATION_T){
            if(lastData.temperature == 0.0f){
                lastData.temperature = newData.temperature;
            }
            filtered.temperature = lastData.temperature;
        }else{
            filtered.temperature = newData.temperature;
        }
        lastData.temperature = filtered.temperature;
    }
    
    
    //判断当前的值是否是0
    if(newData.pressure == 0.0f){
        if(lastData.pressure == 0.0f){
            filtered.pressure = 0.0f;
        }else{
            filtered.pressure = lastData.pressure;
        }
    }else{
        //压力限幅 - 0.2pa - 2m  0.04 0.41 
        /*当前和上一次压力差大于0.2*/        
        if(fabs(newData.pressure - lastData.pressure) > MAX_DEVIATION_T){
            if(lastData.pressure == 0.0f){
                lastData.pressure = newData.pressure;
            }
            filtered.pressure = lastData.pressure;
        }else{
            filtered.pressure = newData.pressure;
        }
        lastData.pressure = filtered.pressure;
    }
//    if(lastData.pressure < 0.1)
//        printf("%f %f %f %f \r\n",filtered.temperature,filtered.pressure,lastData.temperature,lastData.pressure);
    return filtered;
}



/*
    KALMAN
*/    
/*kalman滤波算法*/      //10
void kalmanFilter_ADC(float observation, float* predicted_state, float* predicted_covariance,
    float* filtered_state, float* filtered_covariance){
    float A = 1.0;
    float H = 1.0;
//    double Q = 1e-5;
//    double R = 0.1;
    float Q = 1e-5;
    float R = 1.0;
        // 10-10 = 0
    float innovation = observation - H * (*predicted_state);    
        // 10*0.1+0.1=1.1
    float innovation_covariance = H * (*predicted_covariance) * H + R;
        // 10/1.1=9.09
    float kalman_gain = (*predicted_covariance) * H / innovation_covariance;
        // 
    *filtered_state = *predicted_state + kalman_gain * innovation;
        // 0.5*100 = 50
    *filtered_covariance = (1 - kalman_gain * H) * (*predicted_covariance);
        //5
    *predicted_state = A * (*filtered_state);
        //
    *predicted_covariance = A * (*filtered_covariance) * A + Q;
}
float kaltemp_min = 100.00f;
float kaltemp_max = 100.00f;
float kalpress_min = 0.00f;
float kalpress_max = 50.00f;
float kalmanoutput_t;
float kalmanoutput_p;
uint8_t adckalman_flag;
float PT100_KALMANBUF[50];
float PRESS_KALMANBUF[50];
int num_adcbufcount;
float tstart,pstart;
SensorData_t adc_kalman_func(SensorData_t SensorData){
	SensorData_t adcSensorData;
    int i = 1,t=0;
    if (num_adcbufcount < 50){
        PT100_KALMANBUF[num_adcbufcount] = SensorData.temperature;
        PRESS_KALMANBUF[num_adcbufcount] = SensorData.pressure;
        num_adcbufcount++;
    }else{
        for(i = 1; i < 50; i++){
            PT100_KALMANBUF[i-1] = PT100_KALMANBUF[i];
            PRESS_KALMANBUF[i-1] = PRESS_KALMANBUF[i];
            //observations[MAX_OBSERVATIONS - 1] = sound_velocity;
        }
        PT100_KALMANBUF[num_adcbufcount-1] = SensorData.temperature;
        PRESS_KALMANBUF[num_adcbufcount-1] = SensorData.pressure;
        adckalman_flag++;
    }
    int size_observations = sizeof(PT100_KALMANBUF) / sizeof(PT100_KALMANBUF[0]);  
    
    for (t = 0; t < size_observations; t++){
        kalmanFilter_ADC(PT100_KALMANBUF[t],&kaltemp_min , &kaltemp_max, &tstart, &kalmanoutput_t);
        kalmanFilter_ADC(PRESS_KALMANBUF[t],&kalpress_min , &kalpress_max, &pstart, &kalmanoutput_p);
    }
    
    if(adckalman_flag >= 30){
        adcSensorData.temperature = tstart;
        adcSensorData.pressure = pstart;
    }else{
        adcSensorData.temperature = SensorData.temperature;
        adcSensorData.pressure = SensorData.pressure;
        
    }
	return adcSensorData;
}

void AD7124_Init(void){
    //SPI_Config+GPIO
	SPI2_Init();
    AD7124_PT100_Init();
    
    /*init*/
    nowData.pressure = 0.0f;
    nowData.temperature = 0.0f;
}  

void Filter_func(void){
    AD7124_DATA();
    SensorData_t tempData;
    tempData.pressure = PA,
    tempData.temperature = PT100_TEMP;
    //printf("%f %f\r\n",PA,PT100_TEMP);
    SensorData_t tempp = limitFilter(tempData);
     
    nowData =  adc_kalman_func(tempp);
    nowData.pressure -= 0.4f;
}


