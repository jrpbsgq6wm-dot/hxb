#ifndef _UPDATE_H_
#define _UPDATE_H_
/***************************串口配置结构体**********************/
typedef struct
{
	char *dev;
	int nSpeed;
	int nBits;
	char nEvent;
	int nStop;
} COM_CONFIG_PARAMETER;
#endif /* _UPDATE_H_ */
