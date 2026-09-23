#ifndef FPGA_INIT_H
#define FPGA_INIT_H

#include "beam.h"

/*
 * 功能: 向映射后的 GPIO 寄存器写入指定值。
 * 参数: gpio_base 为 GPIO 基地址；offset 为寄存器偏移；value 为待写入值。
 * 返回值: 无。
 */
void gpio_write(void *gpio_base, unsigned int offset, unsigned int value);

/*
 * 功能: 读取映射后的 GPIO 寄存器值。
 * 参数: gpio_base 为 GPIO 基地址；offset 为寄存器偏移。
 * 返回值: 返回指定偏移地址处的寄存器值。
 */
unsigned int gpio_read(void *gpio_base, unsigned int offset);

/*
 * 功能: 通过 sysfs 导出指定 GPIO 管脚。
 * 参数: gpio 为 GPIO 管脚号。
 * 返回值: 成功返回 0；失败返回文件描述符错误值。
 */
int gpio_export(unsigned int gpio);

/*
 * 功能: 通过 sysfs 取消导出指定 GPIO 管脚。
 * 参数: gpio 为 GPIO 管脚号。
 * 返回值: 成功返回 0；失败返回文件描述符错误值。
 */
int gpio_unexport(unsigned int gpio);

/*
 * 功能: 设置指定 GPIO 管脚方向。
 * 参数: gpio 为 GPIO 管脚号；out_flag 非 0 表示输出，0 表示输入。
 * 返回值: 成功返回 0；失败返回文件描述符错误值。
 */
int gpio_set_dir(unsigned int gpio, unsigned int out_flag);

/*
 * 功能: 设置指定 GPIO 管脚输出电平。
 * 参数: gpio 为 GPIO 管脚号；value 为输出值，0 为低电平，非 0 为高电平。
 * 返回值: 成功返回 0；失败返回文件描述符错误值。
 */
int gpio_set_value(unsigned int gpio, unsigned int value);

/*
 * 功能: 从 UIO sysfs size 文件读取映射内存大小。
 * 参数: sysfs_path_file 为 /sys/class/uio/.../size 文件路径。
 * 返回值: 返回解析出的内存大小，单位为字节。
 */
unsigned int get_memory_size(const char *sysfs_path_file);

/*
 * 功能: 打开 UIO 设备并 mmap 对应物理地址空间。
 * 参数: uio_parameter 为 UIO 配置结构体，函数会填充 fd、mem_size 和 mem_ptr。
 * 返回值: 无；打开或映射失败时打印错误并退出进程。
 */
void uio_init(UIO_CONFIG_PARAMETER *uio_parameter);

/*
 * 功能: 初始化 FPGA 相关 UIO 映射和共享内存指针。
 * 参数: 无。
 * 返回值: 无。
 */
void fpga_interface_init(void);

/*
 * 功能: 将 EPLD 相关 MIO32 管脚初始化为低电平。
 * 参数: 无。
 * 返回值: 无。
 */
void init_EPLD_MIO32_bit_low(void);

/*
 * 功能: 将 EPLD 相关 MIO33 管脚初始化为高电平。
 * 参数: 无。
 * 返回值: 无。
 */
void init_EPLD_MIO33_bit_hign(void);

/*
 * 功能: 将 EPLD 相关 MIO28 管脚初始化为高电平。
 * 参数: 无。
 * 返回值: 无。
 */
void init_EPLD_MIO28_bit_hign(void);

#endif
