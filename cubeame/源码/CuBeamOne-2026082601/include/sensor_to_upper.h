#ifndef _UPDATE_H_
#define _UPDATE_H_

/**************************************TMP451温度传感器相关**************************************/
#define Address 0x4C	 // TMP451地址
#define I2C_SLAVE 0x0703 // IIC从器件的地址设置
#define REG_LOCAL_HIGH 0x00
#define REG_REMOTE_HIGH 0x01 // 目前没有使用
#define REG_LOCAL_LOW 0x15
#define REG_REMOTE_LOW 0x10 // 目前没有使用
#define STATUS_REG 0x02
#define REG_CONFIG_WRITE 0x09

/***************************串口配置结构体**********************/
typedef struct
{
	char *dev;
	int nSpeed;
	int nBits;
	char nEvent;
	int nStop;
} COM_CONFIG_PARAMETER;

extern void read_TMP451_sensor_and_save(void);

#endif /* _UPDATE_H_ */
