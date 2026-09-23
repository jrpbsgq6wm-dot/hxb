#include "eeprom.h"
#include "network.h"
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define I2C_DEV_PATH "/dev/i2c-0"
#define EEPROM_I2C_ADDR 0x55

unsigned int eeprom_fd;

/**
 * AT24C256初始化 - 使用I2C设备方式
 */
int at24c256_init(void){
    int fd;
    
    // 打开I2C总线设备
    fd = open(I2C_DEV_PATH, O_RDWR);
    if (fd < 0) {
        perror("open i2c device failed");
        return -1;
    }
    
    // 设置从设备地址
    if (ioctl(fd, I2C_SLAVE, EEPROM_I2C_ADDR) < 0) {
        perror("ioctl set slave address failed");
        close(fd);
        return -1;
    }
    
    printf("EEPROM initialized successfully at 0x%02x\n", EEPROM_I2C_ADDR);
    return fd;
}

/**
 * 内部函数：检查地址合法性
 */
static int check_addr(unsigned int addr) {
    if (addr >= AT24C256_TOTAL_BYTES) {
        fprintf(stderr, "Error: Address 0x%04X out of range (0~0x7FFF)\n", addr);
        return -1;
    }
    return 0;
}

/**
 * 单字节读取
 */
int at24c256_read_byte(unsigned int fd, unsigned int addr, unsigned char *data) {
    unsigned char addr_buf[2];
    ssize_t ret;
    
    if (check_addr(addr) < 0 || data == NULL) {
        return -1;
    }
    
    // 构造16位地址（高字节在前）
    addr_buf[0] = (addr >> 8) & 0xFF;
    addr_buf[1] = addr & 0xFF;
    
    // 先写地址
    if (write(fd, addr_buf, 2) != 2) {
        perror("write address failed");
        return -1;
    }
    
    // 再读取数据
    ret = read(fd, data, 1);
    if (ret != 1) {
        fprintf(stderr, "Read byte failed (addr:0x%04X)\n", addr);
        return -1;
    }
    
    return 0;
}

/**
 * 单字节写入
 */
int at24c256_write_byte(unsigned int fd, unsigned int addr, unsigned char data) {
    unsigned char write_buf[3];
    ssize_t ret;
    
    if (check_addr(addr) < 0) {
        return -1;
    }
    
    // 构造写入命令：高地址 + 低地址 + 数据
    write_buf[0] = (addr >> 8) & 0xFF;
    write_buf[1] = addr & 0xFF;
    write_buf[2] = data;
    
    ret = write(fd, write_buf, 3);
    usleep(1000);  // 1ms延时，等待写入完成
    
    if (ret != 3) {
        fprintf(stderr, "Write byte failed (addr:0x%04X)\n", addr);
        return -1;
    }
    
    return 0;
}

/**
 * 多字节读取
 */
int at24c256_read_multi(unsigned int fd, unsigned int start_addr, unsigned char *buf, unsigned int len) {
    unsigned char addr_buf[2];
    ssize_t ret;
    unsigned int end_addr = start_addr + len - 1;
    
    if (check_addr(start_addr) < 0 || check_addr(end_addr) < 0 || buf == NULL || len == 0) {
        return -1;
    }
    
    // 构造16位地址
    addr_buf[0] = (start_addr >> 8) & 0xFF;
    addr_buf[1] = start_addr & 0xFF;
    
    // 写地址
    if (write(fd, addr_buf, 2) != 2) {
        perror("write address failed");
        return -1;
    }
    
    // 读取数据
    ret = read(fd, buf, len);
    if (ret < 0) {
        perror("read multi failed");
        return -1;
    }
    
    return (int)ret;
}

/**
 * 多字节写入（处理页对齐）
 */
