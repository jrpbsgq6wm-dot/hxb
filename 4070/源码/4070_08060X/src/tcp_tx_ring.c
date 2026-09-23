#include "CuBeamOne.h"
#include "network.h"
#include "tcp_tx_ring.h"
#include "fpga_to_upper.h"

typedef enum
{
    TCP_TX_SLOT_FREE = 0,      /* 空闲槽位，可以被采集线程写入新帧 */
    TCP_TX_SLOT_WRITING,       /* 采集线程正在拷贝当前帧，发送线程不能读取 */
    TCP_TX_SLOT_READY,         /* 当前帧已完整入队，等待TCP发送线程发送 */
    TCP_TX_SLOT_SENDING,       /* TCP发送线程正在发送当前槽位数据 */
} TCP_TX_SLOT_STATE;

typedef struct
{
    unsigned char data[TCP_TX_PACKET_MAX];  /* 完整TCP字节流：包头4字节 + 长度4字节 + 数据体 */
    unsigned int len;                       /* 当前槽位实际需要发送的总字节数 */
    unsigned int payload_len;               /* ddr_sonar_data有效长度，不包含包头和长度字段 */
    TCP_TX_SLOT_STATE state;                /* 当前槽位状态 */
} TCP_TX_SLOT;

typedef struct
{
    TCP_TX_SLOT slot[TCP_TX_RING_DEPTH];    /* 固定深度环形队列，避免运行时malloc/free */
    unsigned int write_idx;                 /* 采集线程写入位置 */
    unsigned int read_idx;                  /* TCP发送线程读取位置 */
    unsigned int used;                      /* 当前已经占用的槽位数量 */
    unsigned int max_used;                  /* 历史最大占用数量，用于观察发送压力 */
    unsigned int drop_count;                /* 队列满时丢弃的帧数 */
    pthread_mutex_t lock;                   /* 保护队列索引、状态和统计值 */
    pthread_cond_t not_empty;               /* 队列非空通知，唤醒TCP发送线程 */
} TCP_TX_RING;

static TCP_TX_RING g_tcp_tx_ring = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
};

static int tcp_send_all_chunked(int fd, const unsigned char *data, unsigned int len)
{
    unsigned int offset = 0;
    const unsigned int chunk = 16U * 1024U;

    /* send不保证一次发完全部数据，所以这里按16KB分块循环发送。 */
    while (offset < len)
    {
        unsigned int remain = len - offset;
        unsigned int send_size = (remain > chunk) ? chunk : remain;
        ssize_t ret = send(fd, data + offset, send_size, MSG_NOSIGNAL);

        if (ret > 0)
        {
            offset += (unsigned int)ret;
            continue;
        }

        if (ret < 0 && errno == EINTR)
        {
            continue;
        }

        if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            /* 非阻塞场景下socket暂时不可写，短暂等待后继续尝试。 */
            usleep(1000);
            continue;
        }

        return -1;
    }

    return (int)offset;
}

int tcp_tx_ring_enqueue(const unsigned char head[4],
                        const unsigned char *payload,
                        unsigned int payload_len)
{
    TCP_TX_SLOT *slot;
    unsigned int packet_len;
    // 数据异常
    if (head == NULL || payload == NULL || payload_len > TCP_TX_PAYLOAD_MAX)
    {
        return -1;
    }
    //计算最终数据的发送长度 packet_len = payload_len + 4head + 4sendlen;
    packet_len = payload_len + 8U;

    pthread_mutex_lock(&g_tcp_tx_ring.lock);

    if (g_tcp_tx_ring.used >= TCP_TX_RING_DEPTH)
    {
        g_tcp_tx_ring.drop_count++;
        printf("tcp tx ring full, drop current frame. used=%u drop=%u\n",
               g_tcp_tx_ring.used, g_tcp_tx_ring.drop_count);
        pthread_mutex_unlock(&g_tcp_tx_ring.lock);
        return -2;
    }

    /*
     * 先占用一个槽位，再释放队列锁去做大块memcpy。
     * 发送线程只会读取READY状态的槽位，因此不会读到正在拷贝的半帧数据。
     */
    slot = &g_tcp_tx_ring.slot[g_tcp_tx_ring.write_idx];
    slot->state = TCP_TX_SLOT_WRITING;
    g_tcp_tx_ring.write_idx = (g_tcp_tx_ring.write_idx + 1U) % TCP_TX_RING_DEPTH;
    g_tcp_tx_ring.used++;
    if (g_tcp_tx_ring.used > g_tcp_tx_ring.max_used)
    {
        g_tcp_tx_ring.max_used = g_tcp_tx_ring.used;
    }

    pthread_mutex_unlock(&g_tcp_tx_ring.lock);

    /*
     * 保持原有TCP协议顺序不变：
     *   DataHead_S(4) + total_len(4) + ddr_sonar_data(total_len)
     */
    memcpy(slot->data, head, 4U);
    memcpy(slot->data + 4U, &payload_len, 4U);
    memcpy(slot->data + 8U, payload, payload_len);
    slot->len = packet_len;
    slot->payload_len = payload_len;

    pthread_mutex_lock(&g_tcp_tx_ring.lock);
    slot->state = TCP_TX_SLOT_READY;
    pthread_cond_signal(&g_tcp_tx_ring.not_empty);
    pthread_mutex_unlock(&g_tcp_tx_ring.lock);

    return 0;
}

