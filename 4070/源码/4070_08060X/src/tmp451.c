#include "tmp451.h"

int tmp451_fd;
float local_temp;

/**
 * 初始化TMP451：打开I2C总线 + 绑定从设备地址
 */
int tmp451_init(void){
    int fd;
    // 1. 打开I2C总线设备文件
    fd = open(TMP451_I2C_BUS, O_RDWR);
    if (fd < 0){
        fprintf(stderr, "ERROR: 打开I2C总线失败 <%s> : %s\n", TMP451_I2C_BUS, strerror(errno));
        return -1;
    }
    // 2. 设置I2C从设备地址（TMP451@0x4C）
    if (ioctl(fd, I2C_SLAVE, TMP451_I2C_ADDR) < 0){
        fprintf(stderr, "ERROR: 设置I2C地址失败 (0x%02X) : %s\n", TMP451_I2C_ADDR, strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

/**
 * 单独读取某个寄存器的值
 */
int tmp451_read_reg(int fd, unsigned char reg, unsigned char *value)
{
    int ret;

    if (fd < 0 || value == NULL)
    {
        fprintf(stderr, "ERROR: 输入参数无效\n");
        return -1;
    }

    // 1. 写入要读取的寄存器地址
    ret = write(fd, &reg, 1);
    if (ret != 1)
    {
        fprintf(stderr, "ERROR: 写入寄存器地址(0x%02X)失败 : %s\n", reg, strerror(errno));
        return -1;
    }

    // 2. 读取1字节寄存器值
    ret = read(fd, value, 1);
    if (ret != 1)
    {
        fprintf(stderr, "ERROR: 读取寄存器(0x%02X)值失败 : %s\n", reg, strerror(errno));
        return -1;
    }

    return 0;
}

/**
 * 通用温度读取函数
 */
int tmp451_read_temperature(int fd, int is_local, float *temp)
{
    unsigned char reg_int, reg_frac;
    unsigned char int_val, frac_val;
    float frac;

    if (fd < 0 || temp == NULL)
    {
        fprintf(stderr, "ERROR: 输入参数无效\n");
        return -1;
    }

    // 1. 确定要读取的寄存器（本地/远程）
    if (is_local)
    {
        reg_int = TMP451_REG_LOCAL_TEMP_INT;
        reg_frac = TMP451_REG_LOCAL_TEMP_FRAC;
    }
    else
    {
        reg_int = TMP451_REG_REMOTE_TEMP_INT;
        reg_frac = TMP451_REG_REMOTE_TEMP_FRAC;
    }

    // 2. 读取整数部分寄存器
    if (tmp451_read_reg(fd, reg_int, &int_val) != 0)
    {
        fprintf(stderr, "ERROR: 读取温度整数部分失败\n");
        return -1;
    }

    // 3. 读取小数部分寄存器（仅高4位有效）
    if (tmp451_read_reg(fd, reg_frac, &frac_val) != 0)
    {
        fprintf(stderr, "ERROR: 读取温度小数部分失败\n");
        return -1;
    }

    // 4. 解析温度值（严格按手册）
    // 整数部分：直接取int_val
    // 小数部分：frac_val的高4位 × 0.0625（4位LSB的分辨率）
    frac = ((frac_val >> 4) & 0x0F) * 0.0625;
    *temp = int_val + frac;

    return 0;
}

/**
 * 读取本地温度（封装）
 */
int tmp451_read_local_temp(int fd, float *temp)
{
    return tmp451_read_temperature(fd, 1, temp);
}

/**
 * 读取远程温度（暂时不用）
 */
int tmp451_read_remote_temp(int fd, float *temp)
{
    return tmp451_read_temperature(fd, 0, temp);
}

/**
 * 关闭TMP451
 */
void tmp451_close(int fd)
{
    if (fd >= 0)
    {
        close(fd);
        printf("TMP451已关闭\n");
    }
}

void tmp451_func(void){
    while(1){
        tmp451_read_local_temp(tmp451_fd,&local_temp);
        sleep(1);
    }
}