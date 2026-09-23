#ifndef __LTC2943__H__
#define __LTC2943__H__

#include "iic.h"

#define LTC2943_I2C_ADDRESS 0xC8
#define I2C_READ_BIT        0X01
#define I2C_WRITE_BIT       0X00

#define WITH_ACK    0
#define WITH_NACK   1

//Registers
#define LTC2943_STATUS_REG                          0X00
#define LTC2943_CONTROL_REG                         0X01
#define LTC2943_ACCUM_CHARGE_MSB_REG                0X02
#define LTC2943_ACCUM_CHARGE_LSB_REG                0X03
#define LTC2943_CHARGE_THRESH_HIGH_MSB_REG          0X04
#define LTC2943_CHARGE_THRESH_HIGH_LSB_REG          0X05
#define LTC2943_CHARGE_THRESH_LOW_MSB_REG           0X06
#define LTC2943_CHARGE_THRESH_LOW_LSB_REG           0X07
#define LTC2943_VOLTAGE_MSB_REG                     0X08
#define LTC2943_VOLTAGE_LSB_REG                     0X09
#define LTC2943_VOLTAGE_THRESH_HIGH_MSB_REG         0X0A
#define LTC2943_VOLTAGE_THRESH_HIGH_LSB_REG         0X0B
#define LTC2943_VOLTAGE_THRESH_LOW_MSB_REG          0X0C
#define LTC2943_VOLTAGE_THRESH_LOW_LSB_REG          0X0D
#define LTC2943_CURRENT_MSB_REG                     0X0E
#define LTC2943_CURRENT_LSB_REG                     0X0F
#define LTC2943_CURRENT_THRESH_HIGH_MSB_REG         0X10
#define LTC2943_CURRENT_THRESH_HIGH_LSB_REG         0X11
#define LTC2943_CURRENT_THRESH_LOW_MSB_REG          0X12
#define LTC2943_CURRENT_THRESH_LOW_LSB_REG          0X13
#define LTC2943_TEMPERATURE_MSB_REG                 0X14
#define LTC2943_TEMPERATURE_LSB_REG                 0X15
#define LTC2943_TEMPERATURE_THRESH_HIGH_REG         0X16
#define LTC2943_TEMPERATURE_THRESH_LOW_REG          0X17

//CMD
#define LTC2943_AUTOMATIC_MODE                      0XC0
#define LTC2943_SCAN_MODE                           0X80
#define LTC2943_MANUAL_MODE                         0X40
#define LTC2943_SLEEP_MODE                          0X00

#define LTC2943_PRESCALAR_M_1                       0X00
#define LTC2943_PRESCALAR_M_4                       0X08
#define LTC2943_PRESCALAR_M_16                      0X10
#define LTC2943_PRESCALAR_M_64                      0X18
#define LTC2943_PRESCALAR_M_256                     0X20
#define LTC2943_PRESCALAR_M_1024                    0X28
#define LTC2943_PRESCALAR_M_4096                    0X30
#define LTC2943_PRESCALAR_M_4096_2                  0X31

#define LTC2943_ALRET_MODE                          0X04
#define LTC2943_CHARGE_COMPLETE_MODE                0X02
        
#define LTC2943_DISABLE_ALCC_PIN                    0X00
#define LTC2943_SHUTDOWN_MODE                       0X01

extern uint8_t LTC2943_Write(uint8_t i2c_address,uint8_t adc_command,uint8_t code);
extern uint8_t LTC2943_Write_16_Bits(uint8_t i2c_address,uint8_t adc_command,uint16_t code);
extern uint8_t LTC2943_Read(uint8_t i2c_address,uint8_t adc_command,uint8_t* adc_code);
extern uint8_t LTC2943_Read_16_Bits(uint8_t i2c_address,uint8_t adc_command,uint16_t* adc_code);
/*计算库伦电荷*/
extern float LTC2943_code_to_coulombs(uint16_t adc_code,float resistor,uint16_t prescallar);
/*计算mA*/
extern float LTC2943_code_to_mAh(uint16_t adc_code,float resistor,uint16_t prescallar);
/*计算SENSE+电压*/
extern float LTC2943_code_to_voltage(uint16_t adc_code);
/*用感应电阻计算电流*/
extern float LTC2943_code_to_current(uint16_t adc_code,float resistor);
/*计算温度 kelvin*/
extern float LTC2943_code_to_kelvin_temperature(uint16_t adc_code);
extern float LTC2943_code_to_celcius_temperature(uint16_t adc_code);
#endif


