#include "power_app.h"


const float resistor = .100;    //²Î¿¼µç×è
uint8_t ack = 0;
static uint8_t mAh_or_Coulombs = 0;
static uint8_t celcius_or_kelvin = 0;
static uint16_t prescalar_mode = LTC2943_PRESCALAR_M_4096;
static uint16_t prescalar_value = 4096;
static uint16_t alcc_mode = LTC2943_ALRET_MODE;
float charge = 0,current = 0,voltage = 0,temperature = 0;

HAL_StatusTypeDef read_power_data(void){
    if(menu_1_automatic_mode(mAh_or_Coulombs,celcius_or_kelvin,prescalar_mode,prescalar_value,alcc_mode) == HAL_OK)
        return HAL_OK;
    return HAL_ERROR_LTC2943;
}

uint8_t menu_1_automatic_mode(uint8_t mAh_or_Coulombs,uint8_t celcius_or_kelvin,uint16_t prescalar_mode,
                uint16_t prescalar_value,uint16_t alcc_mode)
{
    uint8_t LTC2943_mode,ack=0;
    LTC2943_mode = LTC2943_AUTOMATIC_MODE | prescalar_mode | alcc_mode; //ÔÝ´æ¿ØÖÆ¼Ä´æÆ÷
    ack |= LTC2943_Write(LTC2943_I2C_ADDRESS,LTC2943_CONTROL_REG,LTC2943_mode);
    
    uint8_t status_code = 0;
    uint16_t charge_code = 0,current_code = 0,voltage_code = 0,temperature_code = 0;
    uint16_t charge_l2b = 0,current_l2b = 0,voltage_l2b = 0,temperature_l2b = 0;
    
    ack |= LTC2943_Read_16_Bits(LTC2943_I2C_ADDRESS|I2C_READ_BIT,LTC2943_ACCUM_CHARGE_MSB_REG,&charge_code);
    ack |= LTC2943_Read_16_Bits(LTC2943_I2C_ADDRESS|I2C_READ_BIT,LTC2943_VOLTAGE_MSB_REG,&voltage_code);
    ack |= LTC2943_Read_16_Bits(LTC2943_I2C_ADDRESS|I2C_READ_BIT,LTC2943_CURRENT_MSB_REG,&current_code);
    ack |= LTC2943_Read(LTC2943_I2C_ADDRESS|I2C_READ_BIT,LTC2943_STATUS_REG,&status_code);
    ack |= LTC2943_Read_16_Bits(LTC2943_I2C_ADDRESS|I2C_READ_BIT,LTC2943_TEMPERATURE_MSB_REG,&temperature_code);
    
    charge_l2b = (((charge_code & 0x00FF) << 8) | ((charge_code & 0xff00)>>8));
    current_l2b = (((current_code & 0x00FF) << 8) | ((current_code & 0xff00)>>8));
    voltage_l2b = (((voltage_code & 0x00FF) << 8) | ((voltage_code & 0xff00)>>8));
    temperature_l2b = (((temperature_code & 0x00FF) << 8) | ((temperature_code & 0xff00)>>8));
    
    charge = LTC2943_code_to_mAh(charge_l2b,resistor,prescalar_value);
    current = LTC2943_code_to_current(current_l2b,resistor);
    voltage = LTC2943_code_to_voltage(voltage_l2b);
    temperature = LTC2943_code_to_celcius_temperature(temperature_l2b);
    //printf("[%s]:%fmA CURRENT=%f %fV\r\n",__func__,charge,current,voltage);
    return ack;
}



