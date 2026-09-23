#include "ltc2943.h"
#include "usart.h" 

const float LTC2943_CHARGE_lsb = 0.34E-3;
const float LTC2943_VOLTAGE_lsb = 1.44E-3;          //
const float LTC2943_CURRENT_lsb = 29.3E-6;          //
const float LTC2943_TEMPERATURE_lsb = 0.25;
const float LTC2943_FULLSCALE_VOLTAGE = 23.6;
const float LTC2943_FULLSCALE_CURRENT = 60E-3f;      //0.060
const float LTC2943_FULLSCALE_TEMPERATURE = 510;


uint8_t LTC2943_Write(uint8_t i2c_address,uint8_t adc_command,uint8_t code){
    return HAL_I2C_Mem_Write(&IIC_Config,i2c_address,adc_command,I2C_MEMADD_SIZE_8BIT,(uint8_t *)&code,1,0xffff);
}

uint8_t LTC2943_Write_16_Bits(uint8_t i2c_address,uint8_t adc_command,uint16_t code){
    return HAL_I2C_Mem_Write(&IIC_Config,i2c_address,adc_command,I2C_MEMADD_SIZE_8BIT,(uint8_t *)&code,2,0xffff);
}
uint8_t LTC2943_Read(uint8_t i2c_address,uint8_t adc_command,uint8_t* adc_code){
    return HAL_I2C_Mem_Read(&IIC_Config,i2c_address,adc_command,I2C_MEMADD_SIZE_8BIT,adc_code,1,0xffff);
}
uint8_t LTC2943_Read_16_Bits(uint8_t i2c_address,uint8_t adc_command,uint16_t* adc_code){
    return HAL_I2C_Mem_Read(&IIC_Config,i2c_address,adc_command,I2C_MEMADD_SIZE_8BIT,(uint8_t *)adc_code,2,0xffff);
}
/*计算库伦电荷*/
float LTC2943_code_to_coulombs(uint16_t adc_code,float resistor,uint16_t prescallar){
    float coulomb_charge;
    //coulomb_charge = LTC2943_CHARGE_lsb*(0.5f/prescallar)*4096/4096;
    coulomb_charge = 1000+(float)(adc_code+LTC2943_CHARGE_lsb*prescallar*10E-3f)/(resistor*4096);
    coulomb_charge = coulomb_charge * 3.6f;
    return (coulomb_charge);
}
/*
    计算mAh
    qlsb代表累计电荷寄存器中每个计数对应的电荷量，其值由检测电阻和预分频因子决定M=4096
        公式：qlsb = 0.340mAh * (50mR/Rsense) * (M/4096)
        检测电阻 50mR ，预分频因子4096
        公式简化：qlsb 0.340mAh * (50mR/Rsense)

        检测电阻 10mR ，预分频因子4096
        公式:qlsb= 0.340 * 5 * 1 = 1.7 

    实际电荷量mAh = 寄存器值 * qlsb

    //但是要通过校准与电池的实际容量关联 - 我的电阻为0.01R 10mR
    mAh = 寄存器值 * C总/65535
*/
float LTC2943_code_to_mAh(uint16_t adc_code,float resistor,uint16_t prescallar){
    float mAh_charge;
    //printf("%d\r\n",adc_code);
    
    mAh_charge = (float)(adc_code) * 0.425f;
    //mAh_charge = (float)(adc_code*0.34f*prescallar*50E-6f)/(0.05*32768.0f);
    return (mAh_charge);
}

/*计算SENSE+电压*/
float LTC2943_code_to_voltage(uint16_t adc_code){
    float voltage;
    voltage = LTC2943_FULLSCALE_VOLTAGE*((float)adc_code/(65535));
    return(voltage);
}

/*用感应电阻计算电流 - 10mo*/
float LTC2943_code_to_current(uint16_t adc_code,float resistor){
    float current;
	//printf("%x\r\n",adc_code);
	float i_fs = 0.05f/resistor;	//计算满量程电流
	current = i_fs * (adc_code -32767) / 32767;	//计算当前电流
    return (current);
}

/*计算温度 kelvin*/
float LTC2943_code_to_kelvin_temperature(uint16_t adc_code){
    float temperature;
    temperature = adc_code*((float)(LTC2943_FULLSCALE_TEMPERATURE)/65535);
    return (temperature);
}

float LTC2943_code_to_celcius_temperature(uint16_t adc_code){
    float temperature;
    temperature = adc_code*((float)(LTC2943_FULLSCALE_TEMPERATURE)/65535)-273.15f;
    return (temperature);
}






