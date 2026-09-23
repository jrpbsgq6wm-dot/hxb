#ifndef _MYIIC_H
#define _MYIIC_H
#include "sys.h"
#include "iic.h"

#define IIC_ADDR	0x28
#define WINDOWS_SIZE		3		//压力平均滑动窗口大小

extern float Pressure_value;
typedef struct{
	float x;	//当前最优估计�?
	float P;	//当前误差协方�?
	float Q;	//过程噪声
	float R;	//测量噪声
} KALmanFilter;

typedef struct {
	float buffer[WINDOWS_SIZE];
	int index;
	float zero_offset;
	char is_tared;
}PressureFilter;

extern PressureFilter PressureFilter_t;
extern void PressureFilter_Init(PressureFilter* pf);
extern HAL_StatusTypeDef IIC_WRITE(uint8_t address,uint8_t *data);
extern HAL_StatusTypeDef IIC_READ(uint8_t address,uint8_t *data);
extern HAL_StatusTypeDef SendRead_reg(uint8_t regaddr,uint8_t *pdata,uint16_t len);
extern HAL_StatusTypeDef SendWrite_reg(uint8_t regaddr,uint8_t *pdata,uint16_t len);
extern void I2C_SCAN(I2C_HandleTypeDef *hi2c);
extern uint8_t IIC_PressureSensor_IsReady(void);
extern void GetPressure(PressureFilter* pf);
extern float Calculate_Pressure(uint8_t *reg,float full_scale);
#endif

