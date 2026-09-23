#include "Communication.h"

work_parameter_t current_work_parameter; // 全局变量，用于存储当前工作参数


/*
* @brief  同步功能处理函数
* @param  client_info *client
*/
static void sync_function(struct client_info *client){
    sync_t sync_param;
    //接收两字节数据
    char sync_cmd_buf[2];
    int bytes_received = recv(client->socket_num, sync_cmd_buf, sizeof(sync_cmd_buf), 0);
    if (bytes_received <= 0 || bytes_received != sizeof(sync_cmd_buf))
    {
        printf("Failed to receive sync command data.\n");
        return;
    }else{
        int bytes_received = recv(client->socket_num, &sync_param, sizeof(sync_param), 0);
        if (bytes_received <= 0 || bytes_received != sizeof(sync_param))
        {
            printf("Failed to receive sync parameters.\n");
            return;
        }else{
            //根据前两个字节判断是同步输入还是同步输出
            if(strcmp("OT", sync_cmd_buf) == 0){
                //同步输出
                if(sync_param.edge_mode == 0x01){
                    sync_config(SYNC_MODE_OUTPUT, SYNC_EDGE_RISING);
                }else if(sync_param.edge_mode == 0x02){
                    sync_config(SYNC_MODE_OUTPUT, SYNC_EDGE_FALLING);
                }
            }else if(strcmp("IN", sync_cmd_buf) == 0){
                //同步输入
                if(sync_param.edge_mode == 0x01){
                    sync_config(SYNC_MODE_INPUT, SYNC_EDGE_RISING);
                }else if(sync_param.edge_mode == 0x02){
                    sync_config(SYNC_MODE_INPUT, SYNC_EDGE_FALLING);
                }
            }
        }
    }
    //同步设置成功返回指令给显控
    send(client->socket_num, (const void *)SYNC_RESP, strlen(SYNC_RESP), 0);
}

/*
* @brief  接口选择函数
* @param  void
*/
void interface_selection(work_parameter_t work_mode){
    /*heading*/
    heading_set_input_mode((heading_input_mode_t)work_mode.HEADING_mode);
    /*motion*/
    motion_set_input_mode((motion_input_mode_t)work_mode.MOTION_mode);
    /*gnss*/
    gnss_set_input_mode((gnss_input_mode_t)work_mode.GNSS_mode);
    /*pps*/
    pps_set_input_mode((pps_input_mode_t)work_mode.PPS_mode);
    /* SVS */
    if (work_mode.SVS_mode == 0x00){
        /*
        * SVS 干端输入：
        * 干端 J9 接收表声 RS232/RS485，然后转 TTL，再经 U31 转 485 给湿端。
        * 此时 SVS 占用 U26/U31 通道，SYNC 不可用。
        */
        SVS_set_input_mode((SVS_input_mode_t)work_mode.SVS_comm_mode);

        /* SYNC disabled */
    }else{
        /*
        * SVS 湿端输入：
        * SVS disable 
        */
    } 
}


/*
* @brief  设置工作参数
* @param  client_info *client
* @retval None
*/
static void set_work_parameter(struct client_info *client)
{
    work_parameter_t work_param;
    // 接收四字节数据长度
    uint32_t data_length;
    int bytes_received = recv(client->socket_num, (char *)&data_length, sizeof(data_length), 0);
    if (bytes_received <= 0)
    {
        printf("Failed to receive data length for work parameter.\n");
        return;
    }else{
        recv(client->socket_num, (char *)&work_param, sizeof(work_param), 0);
    }
    //将当前工作状态更新至全局
    memcpy(&current_work_parameter, &work_param, sizeof(work_parameter_t));
    interface_selection(current_work_parameter);
}

/*
* @brief  硬件信息上报函数
* @param  client_info *client
* @retval None
*/
static void requset_hardware_info(struct client_info *client)
{
    report_hardware_info_t hardware_info;
    // Fill in the hardware_info structure with actual data
    hardware_info.version_number = VERSION_NUMBER; 
    hardware_info.device_type = 1005; // 2040s
    hardware_info.HEADING_status = 1; 
    hardware_info.MOTION_status = 1;
    hardware_info.GNSS_status = 1;
    hardware_info.PPS_status = 1;
    hardware_info.SVS_status = 1;
    hardware_info.reserved[0] = 0;         
    hardware_info.reserved[1] = 0;
    hardware_info.reserved[2] = 0;
    hardware_info.reserved[3] = 0;
    hardware_info.reserved[4] = 0;
    //发送数据头
    send(client->socket_num, (const void *)HARDWARE_INFO_RESP, strlen(HARDWARE_INFO_RESP), 0);
    //发送数据体
    send(client->socket_num, (const void *)&hardware_info, sizeof(hardware_info), 0);
    //发送数据尾
    send(client->socket_num, (const void *)DATA_END, strlen(DATA_END), 0);
}

/*
* @brief  设备复位函数
* @param  client_info *client
* @retval None
*/
static void reset_device(struct client_info *client)
{
    // Implement the device reset logic here
    printf("Device reset command received.\n");
    //关闭连接
    closesocket(client->socket_num);
    //释放内存
    mem_free(client);
    //重启
    __set_FAULTMASK(1);   // 1. 关闭所有中断，防止复位过程被打断[citation:4][citation:7][citation:11]
    NVIC_SystemReset();   // 2. 触发系统复位[citation:4][citation:11]
    while(1);             // 3. 等待复位发生
}

/*
* @brief  指令判断函数
* @param    client_info *client
            char *buf
            int len
* @retval 成功返回1 失败返回-1
*/
int Communication_Instruction_Judge(struct client_info *client,char *buf, int len)
{
    printf("ETH recv %d bytes\r\n", len);
    printf("Received command: %s\n", buf);
    if (strcmp(buf, RESET_CMD) == 0) {
        reset_device(client);
    } else if (strcmp(buf, HARDWARE_INFO_CMD) == 0) {
        requset_hardware_info(client);
    } else if (strcmp(buf, UPDATE_DRY_CMD) == 0) {
        // ...
    } else if (strcmp(buf, WORK_PARAMETER_CMD) == 0) {
        set_work_parameter(client);
    } else if (strcmp(buf, SYNC_CMD) == 0) {
        // Handle sync command if necessary
        sync_function(client); 
    }else {
        printf("Unknown command received: %s\n", buf);
        return -1; // Unknown command
    }
    return 1; // Success
}
