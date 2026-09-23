#ifndef _TCP_TX_RING_H_
#define _TCP_TX_RING_H_

#define TCP_TX_RING_DEPTH      4U
#define TCP_TX_PAYLOAD_MAX     13000000U
#define TCP_TX_PACKET_MAX      (TCP_TX_PAYLOAD_MAX + 8U)

/*
 * 将一帧已经组好的数据放入TCP发送环形队列。
 *
 * 注意：这里不会直接send，只负责把当前帧复制到环形队列中，
 * 这样采集线程可以尽快返回继续等待下一次PL中断。
 *
 * 最终TCP发送线程发出的字节流仍然保持原协议格式：
 *   head[4] + payload_len[4] + payload[payload_len]
 *
 * 返回值：
 *   0   入队成功
 *  -1   参数错误或长度超过最大缓存
 *  -2   环形队列已满，当前帧被丢弃
 */
int tcp_tx_ring_enqueue(const unsigned char head[4],
                        const unsigned char *payload,
                        unsigned int payload_len);

/* 获取发送队列满导致的丢帧次数，用于后续定位TCP发送是否跟不上采集速度。 */
unsigned int tcp_tx_ring_drop_count(void);

/* 获取发送队列历史最大占用深度，用于判断缓存深度是否足够。 */
unsigned int tcp_tx_ring_max_used(void);

/* TCP发送线程入口函数，负责从环形队列取完整帧并通过Connect_fd发送。 */
void *thread_tcp_tx_ring_send(void *arg);

#endif /* _TCP_TX_RING_H_ */
