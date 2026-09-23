#ifndef __FPGA_INIT_H
#define __FPGA_INIT_H



extern int fd_uio5, fd_uio6;//中断的驱动设备名称
extern int fd;

/********************************FPGA memory UIO0******************************/
#define   TVG_REGISTER_BASEADDR              0x40000000//ARM给FPGA进行TVG下发的基地址
#define   TVG_ROLL_BASEADDR		             0x42000000//ARM给FPGA进行TVG下发的基地址
#define   FPGA_REGISTER_BASEADDR             0x43C00000//ARM给FPGA进行参数配置的寄存器基地址
#define   FPGA_REGISTER_BASEADDR_2	         0x43C10000
#define   WET_FPGA_REGISTER_BASEADDR	     0x43C20000
#define   MARK_FPGA_REGISTER_BASEADDR         0x43C30000
/*********************************DDR memory UIO1*****************************/
#define   DDR_SHARE_MEM_BASEADDR_IQ_1       0x19000000//DDR基地址IQ_buffer1
#define   DDR_SHARE_MEM_BASEADDR_IQ_2		0x25C00000//DDR基地址IQ_buffer2
#define   DDR_SHARE_MEM_BASEADDR_IQ_3		0x32800000//DDR基地址IQ_buffer3
#define	  DDR_SHARE_MEM_BASEADDR_IMAGE_0	0x3F400000//dsp return data



//MIO运行状态灯
#define SYSFS_GPIO_EXPORT                "/sys/class/gpio/export" 
#define SYSFS_GPIO_RUN_LIGHT             "962"                         
#define SYSFS_GPIO_RUN_LIGHT_DIR         "/sys/class/gpio/gpio962/direction"
#define SYSFS_GPIO_RUN_LIGHT_VAL         "/sys/class/gpio/gpio962/value"
//输入输出设置
#define SYSFS_GPIO_OUT                   "out" 
#define SYSFS_GPIO_IN                    "in"
//高低电平设置
#define SYSFS_GPIO_VAL_H                 "1"
#define SYSFS_GPIO_VAL_L                 "0"




/***************************UIO配置结构体**********************/
typedef struct
{
    int               fd;    //文件描述符
    char              * uiod;  //设备驱动名称
    char              * sysfs_path_file; //设备驱动所在路径
    unsigned int      * physical_addr; //物理地址
    int               mem_size;  //大小
    int               * mem_ptr;  //映射回的指针
}UIO_CONFIG_PARAMETER;

extern UIO_CONFIG_PARAMETER uio_fpga_register;
extern UIO_CONFIG_PARAMETER uio_fpga_register2;
extern UIO_CONFIG_PARAMETER uio_ddr_mem_IQ_1;
extern UIO_CONFIG_PARAMETER uio_ddr_mem_IQ_2;
extern UIO_CONFIG_PARAMETER uio_ddr_mem_IQ_3;
extern UIO_CONFIG_PARAMETER uio_dspreturn_mem;
extern UIO_CONFIG_PARAMETER uio_extsensor_bram;
extern UIO_CONFIG_PARAMETER uio_beamangel_bram;
extern UIO_CONFIG_PARAMETER uio_wet_fpga_register;
extern UIO_CONFIG_PARAMETER uio_dspreturn_mem2, uio_sensor_mem_1, uio_sensor_mem_2, uio_sensor_mem_3, uio_sensor_mem_4, uio_sensor_mem_8, uio_sensor_mem_9, uio_sensor_mem_A, uio_sensor_mem_B;

extern void uio_init(UIO_CONFIG_PARAMETER* uio_parameter);
extern void fpga_interface_init(void);

#endif
