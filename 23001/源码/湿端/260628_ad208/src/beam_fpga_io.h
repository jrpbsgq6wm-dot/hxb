#ifndef BEAM_FPGA_IO_H
#define BEAM_FPGA_IO_H

#include "beam_common.h"

void gpio_write(void *gpio_base, unsigned int offset, unsigned int value);
unsigned int gpio_read(void *gpio_base, unsigned int offset);
int gpio_export(unsigned int gpio);
int gpio_unexport(unsigned int gpio);
int gpio_set_dir(unsigned int gpio, unsigned int out_flag);
int gpio_set_value(unsigned int gpio, unsigned int value);
unsigned int get_memory_size(const char *sysfs_path_file);
void uio_init(UIO_CONFIG_PARAMETER *uio_parameter);
void fpga_interface_init(void);
void init_EPLD_MIO32_bit_low(void);
void init_EPLD_MIO33_bit_hign(void);
void init_EPLD_MIO28_bit_hign(void);

#endif /* BEAM_FPGA_IO_H */