int at24c256_write_multi(unsigned int fd, unsigned int start_addr, const unsigned char *buf, unsigned int len) {
    unsigned int end_addr = start_addr + len - 1;
    unsigned int written = 0;
    unsigned int current_addr = start_addr;
    unsigned char *write_buf;
    unsigned int write_len;
    ssize_t ret;
    
    if (check_addr(start_addr) < 0 || check_addr(end_addr) < 0 || buf == NULL || len == 0) {
        return -1;
    }
    
    while (written < len) {
        // 计算当前页剩余字节数（AT24C256页大小64字节，注意不是256！）
        unsigned int page_remain = AT24C256_PAGE_SIZE - (current_addr % AT24C256_PAGE_SIZE);
        write_len = (len - written) < page_remain ? (len - written) : page_remain;
        
        // 分配缓冲区：2字节地址 + 数据
        write_buf = malloc(write_len + 2);
        if (!write_buf) {
            perror("malloc failed");
            return -1;
        }
        
        // 构造写入命令
        write_buf[0] = (current_addr >> 8) & 0xFF;
        write_buf[1] = current_addr & 0xFF;
        memcpy(write_buf + 2, &buf[written], write_len);
        
        // 写入数据
        ret = write(fd, write_buf, write_len + 2);
        free(write_buf);
        
        if (ret != (ssize_t)(write_len + 2)) {
            fprintf(stderr, "Write multi failed (addr:0x%04X, len:%u)\n", current_addr, write_len);
            return -1;
        }
        
        // 页写入需要等待（AT24C256典型5ms）
        usleep(5000);
        
        written += write_len;
        current_addr += write_len;
    }
    
    return (int)written;
}

// 其余函数保持不变...
int eeprom_read_vendor(void){
    char send_head[4] = {'#','#','R','E'};
    char read_buffer[EEPROM_WR_SIZE_VENDOR];
    char send_buffer[EEPROM_WR_SIZE_VENDOR+5];
    memset(read_buffer, 0, EEPROM_WR_SIZE_VENDOR);
    int read_len, str_len, bufcount;
    
    read_len = at24c256_read_multi(eeprom_fd, 0, read_buffer, EEPROM_WR_SIZE_VENDOR);
    if(read_len == -1){
        perror("Read vendor info failed\n");
        return -1; 
    }
    
    for(bufcount=0; bufcount<1024; bufcount++){
        if(read_buffer[bufcount] == '\0'){
            break;
        }
    }
    
    if((read_len == (int)1024) && (bufcount == (int)1024)){
        char buf[5] = {'4','0','7','0','\0'};
        // 向0地址写入默认ID 4070
        at24c256_write_multi(eeprom_fd, 0, buf, 5);
        memcpy(send_buffer, send_head, 4);
        send_buffer[4] = 4;
        send_buffer[5] = '4';
        send_buffer[6] = '0';
        send_buffer[7] = '7';
        send_buffer[8] = '0';
        if(send(Connect_fd, send_buffer, 4+5, 0) < 0){
            perror("send error\n");
            error_process();
            return -1;
        }
    } else {
        memcpy(send_buffer, send_head, 4);
        send_buffer[4] = bufcount;
        memcpy(send_buffer+5, read_buffer, bufcount);
        
        if(send(Connect_fd, send_buffer, bufcount+5, 0) < 0){
            perror("send error\n");
            error_process();
            return -1;
        }
    }
    return 0;
}

int eeprom_write_vendor(void){
    char send_head[4] = {'#','#','R','E'};
    char write_buffer[EEPROM_WR_SIZE_VENDOR];
    int recv_len, write_len;
    
    memset(write_buffer, 0, sizeof(write_buffer));
    recv_len = recv(Connect_fd, write_buffer, EEPROM_WR_SIZE_VENDOR, 0); 
    if (recv_len < 0) {
        error_process();
        return -1;
    }
    write_buffer[recv_len] = '\0';
    write_len = at24c256_write_multi(eeprom_fd, 0, write_buffer, recv_len+1);
    if(write_len < 0) {
        perror("Write Vendor Info to eeprom failed\n");
        return -1;
    }
    return 0;
}

int eeprom_id_func(void){
    int ret;
    char DataHead[6];
    ret = recv(Connect_fd, DataHead, 6, 0);
    if (ret <= 0) {
        error_process();
        return -1;
    }
    if(DataHead[0] == '_' && DataHead[1] == 'R' && DataHead[2] == 'E' && DataHead[3] == 'A' && DataHead[4] == 'D' && DataHead[5] == '_'){
        printf("read eeprom vendor msg send to upper\n");
        if(eeprom_read_vendor() == -1){
            perror("eeprom_read_vendor error\n");
            close(eeprom_fd);
            return -1;
        }
    } else if(DataHead[0] == '_' && DataHead[1] == 'W' && DataHead[2] == 'R' && DataHead[3] == 'I' && DataHead[4] == 'T' && DataHead[5] == 'E'){
        printf("write vendor msg to eeprom\n");
        if(eeprom_write_vendor() == -1){
            perror("eeprom_write_vendor error\n");
            close(eeprom_fd);
            return -1;
        }
    }
    return 0;
}