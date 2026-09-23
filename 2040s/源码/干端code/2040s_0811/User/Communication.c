#include "Communication.h"
#include "signal_switch.h"
#include "signal_monitor.h"

/* 当前由显控下发并已生效的工作模式参数。 */
work_parameter_t current_work_parameter;

/**
 * @brief  从 TCP 连接中读取指定长度的数据。
 * @param  socket_num: 客户端 socket。
 * @param  buffer: 接收缓冲区。
 * @param  length: 必须读取到的字节数。
 * @retval 1: 已完整接收；0: 客户端断开或网络异常。
 *
 * @note
 * TCP 是字节流协议，一次 recv() 不保证返回完整结构体。此函数循环接收，
 * 直到读满 length，避免结构体只接收半包时被误当作完整协议处理。
 */
static int communication_recv_all(int socket_num, void *buffer, uint16_t length)
{
    uint16_t received = 0U;
    int result;
    while (received < length)
    {
        result = recv(socket_num,(char *)buffer + received,(int)(length - received),0);
        if (result <= 0){
            return 0;
        }
        received += (uint16_t)result;
    }
    return 1;
}

/**
 * @brief  向 TCP 连接完整发送一段数据。
 * @retval 1: 已完整发送；0: 网络异常或客户端断开。
 *
 * @note send() 同样可能只发送部分数据，因此需要循环直到所有字节发出。
 */
static int communication_send_all(int socket_num, const void *buffer, uint16_t length)
{
    uint16_t sent = 0U;
    int result;

    while (sent < length)
    {
        result = send(socket_num,
                      (const char *)buffer + sent,
                      (int)(length - sent),
                      0);
        if (result <= 0)
        {
            return 0;
        }

        sent += (uint16_t)result;
    }

    return 1;
}

/**
 * @brief  处理显控下发的 SYNC 配置。
 * @note   协议格式：2 字节方向命令（"OT" 或 "IN"）+ sync_t 参数。
 */
static void sync_function(struct client_info *client)
{
    char sync_command[2];
    sync_t sync_parameter;
    signal_monitor_sync_mode_t sync_mode;

    if (communication_recv_all(client->socket_num, sync_command, sizeof(sync_command)) == 0)
    {
        printf("SYNC command receive failed.\r\n");
        return;
    }

    if (communication_recv_all(client->socket_num, &sync_parameter, sizeof(sync_parameter)) == 0)
    {
        printf("SYNC parameter receive failed.\r\n");
        return;
    }

    if (signal_monitor_svs_wet_input_is_enabled() == 0U)
    {
        printf("SYNC config rejected: SVS wet input is disabled.\r\n");
        return;
    }

    /*
     * sync_command 没有 '\0' 结尾，不能使用 strcmp()。
     * 使用 memcmp() 精确比较前两个协议字节，避免越界访问。
     */
    if (memcmp(sync_command, SYNC_OUTPUT_CMD, sizeof(sync_command)) == 0)
    {
        if (sync_parameter.edge_mode == 0x01)
        {
            sync_config(SYNC_MODE_OUTPUT, SYNC_EDGE_RISING);
        }
        else if (sync_parameter.edge_mode == 0x02)
        {
            sync_config(SYNC_MODE_OUTPUT, SYNC_EDGE_FALLING);
        }
        else
        {
            printf("Invalid SYNC output edge: %d\r\n", sync_parameter.edge_mode);
            return;
        }

        sync_mode = SIGNAL_MONITOR_SYNC_OUTPUT;
    }
    else if (memcmp(sync_command, SYNC_INPUT_CMD, sizeof(sync_command)) == 0)
    {
        if (sync_parameter.edge_mode == 0x01)
        {
            sync_config(SYNC_MODE_INPUT, SYNC_EDGE_RISING);
        }
        else if (sync_parameter.edge_mode == 0x02)
        {
            sync_config(SYNC_MODE_INPUT, SYNC_EDGE_FALLING);
        }
        else
        {
            printf("Invalid SYNC input edge: %d\r\n", sync_parameter.edge_mode);
            return;
        }

        sync_mode = SIGNAL_MONITOR_SYNC_INPUT;
    }
    else
    {
        printf("Invalid SYNC direction command.\r\n");
        return;
    }

    signal_monitor_sync_mode_set(sync_mode);

    (void)communication_send_all(client->socket_num, SYNC_RESP, strlen(SYNC_RESP));
}

/**
 * @brief  根据显控下发的工作参数，切换各路信号的硬件输入通道。
 */
