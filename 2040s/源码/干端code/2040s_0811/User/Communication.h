#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include "sys.h"
#include "lwip_demo.h"
#include "lwip/netdb.h"
#include <lwip/sockets.h>
#include <string.h>
#include <stdlib.h>

/*业务逻辑*/

/*版本号*/
#define VERSION_NUMBER 			2026081000

/*接收显控的指令*/
//请求
#define RESET_CMD				"@@RD"
#define HARDWARE_INFO_CMD		"@@RH"
#define UPDATE_DRY_CMD			"@@UD"
#define WORK_PARAMETER_CMD		"@@SD"
#define SYNC_CMD				"<<SY"
#define SYNC_OUTPUT_CMD			"OT"
#define SYNC_INPUT_CMD			"IN"
//响应
#define HARDWARE_INFO_RESP		"@@SH"
#define SYNC_RESP				"<<YS"
//数据尾
#define DATA_END				"@@ED"

/*工作参数结构体 - 显控设置*/
typedef struct {
	char wet_workmode;	//湿端工作模式
	char HEADING_mode;
	char MOTION_mode;
	char GNSS_mode;
	char PPS_mode;
	char SVS_mode;
	char SVS_comm_mode;
	char RSV[5];
	uint32_t crc;
	char data_tail[4];
}work_parameter_t;

/*硬件信息上报结构体 - 干端数据上报*/
typedef struct{
	uint32_t version_number;
	uint16_t device_type;
	char HEADING_status;
	char MOTION_status;
	char GNSS_status;
	char PPS_status;
	char SVS_status;
	char SYNC_status;	//0同步状态未设置，1同步输入 2同步输出
	char reserved[4];
}report_hardware_info_t;

/*定时协议格式 - 干端数据上报*/
typedef struct{
	float temp;
}report_temperature;

/*同步相关*/
typedef struct{
	char edge_mode;	//0x01=上升沿触发 0x02=下降沿触发 
	float delay_time; //延时时间
}sync_t;

int Communication_Instruction_Judge(struct client_info *client,char *buf, int len);

#endif

