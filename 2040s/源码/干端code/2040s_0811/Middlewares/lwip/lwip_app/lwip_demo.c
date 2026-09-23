#include "lwip_demo.h"
#include "Communication.h"
#include "./BSP/LP5012/LP5012.h"
#include "lwip/inet_chksum.h"


/* 湿端状态灯监测参数：目标湿端固定在同一网段的 192.168.0.5。 */
#define WET_END_IP_ADDRESS                 "192.168.0.5"
#define WET_END_PING_INTERVAL_MS           1000U  /* Ping 周期。 */
#define WET_END_PING_TIMEOUT_MS            500U   /* 单次等待回包的超时时间。 */
#define WET_END_PING_FAILURE_THRESHOLD     3U     /* 连续失败次数达到该值后显示红灯。 */
#define WET_END_PING_IDENTIFIER            0x2040U /* 用于过滤本机发出的 ICMP 回包。 */
#define WET_END_LED_BRIGHT                 0x33U  /* LP5012 D2 的亮度。 */
#define WET_END_ICMP_ECHO_REQUEST          8U
#define WET_END_ICMP_ECHO_REPLY            0U
#define WET_END_LED_STATUS_UNREACHABLE     0U
#define WET_END_LED_STATUS_REACHABLE       1U
#define WET_END_LED_STATUS_SLEEP           2U
#define WET_END_MONITOR_DEBUG              0U

/* ICMP Echo 报文头。发送请求和校验回包时均使用该固定 8 字节结构。 */
typedef struct
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} wet_end_icmp_echo_t;

/* 任务句柄用于避免重复创建；省电标志可由显控命令处理线程异步更新。 */
static TaskHandle_t g_wet_end_monitor_task_handle = NULL;
static volatile uint8_t g_wet_end_sleep_mode = 0U;
static uint8_t g_wet_end_led_status = 0xFFU;

/**
 * @brief 根据湿端状态设置 LP5012 的 D2 指示灯。
 * @note  相同状态不重复写 I2C，避免 Ping 周期内反复访问 LP5012。
 *        可达为绿色，不可达为红色，显控省电且湿端已下电为蓝色。
 */
static void wet_end_led_status_set(uint8_t status)
{
    if (status == g_wet_end_led_status)
    {
        return;
    }

    g_wet_end_led_status = status;

#if (WET_END_MONITOR_DEBUG != 0U)
    printf("Wet-end LED status: %u\r\n", status);
#endif

    if (status == WET_END_LED_STATUS_REACHABLE)
    {
        LP5012_U1_Set_D2(0U, WET_END_LED_BRIGHT, 0U);
    }
    else if (status == WET_END_LED_STATUS_SLEEP)
    {
        LP5012_U1_Set_D2(0U, 0U, WET_END_LED_BRIGHT);
    }
    else
    {
        LP5012_U1_Set_D2(WET_END_LED_BRIGHT, 0U, 0U);
    }
}

/**
 * @brief 校验接收的 IP/ICMP 报文是否为本次 Ping 对应的 Echo Reply。
 * @note  同时比对源 IP、ICMP identifier 和 sequence，避免把网络中其他
 *        ICMP 报文误判为湿端在线。
 */
static uint8_t wet_end_ping_reply_is_valid(const uint8_t *packet,
                                            int packet_length,
                                            const struct sockaddr_in *source,
                                            uint32_t expected_source,
                                            uint16_t expected_sequence)
{
    uint16_t ip_header_length;
    const wet_end_icmp_echo_t *reply;

    if ((source->sin_addr.s_addr != expected_source) ||
        (packet_length < 28) ||
        ((packet[0] >> 4) != 4U))
    {
        return 0U;
    }

    ip_header_length = (uint16_t)((packet[0] & 0x0FU) * 4U);
    if ((ip_header_length < 20U) ||
        (packet_length < (int)(ip_header_length + sizeof(wet_end_icmp_echo_t))))
    {
        return 0U;
    }

    reply = (const wet_end_icmp_echo_t *)(packet + ip_header_length);
    return ((reply->type == WET_END_ICMP_ECHO_REPLY) &&
            (reply->code == 0U) &&
            (reply->identifier == htons(WET_END_PING_IDENTIFIER)) &&
            (reply->sequence == htons(expected_sequence))) ? 1U : 0U;
}

/**
 * @brief 向湿端发送一次 ICMP Echo Request，并等待对应的回包。
 * @retval 1: 收到有效回包；0: 发送失败、接收超时或回包不匹配。
 */
