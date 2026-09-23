#include "myiic.h"
#include "delay.h"
#include "usart.h"



KALmanFilter kf_p;
float Pressure_value;

/*压力平均平滑处理初始化*/
void PressureFilter_Init(PressureFilter* pf){
	for(int i=0;i<WINDOWS_SIZE;i++){
		pf->buffer[i] = 0.0f;
	}
	pf->index = 0;
	pf->zero_offset = 0.0f;
	pf->is_tared = 0;
}

/*归零*/
void Pressure_Tare(PressureFilter *pf,float current_raw){
	pf->zero_offset = current_raw;	//记录当前值为零点偏移
	pf->is_tared = 1;				//标记已归零
	//清空滑动窗口
	for(int i=0;i<WINDOWS_SIZE;i++){
		pf->buffer[i] = 0.0f;
	}
	pf->index = 0;
}

/*归零+滤波*/
float Pressure_Update(PressureFilter *pf,float raw_value){
	float compensated;
	//归零补偿
	if(pf->is_tared){
		compensated = raw_value - pf->zero_offset;
	}else{
		compensated = raw_value;
	}
	//存入滑动窗口
	pf->buffer[pf->index] = compensated;
	pf->index = (pf->index + 1) % WINDOWS_SIZE;
	//计算平均
	float sum=0.0f;
	for(int i=0;i<WINDOWS_SIZE;i++){
		sum += pf->buffer[i];
	}
	return sum/WINDOWS_SIZE;
}

void KalmanFilter_init(KALmanFilter* kf ,float Q,float R){
	kf->x = 0.0f;
	kf->P = 1.0f;
	kf->Q = Q;
	kf->R = R;
}

float KalmanFilter_update(KALmanFilter *kf,float z){
	/*预测*/
	kf->x = kf->x;
	kf->P = kf->P + kf->Q;
	//更新
	float K = kf->P / (kf->P + kf->R);	//增益
	kf->x = kf->x + K * (z - kf->x);	//更新最优估
	kf->P = (1 - K ) * (kf->P);
	return kf->x;
}


HAL_StatusTypeDef IIC_WRITE(uint8_t address,uint8_t *data){
	return HAL_I2C_Mem_Write(&Pressure_Config,IIC_ADDR<<1,address,I2C_MEMADD_SIZE_8BIT,data,1,100);
}

HAL_StatusTypeDef IIC_READ(uint8_t address,uint8_t *data){
    return HAL_I2C_Mem_Read(&Pressure_Config,IIC_ADDR<<1,address,I2C_MEMADD_SIZE_8BIT,data,1,100);
}

HAL_StatusTypeDef SendRead_reg(uint8_t regaddr,uint8_t *pdata,uint16_t len){
	return HAL_I2C_Mem_Read(&Pressure_Config,IIC_ADDR<<1,regaddr,I2C_MEMADD_SIZE_8BIT,pdata,len,100);
}

HAL_StatusTypeDef SendWrite_reg(uint8_t regaddr,uint8_t *pdata,uint16_t len){
	return HAL_I2C_Mem_Write(&Pressure_Config,IIC_ADDR<<1,regaddr,I2C_MEMADD_SIZE_8BIT,pdata,len,100);
}

/*
* @brief
*/
uint8_t IIC_PressureSensor_IsReady(void)
{
    if (HAL_I2C_IsDeviceReady(&Pressure_Config,
                               IIC_ADDR << 1,
                               3,
                               20) == HAL_OK)
    {
        return 1U;
    }

    return 0U;
}

void DelayMicroSenconds(uint32_t time){
	uint32_t i=16*time;
	while(i--);
}

void I2C_SCAN(I2C_HandleTypeDef *hi2c){
	for(uint8_t addr=1;addr<128;addr++){
		if(HAL_I2C_IsDeviceReady(hi2c,addr<<1,1,10) == HAL_OK){
			printf("    IIC Device Addr 0x%02x\r\n",addr);
		}else{
			//printf("没有找到任何设备");
		}
	}
}

float Mpa_to_Meter(float p){
	return p * 1000000.0f / 9777.23f;
}

float Calculate_Pressure(uint8_t *reg,float full_scale){
	float LL = 8388608.0f;
	float offset = LL *0.1f;
	float scale  = LL * 0.8f;
	float data = reg[0] * 0x10000 + reg[1] * 0x100 + reg[2];
	
	if(data >= 0x800000){
		return -999.99f;
	}else{
		float offset = 	(float)(1<<23) * 0.1f;
		float scale = 	(float)(1<<23) * 0.8f;
		float pressure = ((data - offset) / scale) * 7.0f;
		return pressure;
	}
}

uint8_t oneflag1=0,oneflag2=0,oneflag3=0;
float offset1=0.0f,offset2=0.0f,offset3=0.0f;
float pa1,pa2,pa3;


/*读取一次压力数*/
void GetPressure(PressureFilter *pf){
	/*发送指*/
	uint8_t cmd = 0x0A;
	uint8_t temp[3] = {0};
	
	if(IIC_WRITE(0x30,&cmd) != HAL_OK){
		printf("write error\r\n");
	}
	/*读取0x06 0x07 0x08*/
	SendRead_reg(0x06,temp,3);
	
	/*转换完成数据*/
	float pa_temp1 = Mpa_to_Meter(Calculate_Pressure(temp,7));
	//printf("%6.3f,",pa_temp1);

	//首先获取5次等传感器稳定
	if(oneflag1 <= 4){
		oneflag1++;
		return ;
	}
	
	//然后在获取五次数据作为传感器归零平均数据
	if(oneflag2 <= 100){
		oneflag2++;
		offset1 += pa_temp1;
		return ;
	}
	
	/*添加归零数*/
	if(pf->is_tared == 0){
		Pressure_Tare(pf,offset1/100);
	}
	
	/*平滑*/
	Pressure_value = Pressure_Update(pf,pa_temp1);

	/*
	 * 压力传感器系数采用 ax^2 + bx + c 的形式进行深度校准。
	 * EEPROM 中三个系数均为 0 时，表示尚未写入有效校准参数；
	 * 此时保持原始滤波压力值，避免深度被错误计算为 0。
	 */
	if ((svp_cmd.PA_COE_A == 0.0f) &&
		(svp_cmd.PA_COE_E == 0.0f) &&
		(svp_cmd.PA_COE_I == 0.0f)){
		Depth = Pressure_value;
	}else{
		Depth = svp_cmd.PA_COE_A * Pressure_value * Pressure_value
			  + svp_cmd.PA_COE_E * Pressure_value
			  + svp_cmd.PA_COE_I;
	}
}
