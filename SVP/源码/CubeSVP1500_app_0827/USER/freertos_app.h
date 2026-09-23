#ifndef FREERTOS_APP_H
#define FREERTOS_APP_H

#include "FreeRTOS.h"

BaseType_t freertos_app_create(void);

/*
 * I2C1 事务互斥接口：
 * - 调度器启动前不真正加锁，确保原有初始化流程保持不变；
 * - 调度器启动后只能由任务上下文调用，不能在中断服务函数中调用；
 * - 同一任务不得重复获取该普通互斥锁。
 */
BaseType_t freertos_i2c1_lock(TickType_t timeout);
void freertos_i2c1_unlock(void);

#endif
