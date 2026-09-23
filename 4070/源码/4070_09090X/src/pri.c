// pri.c
#include "pri.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// 全局队列定义
msg_queue_t g_msg_queue;

// 队列初始化
void queue_init(void) {
    g_msg_queue.head = 0;
    g_msg_queue.tail = 0;
    g_msg_queue.count = 0;
    pthread_mutex_init(&g_msg_queue.mutex, NULL);
}

// 生产者：非阻塞放入（队列满则丢弃）
int queue_push_nonblock(const char *msg) {
    if (pthread_mutex_trylock(&g_msg_queue.mutex) != 0) {
        return -1;  // 锁被占用，丢弃
    }
    
    if (g_msg_queue.count >= QUEUE_SIZE) {
        pthread_mutex_unlock(&g_msg_queue.mutex);
        return -2;  // 队列满，丢弃
    }
    
    strcpy(g_msg_queue.msgs[g_msg_queue.tail], msg);
    g_msg_queue.tail = (g_msg_queue.tail + 1) % QUEUE_SIZE;
    g_msg_queue.count++;
    
    pthread_mutex_unlock(&g_msg_queue.mutex);
    return 0;
}

// 消费者：取出数据
int queue_pop(char *out) {
    pthread_mutex_lock(&g_msg_queue.mutex);
    
    if (g_msg_queue.count == 0) {
        pthread_mutex_unlock(&g_msg_queue.mutex);
        return -1;  // 队列空
    }
    
    strcpy(out, g_msg_queue.msgs[g_msg_queue.head]);
    g_msg_queue.head = (g_msg_queue.head + 1) % QUEUE_SIZE;
    g_msg_queue.count--;
    
    pthread_mutex_unlock(&g_msg_queue.mutex);
    return 0;
}

// ============ 消费者线程函数 ============
void thread_pri_func(void) {
    char msg[MAX_MSG_SIZE];
    
    while (1) {
        // 从队列取数据
        if (queue_pop(msg) == 0) {
            // 打印数据
            printf("%s\n", msg);
        }
    }
}