static uint8_t wet_end_ping_once(int socket_num,
                                  const struct sockaddr_in *destination,
                                  uint16_t sequence)
{
    wet_end_icmp_echo_t request;
    uint8_t receive_buffer[64];
    struct sockaddr_in source;
    socklen_t source_length;
    int received_length;
    uint8_t received_packets = 0U;

    memset(&request, 0, sizeof(request));
    request.type = WET_END_ICMP_ECHO_REQUEST;
    request.identifier = htons(WET_END_PING_IDENTIFIER);
    request.sequence = htons(sequence);
    request.checksum = inet_chksum(&request, sizeof(request));

    if (sendto(socket_num,
               (const char *)&request,
               (int)sizeof(request),
               0,
               (const struct sockaddr *)destination,
               sizeof(*destination)) != (int)sizeof(request))
    {
        return 0U;
    }

    while (received_packets < 4U)
    {
        source_length = sizeof(source);
        received_length = recvfrom(socket_num,
                                   (char *)receive_buffer,
                                   sizeof(receive_buffer),
                                   0,
                                   (struct sockaddr *)&source,
                                   &source_length);
        if (received_length <= 0)
        {
            return 0U;
        }

        received_packets++;
        if (wet_end_ping_reply_is_valid(receive_buffer,
                                        received_length,
                                        &source,
                                        destination->sin_addr.s_addr,
                                        sequence) != 0U)
        {
            return 1U;
        }
    }

    return 0U;
}

/**
 * @brief 湿端在线状态监测任务。
 * @note  任务创建后永久运行，工作流程如下：
 *        1. 创建 SOCK_RAW / IPPROTO_ICMP socket，并设置 500 ms 接收超时；
 *        2. 正常模式下每 1000 ms 对 192.168.0.5 发送一次 Echo Request；
 *        3. 收到与本次 sequence 对应的 Echo Reply 后，立即清零失败计数并显示绿灯；
 *        4. 一次超时或发送失败只累加失败计数，不立刻改变绿灯，避免偶发丢包闪烁；
 *        5. 连续失败达到 WET_END_PING_FAILURE_THRESHOLD（当前为 3）后显示红灯；
 *        6. 显控确认湿端断电后调用 wet_end_monitor_sleep_mode_set(1U)，
 *           此任务停止 Ping、复位失败计数、D2 保持蓝灯；
 *        7. 退出省电后调用 wet_end_monitor_sleep_mode_set(0U)，任务恢复 Ping。
 *
 *        socket 创建或 setsockopt 失败时，任务不会退出，而是延时一个检测周期后
 *        重试创建 socket。这样 lwIP 短暂资源不足或网络初始化尚未完成时可自动恢复。
 */
