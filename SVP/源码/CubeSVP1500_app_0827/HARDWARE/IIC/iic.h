#ifndef __IIC__H__
#define __IIC__H__

#include "sys.h"
extern I2C_HandleTypeDef IIC_Config;    //iic¾ä±ú
extern I2C_HandleTypeDef Pressure_Config;    //iic¾ä±ú

extern void IIC_Init(void); 
extern void FST800_Init(void);
void Error_Handler(void);
extern void IIC_DeviceInit(void);
#endif

