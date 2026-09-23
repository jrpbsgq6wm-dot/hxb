#ifndef EEPROM_H
#define EEPROM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define AT24C256_TOTAL_BYTES 32768  // 32KB
#define AT24C256_PAGE_SIZE   64     // AT24C256页大小是64字节，不是256！

#define EEPROM_WR_SIZE_VENDOR 1024  // 厂商信息区大小

// 外部变量
extern unsigned int eeprom_fd;

// 函数声明
int at24c256_init(void);  // 不再需要PATH参数
int at24c256_read_byte(unsigned int fd, unsigned int addr, unsigned char *data);
int at24c256_write_byte(unsigned int fd, unsigned int addr, unsigned char data);
int at24c256_read_multi(unsigned int fd, unsigned int start_addr, unsigned char *buf, unsigned int len);
int at24c256_write_multi(unsigned int fd, unsigned int start_addr, const unsigned char *buf, unsigned int len);

int eeprom_read_vendor(void);
int eeprom_write_vendor(void);
int eeprom_id_func(void);

#endif