void interface_selection(work_parameter_t work_mode)
{
    heading_set_input_mode((heading_input_mode_t)work_mode.HEADING_mode);
    motion_set_input_mode((motion_input_mode_t)work_mode.MOTION_mode);
    gnss_set_input_mode((gnss_input_mode_t)work_mode.GNSS_mode);
    pps_set_input_mode((pps_input_mode_t)work_mode.PPS_mode);

    if (work_mode.SVS_mode == 0x00)
    {
        /*
         * SVS 由干端 UART5 直接接收：
         * - LED 按 UART5 报文有效性显示红/绿；
         * - SYNC 通道不参与有效性监测。
         */
        signal_monitor_svs_wet_input_set(0U);
        SVS_set_input_mode((SVS_input_mode_t)work_mode.SVS_comm_mode);
    }
    else if (work_mode.SVS_mode == 0x01)
    {
        /*
         * SVS 由湿端提供：干端不解析 UART5 数据，SVS LED 固定显示蓝色；
         * SYNC 的使能状态由后续 SYNC 命令单独配置。
         */
        signal_monitor_svs_wet_input_set(1U);
    }
    else
    {
        printf("Invalid SVS mode: %d\r\n", work_mode.SVS_mode);
    }
}

/**
 * @brief  接收并应用显控下发的工作参数。
 * @note   协议格式：4 字节参数长度 + work_parameter_t 参数体。
 */
static void set_work_parameter(struct client_info *client)
{
    uint32_t data_length;
    work_parameter_t work_parameter;

    if (communication_recv_all(client->socket_num, &data_length, sizeof(data_length)) == 0)
    {
        printf("Work parameter length receive failed.\r\n");
        return;
    }

    /*
     * 当前协议约定参数体就是一个 work_parameter_t。长度不匹配时不继续接收，
     * 防止错误协议造成结构体越界或后续数据流错位。
     */
    if (data_length != sizeof(work_parameter))
    {
        printf("Invalid work parameter length: %lu\r\n", (unsigned long)data_length);
        return;
    }

    if (communication_recv_all(client->socket_num, &work_parameter, sizeof(work_parameter)) == 0)
    {
        printf("Work parameter body receive failed.\r\n");
        return;
    }

    memcpy(&current_work_parameter, &work_parameter, sizeof(current_work_parameter));
    interface_selection(current_work_parameter);
}

/**
 * @brief  向显控上报干端硬件信息。
 */
static void request_hardware_info(struct client_info *client)
{
    report_hardware_info_t hardware_info;
    signal_monitor_status_t signal_status;

    memset(&hardware_info, 0, sizeof(hardware_info));
    hardware_info.version_number = VERSION_NUMBER;
    hardware_info.device_type = 8U;
    signal_monitor_get_status(&signal_status);
    hardware_info.HEADING_status = (char)signal_status.heading_status;
    hardware_info.MOTION_status = (char)signal_status.motion_status;
    hardware_info.GNSS_status = (char)signal_status.gnss_status;
    hardware_info.SVS_status = (char)signal_status.svs_status;
    hardware_info.SYNC_status = (char)signal_monitor_sync_mode_get();
    if (communication_send_all(client->socket_num, HARDWARE_INFO_RESP, strlen(HARDWARE_INFO_RESP)) == 0)
    {
        return;
    }
	uint32_t size_req = (uint32_t)(sizeof(hardware_info) + sizeof(uint32_t));
	printf("%d\r\n",size_req);
	if (communication_send_all(client->socket_num, &size_req, 4) == 0)
    {
        return;
    }
    if (communication_send_all(client->socket_num, &hardware_info, sizeof(hardware_info)) == 0)
    {
        return;
    }
    (void)communication_send_all(client->socket_num, DATA_END, strlen(DATA_END));
}

/**
 * @brief  执行设备复位。
 * @note   在触发系统复位前关闭当前 socket 并释放客户端结构体。
 */
static void reset_device(struct client_info *client)
{
    printf("Device reset command received.\r\n");
    closesocket(client->socket_num);
    mem_free(client);

    __set_FAULTMASK(1U);
    NVIC_SystemReset();

    while (1)
    {
    }
}

/**
 * @brief  根据已接收的 4 字节命令头执行对应业务。
 * @param  client: 当前客户端连接。
 * @param  command: 以 '\0' 结尾的命令字符串。
 * @param  length: 命令长度，必须为 4。
 * @retval 1: 命令已处理；-1: 未知命令或命令长度错误。
 */
int Communication_Instruction_Judge(struct client_info *client, char *command, int length)
{
    if ((client == NULL) || (command == NULL) || (length != 4))
    {
        return -1;
    }

    printf("ETH command: %.4s\r\n", command);

    if (memcmp(command, RESET_CMD, 4U) == 0)
    {
        reset_device(client);
    }
    else if (memcmp(command, HARDWARE_INFO_CMD, 4U) == 0)
    {
        request_hardware_info(client);
    }
    else if (memcmp(command, UPDATE_DRY_CMD, 4U) == 0)
    {
        printf("Dry-end update command is not implemented.\r\n");
    }
    else if (memcmp(command, WORK_PARAMETER_CMD, 4U) == 0)
    {
        set_work_parameter(client);
    }
    else if (memcmp(command, SYNC_CMD, 4U) == 0)
    {
        sync_function(client);
    }
    else
    {
        printf("Unknown command: %.4s\r\n", command);
        return -1;
    }

    return 1;
}
