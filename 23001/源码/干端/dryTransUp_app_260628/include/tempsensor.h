#ifndef __TEMPSENSOR_H
#define __TEMPSENSOR_H

typedef unsigned char uint8;

extern int     fd_icc;
extern float  dry_temp;
extern float   temprature1;
extern int     tem_num;//当温度不变化时进行累加，累加的值
extern uint8   i2c_read_reg;
extern uint8   local_high_value, local_low_value, remote_high_value, remote_low_value; //读出温度的整数位
extern uint8   status_register;

/**************************************TMP451温度传感器相关**************************************/
#define     Address                     0x4C         //TMP451地址
#define     I2C_SLAVE                   0x0703       //IIC从器件的地址设置
#define     REG_LOCAL_HIGH              0x00
#define     REG_REMOTE_HIGH             0x01
#define     REG_LOCAL_LOW               0x15
#define     REG_REMOTE_LOW              0x10
#define     STATUS_REG                  0x02
#define     REG_CONFIG_WRITE            0x09 

/**************************************NST175温度传感器相关**************************************/
#define     IFB_ADDR                    0x4E         //接口板处温度传感器地址
//#define     I2C_SLAVE                   0x0703       //IIC从器件的地址设置
#define     REG_TEMP                    0x00
#define     REG_CONFIG                  0x01
#define     REG_LOW_LIMIT               0x02
#define     REG_HIGH_LIMIT              0x03
#define     ID_REG                      0x07


extern uint8 TMP451_Init(void);
extern uint8 i2c_write(int fd, uint8 reg, uint8 val);
extern uint8 i2c_read(int fd, uint8 reg, uint8* val);
extern uint8 i2c_read2(int fd, uint8 reg, uint8* val);

extern void get_sensor_data(void);
extern void read_TMP451_sensor_and_save(void);


#endif