static void wet_end_monitor_task(void *pvParameters)
{
    struct sockaddr_in destination;
    struct timeval receive_timeout;
    TickType_t last_wake_time;
    uint16_t sequence = 0U;
    uint8_t failed_count = WET_END_PING_FAILURE_THRESHOLD;
    uint8_t ping_ok;
    int socket_num;

    (void)pvParameters;

    /* 目标地址只在任务启动时初始化一次，后续每次 Ping 都复用该结构。 */
    memset(&destination, 0, sizeof(destination));
    destination.sin_family = AF_INET;
    destination.sin_addr.s_addr = inet_addr(WET_END_IP_ADDRESS);

    /*
     * recvfrom() 的阻塞上限。Ping 没有回包时，wet_end_ping_once() 最多等待
     * 500 ms 后返回失败，任务仍能在下一秒开始下一轮检测。
     */
    receive_timeout.tv_sec = 0;
    receive_timeout.tv_usec = WET_END_PING_TIMEOUT_MS * 1000U;

    /*
     * vTaskDelayUntil() 使用绝对节拍调度，而非“本轮结束后再延时 1 秒”。
     * 因此 Ping 成功或超时消耗的时间不会不断累积造成检测周期漂移。
     */
    last_wake_time = xTaskGetTickCount();

    /*
     * 上电后尚未收到任何有效回包，默认按不可达显示红灯。
     * 若系统已经通过 wet_end_monitor_sleep_mode_set(1U) 进入省电模式，
     * 在后续循环中会立即改为蓝灯。
     */
    wet_end_led_status_set(WET_END_LED_STATUS_UNREACHABLE);

    for (;;)
    {
        /*
         * RAW socket 必须通过 lwIP Socket API 创建。socket 会在整个正常监测期间
         * 复用，不会每秒重复创建，从而减少 lwIP 的内存和邮箱分配次数。
         */
        socket_num = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if ((socket_num < 0) ||
            (setsockopt(socket_num,
                        SOL_SOCKET,
                        SO_RCVTIMEO,
                        &receive_timeout,
                        sizeof(receive_timeout)) != 0))
        {
            /*
             * socket 创建失败或接收超时参数设置失败：
             * - 若 socket 已创建，先关闭，避免描述符和 lwIP 资源泄漏；
             * - 正常模式显示红灯，省电模式继续显示蓝灯；
             * - 不删除任务，等待一个周期后回到外层循环重新建 socket。
             */
            if (socket_num >= 0)
            {
                closesocket(socket_num);
            }

            wet_end_led_status_set((g_wet_end_sleep_mode != 0U) ?
                                   WET_END_LED_STATUS_SLEEP :
                                   WET_END_LED_STATUS_UNREACHABLE);
            vTaskDelay(pdMS_TO_TICKS(WET_END_PING_INTERVAL_MS));
            continue;
        }

        for (;;)
        {
            if (g_wet_end_sleep_mode != 0U)
            {
                /*
                 * 蓝灯代表“受显控命令控制的湿端下电”，不是通信故障。
                 * 将失败计数预置为阈值，是为了离开省电模式后第一次 Ping 失败时
                 * 可以直接按当前不可达状态显示红灯，而不继承休眠前的旧计数。
                 */
                failed_count = WET_END_PING_FAILURE_THRESHOLD;
                wet_end_led_status_set(WET_END_LED_STATUS_SLEEP);
            }
            else
            {
                /*
                 * sequence 每次 Ping 递增。回包校验函数会检查该序号，防止上一次
                 * 延迟到达的 Echo Reply 被误认为本次 Ping 成功。
                 */
                sequence++;
                ping_ok = wet_end_ping_once(socket_num, &destination, sequence);

                if (ping_ok != 0U)
                {
                    /* 任意一次成功都说明湿端当前可达：清零失败计数并显示绿灯。 */
                    failed_count = 0U;
                    wet_end_led_status_set(WET_END_LED_STATUS_REACHABLE);
                }
                else
                {
                    /*
                     * 单次失败仅计数，不立即改灯。failed_count 做饱和递增，避免
                     * uint8_t 长时间运行后溢出而导致红灯被错误清除。
                     */
                    if (failed_count < WET_END_PING_FAILURE_THRESHOLD)
                    {
                        failed_count++;
                    }

                    /*
                     * 连续失败达到阈值后才进入红灯状态。若此前已经是红灯，
                     * wet_end_led_status_set() 会跳过重复的 LP5012 I2C 写操作。
                     */
                    if (failed_count >= WET_END_PING_FAILURE_THRESHOLD)
                    {
                        wet_end_led_status_set(WET_END_LED_STATUS_UNREACHABLE);
                    }
                }
            }

            /*
             * 固定监测周期。省电状态也保持该节拍轮询，以便显控退出省电后
             * 最迟一个周期内自动恢复 Ping，无需重建任务。
             */
            vTaskDelayUntil(&last_wake_time,
                            pdMS_TO_TICKS(WET_END_PING_INTERVAL_MS));
        }
    }
}

/**
 * @brief 创建湿端 Ping 监测任务。
 * @retval pdPASS: 创建成功或任务已经存在；其他值：FreeRTOS 内存不足。
 */
BaseType_t wet_end_monitor_start(void)
{
    if (g_wet_end_monitor_task_handle != NULL)
    {
        return pdPASS;
    }

    return xTaskCreate(wet_end_monitor_task,
                       "wet_end_ping",
                       512U,
                       NULL,
                       8U,
                       &g_wet_end_monitor_task_handle);
}

/**
 * @brief 设置湿端省电状态。
 * @param sleep_mode 非 0：湿端已下电，D2 显示蓝色并暂停 Ping；
 *                   0：湿端已上电，恢复 Ping 监测。
 * @note  显控处理省电命令时，应先调用 sona_power_en_set() 完成湿端上下电，
 *        再调用本函数更新状态灯和 Ping 状态。
 */
void wet_end_monitor_sleep_mode_set(uint8_t sleep_mode)
{
    g_wet_end_sleep_mode = (sleep_mode != 0U) ? 1U : 0U;
}

/**
 * @brief  从 socket 精确读取指定长度的字节。
 * @retval 1: 读取完整；0: 客户端断开或发生网络错误。
 */
