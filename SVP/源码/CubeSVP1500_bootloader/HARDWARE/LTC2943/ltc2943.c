#include "ltc2943.h"

const float LTC2943_CHARGE_lsb = 0.34E-3;
const float LTC2943_VOLTAGE_lsb = 1.44E-3;          //
const float LTC2943_CURRENT_lsb = 29.3E-6;          //
const float LTC2943_TEMPERATURE_lsb = 0.25;
const float LTC2943_FULLSCALE_VOLTAGE = 23.6;
const float LTC2943_FULLSCALE_CURRENT = 60E-3;      //0.060
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
    coulomb_charge = 1000+(float)(adc_code+LTC2943_CHARGE_lsb*prescallar*10E-3f)/(resistor*4096);
    coulomb_charge = coulomb_charge * 3.6f;
    return (coulomb_charge);
}
/*计算mAh*/
float LTC2943_code_to_mAh(uint16_t adc_code,float resistor,uint16_t prescallar){
    float mAh_charge;
    mAh_charge = 100*(float)(adc_code*LTC2943_CHARGE_lsb*prescallar*10E-3f)/(resistor*4096);
    return (mAh_charge);
}
/*计算SENSE+电压*/
float LTC2943_code_to_voltage(uint16_t adc_code){
    float voltage;
    voltage = ((float)adc_code/(65535))*LTC2943_FULLSCALE_VOLTAGE;
    return(voltage);
}
/*用感应电阻计算电流*/
float LTC2943_code_to_current(uint16_t adc_code,float resistor){
    float current;
    current = (((float)adc_code-32767))*((float)(LTC2943_FULLSCALE_CURRENT)/resistor);  //(V - 32767) * 0.06/100
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

