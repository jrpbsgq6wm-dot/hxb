// pri.h
#ifndef PRI_H
#define PRI_H

#include <pthread.h>

#define MAX_MSG_SIZE 512
#define QUEUE_SIZE 20

// 消息队列结构
typedef struct {
    char msgs[QUEUE_SIZE][MAX_MSG_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
} msg_queue_t;

// 全局队列（声明，在pri.c中定义）
extern msg_queue_t g_msg_queue;

// 函数声明
void queue_init(void);
int queue_push_nonblock(const char *msg);
int queue_pop(char *out);
void thread_pri_func(void);

#endif