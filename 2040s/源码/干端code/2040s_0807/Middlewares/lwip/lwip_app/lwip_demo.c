#include "lwip_demo.h"
#include "Communication.h"

/*服务器主动发给客户端的欢迎字符串 */
static const char *send_data = "link Success\n";

/**
 * @brief  单个客户端的处理线程
 * @param  param: 指向 client_info 结构体
 */
void lwip_client_thread_entry(void *param)
{
    struct client_info *client = param;

    /* 打印客户端的 socket、IP、端口 */
    printf("Client[%d]%s:%d is connect server\r\n",
           client->socket_num,
           inet_ntoa(client->ip_addr.sin_addr),
           ntohs(client->ip_addr.sin_port));
    /* 先发一条欢迎信息给客户端 */
    send(client->socket_num, (const void *)send_data, strlen(send_data), 0);
	/* 申请一个接收缓冲区 */
    char *str = (char *)mem_malloc(2048);
    while (1)
    {
        if (str == NULL)
        {
            break;
        }
        memset(str, 0, 2048);
        /* 接收客户端数据 */
        int bytes = recv(client->socket_num, str, 2048, 0);
		/* 收到 0 或负值，一般表示客户端断开或出错 */
        if (bytes <= 0)
        {
            mem_free(str);
			//跳出循环
            break;
        }else{
            printf("Handle business logic\n");
            /* 业务处理 */
            Communication_Instruction_Judge(client, str, bytes);
        }
    }
	mem_free(str);
    /* 客户端断开 */
    printf("[%d]%s:%d is disconnect...\r\n",
           client->socket_num,
           inet_ntoa(client->ip_addr.sin_addr),
           ntohs(client->ip_addr.sin_port));
    closesocket(client->socket_num);
    mem_free(client);
    /* 删除当前任务 */
    vTaskDelete(NULL);
}

/**
 * @brief  lwIP TCP 服务端主流程
 *
 * 作用：
 * 1. 创建监听 socket
 * 2. 绑定端口
 * 3. 进入监听状态
 * 4. accept 新连接
 * 5. 每来一个客户端，就创建一个独立任务处理
 */
void lwip_demo(void)
{
    struct client_info *client_fo;
    struct client_task_info *client_task_fo;
    struct link_socjet_info *socket_link_info;

    int sin_size = sizeof(struct sockaddr_in);
    char client_name[10] = "cli";
    char client_num[10];

    /* 申请监听 socket 管理结构 */
    socket_link_info = mem_malloc(sizeof(struct link_socjet_info));

    /* 申请客户端任务信息结构 */
    client_task_fo = mem_malloc(sizeof(struct client_task_info));
    client_task_fo->client_handler = NULL;
    client_task_fo->client_task_pro = 5;
    client_task_fo->client_task_stk = 1024;

    /* 创建 TCP 监听 socket */
    if ((socket_link_info->sock_listen = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Socket error\r\n");
        return;
    }

    /* 配置监听地址 */
    socket_link_info->listen_addr.sin_family = AF_INET;
    socket_link_info->listen_addr.sin_port = htons(8000);
    socket_link_info->listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(socket_link_info->listen_addr.sin_zero), 0, sizeof(socket_link_info->listen_addr.sin_zero));

    /* 绑定 socket 到 8000 端口 */
    if (bind(socket_link_info->sock_listen,(struct sockaddr *)&socket_link_info->listen_addr,sizeof(struct sockaddr)) < 0)
    {
        printf("Bind fail!\r\n");
        goto __exit;
    }

    /* 开始监听，最大等待队列长度 4 */
    listen(socket_link_info->sock_listen, 4);
    printf("begin listing...\r\n");

    while (1)
    {
        /* 等待客户端连接 */
        socket_link_info->sock_connect = accept(socket_link_info->sock_listen,
                                                (struct sockaddr *)&socket_link_info->connect_addr,
                                                (socklen_t *)&sin_size);
        if (socket_link_info->sock_connect == -1)
        {
            printf("no socket, waitting others socket disconnect.\r\n");
            continue;
        }
        /* 把 socket 号转成字符串，用作任务名后缀 */
        lwip_itoa(client_num, sizeof(client_num), socket_link_info->sock_connect);
        strcat(client_name, client_num);
        client_task_fo->client_name = client_name;
        client_task_fo->client_num = client_num;

        /* 为当前客户端申请信息结构 */
        client_fo = mem_malloc(sizeof(struct client_info));
        client_fo->socket_num = socket_link_info->sock_connect;
        memcpy(&client_fo->ip_addr, &socket_link_info->connect_addr, sizeof(struct sockaddr_in));
        client_fo->sockaddr_len = sin_size;

        /* 为当前客户端创建独立任务 */
        xTaskCreate((TaskFunction_t)lwip_client_thread_entry,
                    (const char *)client_task_fo->client_name,
                    (uint16_t)client_task_fo->client_task_stk,
                    (void *)(void *)client_fo,
                    (UBaseType_t)client_task_fo->client_task_pro++,
                    (TaskHandle_t *)&client_task_fo->client_handler);

        if (client_task_fo->client_handler == NULL)
        {
            printf("no memery for thread %s startup failed!\r\n", client_task_fo->client_name);
            mem_free(client_fo);
            continue;
        }
        else
        {
            printf("thread %s success!\r\n", client_task_fo->client_name);
        }
    }

__exit:
    printf("listener failed\r\n");

    /* 关闭监听 socket */
    closesocket(socket_link_info->sock_listen);

    /* 删除当前任务 */
    vTaskDelete(NULL);
}
