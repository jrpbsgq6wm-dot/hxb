/*******************************eeprom_vendor.c-2024-06-21************************/
/******************************显控厂商信息 - eeprom (测试使用)************************/
#ifndef _EEPROM_VENDOR_H_
#define _EEPROM_VENDOR_H_

#include "CuBeamOne.h"
#include "network.h"
/*
  eeprom 256Kbit = 32KB
    0   8    128     248(byte)... 256(byte)      1024+256(byte)        32KB
    |---|-----|-------|------------|-------------------|-----------------|
    |声速 传感器温度标定-|------------|---MSG(1024byte)---|
*/
#define EEPROM_SEEK_VENDOR      256   //厂商信息存储位置基于eeprom的偏移
#define EEPROM_WR_SIZE_VENDOR   1024  //厂商信息大小
#define EEPROM_DEV_NAME               "/sys/bus/i2c/devices/i2c-0/0-0055/eeprom"

//#define CUBENAMONE_EEPROM_DEF_MSG     "*****************************************\n" \
                                      "*              CuBeam_v1.0              *\n" \
                                      "*               2024-6-24               *\n" \
                                      "*              测试版本 1.0             *\n"  \
                                      "*****************************************\0"

extern int eeprom_read_vendor(int fd_eeprom);
extern int eeprom_write_vendor(int fd_eeprom);
extern int eeprom_init();
extern int eeprom_option(void);
extern void eeprom_file_mapping(void);
#endif  /*eeprom_vendor.h*/