static TCP_TX_SLOT *tcp_tx_ring_pop(void)
{
    TCP_TX_SLOT *slot;

    pthread_mutex_lock(&g_tcp_tx_ring.lock);

    /* 队列为空时发送线程睡眠等待，避免空转占用CPU。 */
    while (g_tcp_tx_ring.used == 0U ||
           g_tcp_tx_ring.slot[g_tcp_tx_ring.read_idx].state != TCP_TX_SLOT_READY)
    {
        pthread_cond_wait(&g_tcp_tx_ring.not_empty, &g_tcp_tx_ring.lock);
    }

    slot = &g_tcp_tx_ring.slot[g_tcp_tx_ring.read_idx];
    slot->state = TCP_TX_SLOT_SENDING;
    g_tcp_tx_ring.read_idx = (g_tcp_tx_ring.read_idx + 1U) % TCP_TX_RING_DEPTH;

    pthread_mutex_unlock(&g_tcp_tx_ring.lock);

    return slot;
}

static void tcp_tx_ring_release(TCP_TX_SLOT *slot)
{
    pthread_mutex_lock(&g_tcp_tx_ring.lock);

    /* 当前帧发送结束后释放槽位，采集线程后续可以继续复用该缓存。 */
    slot->len = 0;
    slot->payload_len = 0;
    slot->state = TCP_TX_SLOT_FREE;
    if (g_tcp_tx_ring.used > 0U)
    {
        g_tcp_tx_ring.used--;
    }

    pthread_mutex_unlock(&g_tcp_tx_ring.lock);
}

unsigned int tcp_tx_ring_drop_count(void)
{
    unsigned int count;

    pthread_mutex_lock(&g_tcp_tx_ring.lock);
    count = g_tcp_tx_ring.drop_count;
    pthread_mutex_unlock(&g_tcp_tx_ring.lock);

    return count;
}

unsigned int tcp_tx_ring_max_used(void)
{
    unsigned int max_used;

    pthread_mutex_lock(&g_tcp_tx_ring.lock);
    max_used = g_tcp_tx_ring.max_used;
    pthread_mutex_unlock(&g_tcp_tx_ring.lock);

    return max_used;
}

void *thread_tcp_tx_ring_send(void *arg)
{
    (void)arg;

    while (1)
    {
        int send_ret;
        TCP_TX_SLOT *slot = tcp_tx_ring_pop();

        /*
         * Connect_fd同时被状态回复、EEPROM回复等控制命令使用。
         * 这里发送完整大包时加mut锁，防止不同线程往同一个TCP流里交叉写数据。
         * 采集线程只负责入队，不持有mut等待网络发送，因此不会被慢TCP拖住。
         */
        pthread_mutex_lock(&mut);
        //  printf_time("       send start\n");
        send_ret = tcp_send_all_chunked(Connect_fd, slot->data, slot->len);
        //  printf_time("       send ok\n");
        pthread_mutex_unlock(&mut);
        
        if (send_ret < 0)
        {
            printf("tcp tx send failed, payload_len=%u\n", slot->payload_len);
            tcp_tx_ring_release(slot);
            error_process();
            continue;
        }

        tcp_tx_ring_release(slot);
    }

    return NULL;
}