static int lwip_recv_all(int socket_num, void *buffer, uint16_t length)
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
 * @brief  向 socket 完整发送指定字节。
 * @retval 1: 发送完整；0: 客户端断开或发生网络错误。
 */
 /*
static int lwip_send_all(int socket_num, const void *buffer, uint16_t length)
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
}*/

/**
 * @brief  处理单个 TCP 客户端。
 * @note
 * 每条业务命令固定为 4 字节。先精确读取这 4 字节，再交给业务层读取
 * 命令后面的参数体，可以正确应对 TCP 半包和粘包。
 */
void lwip_client_thread_entry(void *param)
{
    struct client_info *client = (struct client_info *)param;
    char command[5];

    printf("Client[%d] %s:%d connected\r\n",
           client->socket_num,
           inet_ntoa(client->ip_addr.sin_addr),
           ntohs(client->ip_addr.sin_port));

    for (;;)
    {
        /*
         * 只读取命令头本身。即使后面的参数已经到达 TCP 接收缓存，
         * 也会保留给 Communication_Instruction_Judge() 中的对应处理函数。
         */
        if (lwip_recv_all(client->socket_num, command, 4U) == 0)
        {
            break;
        }
        command[4] = '\0';
        (void)Communication_Instruction_Judge(client, command, 4);
    }

    printf("Client[%d] %s:%d disconnected\r\n",
           client->socket_num,
           inet_ntoa(client->ip_addr.sin_addr),
           ntohs(client->ip_addr.sin_port));
    closesocket(client->socket_num);
    mem_free(client);
    vTaskDelete(NULL);
}

/**
 * @brief  lwIP TCP 服务器主任务。
 * @note   监听 8000 端口；每接入一个客户端就创建一个独立 FreeRTOS 任务。
 */
void lwip_demo(void)
{
    struct link_socjet_info *socket_info;
    struct client_info *client;
    struct sockaddr_in client_address;
    socklen_t client_address_length;
    int client_socket;
    char task_name[configMAX_TASK_NAME_LEN];
    TaskHandle_t client_task_handle;
    static UBaseType_t client_task_priority = 5U;

    socket_info = (struct link_socjet_info *)mem_malloc(sizeof(*socket_info));
    if (socket_info == NULL)
    {
        printf("TCP server memory allocation failed.\r\n");
        vTaskDelete(NULL);
        return;
    }

    socket_info->sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_info->sock_listen == -1)
    {
        printf("Socket create failed.\r\n");
        goto server_exit;
    }

    memset(&socket_info->listen_addr, 0, sizeof(socket_info->listen_addr));
    socket_info->listen_addr.sin_family = AF_INET;
    socket_info->listen_addr.sin_port = htons(7000U);	//端口设置7000
    socket_info->listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socket_info->sock_listen,
             (struct sockaddr *)&socket_info->listen_addr,
             sizeof(socket_info->listen_addr)) < 0)
    {
        printf("TCP bind failed.\r\n");
        goto server_exit;
    }

    if (listen(socket_info->sock_listen, 4) < 0)
    {
        printf("TCP listen failed.\r\n");
        goto server_exit;
    }

    printf("TCP server listening on port 7000.\r\n");

    for (;;)
    {
        client_address_length = sizeof(client_address);
        client_socket = accept(socket_info->sock_listen,
                               (struct sockaddr *)&client_address,
                               &client_address_length);
        if (client_socket == -1)
        {
            printf("TCP accept failed.\r\n");
            continue;
        }

        client = (struct client_info *)mem_malloc(sizeof(*client));
        if (client == NULL)
        {
            printf("Client memory allocation failed.\r\n");
            closesocket(client_socket);
            continue;
        }

        client->socket_num = client_socket;
        client->ip_addr = client_address;
        client->sockaddr_len = (int)client_address_length;

        /*
         * FreeRTOS 在创建任务时会复制任务名，因此 task_name 可以使用当前函数的局部数组，
         * 不再把同一块可变字符串交给多个客户端任务共享。
         */
        snprintf(task_name, sizeof(task_name), "cli%d", client_socket);
        client_task_handle = NULL;

        if (xTaskCreate(lwip_client_thread_entry,
                        task_name,
                        1024U,
                        client,
                        client_task_priority,
                        &client_task_handle) != pdPASS)
        {
            printf("Client task create failed.\r\n");
            closesocket(client_socket);
            mem_free(client);
            continue;
        }

        if (client_task_priority < (configMAX_PRIORITIES - 1U))
        {
            client_task_priority++;
        }
    }

server_exit:
    closesocket(socket_info->sock_listen);
    mem_free(socket_info);
    vTaskDelete(NULL);
}
