#ifndef TMP451_H
#define TMP451_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <errno.h>

/* 核心配置（适配你的硬件：I2C0总线 + 0x4C地址） */
#define TMP451_I2C_BUS     "/dev/i2c-0"  // I2C0总线设备文件
#define TMP451_I2C_ADDR    0x4C          // TMP451的I2C地址

/* TMP451寄存器定义（严格按官方手册） */
#define TMP451_REG_LOCAL_TEMP_INT    0x00    // 本地温度整数部分（8位）
#define TMP451_REG_REMOTE_TEMP_INT   0x01    // 远程温度整数部分（8位）
#define TMP451_REG_LOCAL_TEMP_FRAC   0x15    // 本地温度小数部分（高4位有效）
#define TMP451_REG_REMOTE_TEMP_FRAC  0x10    // 远程温度小数部分（高4位有效）

extern int tmp451_fd;
extern float local_temp;


/**
 * @brief 初始化TMP451传感器
 * @return 成功返回I2C文件描述符，失败返回-1
 */
extern int tmp451_init(void);

/**
 * @brief 读取TMP451指定类型的温度值（严格按手册解析）
 * @param fd: tmp451_init返回的文件描述符
 * @param is_local: 1=读取本地温度，0=读取远程温度
 * @param temp: 输出参数，存储读取到的温度值（保留四位小数）
 * @return 0:成功, -1:失败
 */
int tmp451_read_temperature(int fd, int is_local, float *temp);

/**
 * @brief 读取本地温度（芯片自身温度）
 * @param fd: tmp451_init返回的文件描述符
 * @param temp: 输出参数，存储本地温度值
 * @return 0:成功, -1:失败
 */
int tmp451_read_local_temp(int fd, float *temp);

/**
 * @brief 读取远程温度（外接热敏电阻温度）
 * @param fd: tmp451_init返回的文件描述符
 * @param temp: 输出参数，存储远程温度值
 * @return 0:成功, -1:失败
 */
int tmp451_read_remote_temp(int fd, float *temp);

/**
 * @brief 关闭TMP451传感器（释放I2C文件描述符）
 * @param fd: tmp451_init返回的文件描述符
 */
void tmp451_close(int fd);

/**
 * @brief 单独读取TMP451某个寄存器的值
 * @param fd: I2C文件描述符
 * @param reg: 寄存器地址
 * @param value: 输出参数，存储寄存器值
 * @return 0:成功, -1:失败
 */
int tmp451_read_reg(int fd, unsigned char reg, unsigned char *value);

extern void tmp451_func(void);

#endif // TMP451_H