
#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"

/*************************************GPIO相关**************************************/
#define SYSFS_GPIO_DIR "/sys/class/gpio" // GPIO路径
#define MAX_BUF 64                       // 写入的最大字节数

/**************************************以下为函数定义&声明**************************************/

/*写GPIO,参数分别为基地址，偏移地址，待写入的值*/
void gpio_write(void *gpio_base, unsigned int offset, unsigned int value)
{
    *((volatile unsigned *)(gpio_base + offset)) = value;
}

/*读GPIO，参数分别为基地址，偏移地址，函数返回该偏移地址的内容*/
unsigned int gpio_read(void *gpio_base, unsigned int offset)
{
    return *((volatile unsigned *)(gpio_base + offset));
}

/*打开GPIO*/
int gpio_export(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF];
    fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/export");
        return fd;
    }
    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    return 0;
}

/*关闭GPIO*/
int gpio_unexport(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF];
    fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);

    if (fd < 0)
    {
        perror("gpio/export");
        return fd;
    }
    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    return 0;
}

/*设置GPIO方向，参数gpio:管脚号，out_flag:非0为输出0为输入*/
int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
    int fd, len;
    char buf[MAX_BUF];
    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/direction", gpio);
    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/direction");
        return fd;
    }
    if (out_flag)
        write(fd, "out", 4);
    else
        write(fd, "in", 3);
    close(fd);
    return 0;
}

/*设置gpio，参数gpio为管脚号，value:0置低1置高 */
int gpio_set_value(unsigned int gpio, unsigned int value)
{
    int fd, len;
    char buf[MAX_BUF];
    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(buf, O_WRONLY);

    if (fd < 0)
    {
        perror("gpio/set-value");
        return fd;
    }
    if (value)
        write(fd, "1", 2);
    else
        write(fd, "0", 2);
    close(fd);
    return 0;
}

/********************************************************************************
 * 名称：                  init_EPLD_MIO32_bit_low
 * 功能：                  初始化EPLD管脚MIO32为低电平
 * 入口参数：               无
 * 出口参数：               无
 *********************************************************************************/
void init_EPLD_MIO32_bit_low(void)
{
    gpio_export(938);       // 打开端口938
    gpio_set_dir(938, 1);   // 端口938为输出模式
    gpio_set_value(938, 0); // 低电平
    usleep(10);
}

/********************************************************************************
 * 名称：                  init_EPLD_MIO33_bit_hign
 * 功能：                  初始化EPLD管脚MIO33为高电平
 * 入口参数：               无
 * 出口参数：               无
 *********************************************************************************/
void init_EPLD_MIO33_bit_hign(void)
{
    gpio_export(939);       // 打开端口938
    gpio_set_dir(939, 1);   // 端口938为输出模式
    gpio_set_value(939, 1); // 高电平
    usleep(10);
}

/********************************************************************************
 * 名称：                  init_EPLD_MIO28_bit_hign
 * 功能：                  初始化EPLD管脚MIO28为输出高电平
 * 入口参数：               无
 * 出口参数：               无
 *********************************************************************************/
void init_EPLD_MIO28_bit_hign(void)
{
    gpio_export(934);       // 打开端口938
    gpio_set_dir(934, 1);   // 端口938为输出模式
    gpio_set_value(934, 1); // 高电平
    usleep(10);
}