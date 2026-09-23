#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "stdio.h"
#include "main.h"
#include "fpga_init.h"
#include "armtofpga.h"
#include "armtodsp.h"

int fd_uio5 = 0, fd_uio6 = 0;//中断的驱动设备名称
int fd = 0;

/*****************************************UIO结构体地址映射*****************************************/
UIO_CONFIG_PARAMETER uio_fpga_register = { .physical_addr = (unsigned int*)FPGA_REGISTER_BASEADDR, .uiod = "/dev/uio0", .sysfs_path_file = "/sys/class/uio/uio0/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_ddr_mem_IQ_1 = { .physical_addr = (unsigned int*)DDR_SHARE_MEM_BASEADDR_IQ_1, .uiod = "/dev/uio1", .sysfs_path_file = "/sys/class/uio/uio1/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_ddr_mem_IQ_2 = { .physical_addr = (unsigned int*)DDR_SHARE_MEM_BASEADDR_IQ_2, .uiod = "/dev/uio2", .sysfs_path_file = "/sys/class/uio/uio2/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_ddr_mem_IQ_3 = { .physical_addr = (unsigned int*)DDR_SHARE_MEM_BASEADDR_IQ_3, .uiod = "/dev/uio3", .sysfs_path_file = "/sys/class/uio/uio3/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_dspreturn_mem = { .physical_addr = (unsigned int*)DDR_SHARE_MEM_BASEADDR_IMAGE_0, .uiod = "/dev/uio4", .sysfs_path_file = "/sys/class/uio/uio4/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_extsensor_bram = { .physical_addr = (unsigned int*)TVG_REGISTER_BASEADDR, .uiod = "/dev/uio6", .sysfs_path_file = "/sys/class/uio/uio6/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_beamangel_bram = { .physical_addr = (unsigned int*)TVG_ROLL_BASEADDR, .uiod = "/dev/uio8", .sysfs_path_file = "/sys/class/uio/uio8/maps/map0/size" };
UIO_CONFIG_PARAMETER uio_fpga_register_2 = { .physical_addr = (unsigned int*)FPGA_REGISTER_BASEADDR_2, .uiod = "/dev/uio9", .sysfs_path_file = "/sys/class/uio/uio9/maps/map0/size" };
//湿端发射功能上移部分寄存器
UIO_CONFIG_PARAMETER uio_wet_fpga_register = { .physical_addr = (unsigned int *)WET_FPGA_REGISTER_BASEADDR, .uiod = "/dev/uio10", .sysfs_path_file = "/sys/class/uio/uio10/maps/map0/size"}; 
UIO_CONFIG_PARAMETER uio_mark_fpga_register = { .physical_addr = (unsigned int *)MARK_FPGA_REGISTER_BASEADDR, .uiod = "/dev/uio11", .sysfs_path_file = "/sys/class/uio/uio11/maps/map0/size"};

UIO_CONFIG_PARAMETER uio_dspreturn_mem2 = {0}, uio_sensor_mem_1 = {0}, uio_sensor_mem_2 = {0}, uio_sensor_mem_3 = {0}, uio_sensor_mem_4 = {0}, uio_sensor_mem_8 = {0}, uio_sensor_mem_9 = {0}, uio_sensor_mem_A = {0}, uio_sensor_mem_B = {0};

/*****************************************************************
 * * * 名称：                    get_memory_size
 * * * 功能：                    获取内存长度值
 * * * 入口参数：            *sysfs_path_file:路径
 * * * 出口参数：           内存长度，类型为unsigned int
 * * *****************************************************************/
unsigned int get_memory_size(const char* sysfs_path_file)
{
    FILE* size_fp;
    unsigned int size;
    size_fp = fopen(sysfs_path_file, "r");
    if (!size_fp)
    {
        printf("unable to open the uio size file\n");
        exit(-1);
    }
    fscanf(size_fp, "0x%08X", &size);
    fclose(size_fp);
    return size;
}

/*****************************************************************
 * * 名称：                      uio_init
 * * 功能：                      初始化uio
 * * 入口参数：                  地址映射
 * * 出口参数：                     无
 * *****************************************************************/
void uio_init(UIO_CONFIG_PARAMETER* uio_parameter)
{
    uio_parameter->fd = open(uio_parameter->uiod, O_RDWR);
    if (uio_parameter->fd < 1)
    {
        printf("Invalid UIO device file:%s.\n", uio_parameter->uiod);
    }
    uio_parameter->mem_size = get_memory_size(uio_parameter->sysfs_path_file);
    uio_parameter->mem_ptr = mmap(uio_parameter->physical_addr, uio_parameter->mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, uio_parameter->fd, 0);
    if (uio_parameter->mem_ptr == MAP_FAILED)
    {
        printf("Mmap call failure1.\n");
    }
}

/******************************Interface_init***************************************
 * * 功能：                    FPGA接口初始化
 * * 入口参数：                     无
 * * 出口参数：                     无
 * *********************************************************************************/
void fpga_interface_init(void)
{
    uio_init(&uio_fpga_register);
    uio_init(&uio_fpga_register_2);
    uio_init(&uio_wet_fpga_register);
    uio_init(&uio_mark_fpga_register);
//    printf("uio_fpga_register.mem_ptr=%x\n", uio_fpga_register.mem_ptr);
//    printf("uio_fpga_register_2.mem_ptr=%x\n", uio_fpga_register_2.mem_ptr);
//    printf("uio_wet_fpga_register.mem_ptr=%x\n", uio_wet_fpga_register.mem_ptr);
    //uio_fpga_register_2.mem_ptr=(int *)((char *)(uio_fpga_register.mem_ptr) + 0x10000);
    uio_init(&uio_ddr_mem_IQ_1);
    uio_init(&uio_ddr_mem_IQ_2);
    uio_init(&uio_ddr_mem_IQ_3);
    uio_init(&uio_dspreturn_mem);
    uio_dspreturn_mem2.mem_ptr = (int*)((char*)(uio_dspreturn_mem.mem_ptr) + 0x500000);
    printf("uio_dspreturn_mem.mem_ptr=%x\n", uio_dspreturn_mem.mem_ptr);
    printf("uio_dspreturn_mem2.mem_ptr=%x\n", uio_dspreturn_mem2.mem_ptr);
    DBG("test 5\n");
    uio_sensor_mem_1.mem_ptr = (int*)((char*)(uio_dspreturn_mem.mem_ptr) + 0xA00000);
    uio_sensor_mem_2.mem_ptr = (int*)((char*)(uio_dspreturn_mem.mem_ptr) + 0xA10000);
    uio_sensor_mem_3.mem_ptr = (int*)((char*)(uio_dspreturn_mem.mem_ptr) + 0xA20000);
    uio_sensor_mem_4.mem_ptr = (int*)((char*)(uio_dspreturn_mem.mem_ptr) + 0xA30000);
    printf("uio_sensor_mem_1.mem_ptr=%x\n", uio_sensor_mem_1.mem_ptr);
    printf("uio_sensor_mem_2.mem_ptr=%x\n", uio_sensor_mem_2.mem_ptr);
    printf("uio_sensor_mem_3.mem_ptr=%x\n", uio_sensor_mem_3.mem_ptr);
    printf("uio_sensor_mem_4.mem_ptr=%x\n", uio_sensor_mem_4.mem_ptr);
    uio_sensor_mem_8.mem_ptr = (int*)((char*)(uio_dspreturn_mem.mem_ptr) + 0xA40000);
    uio_sensor_mem_9.mem_ptr = (int*)((char*)(uio_dspreturn_mem.mem_ptr) + 0xA50000);
    uio_sensor_mem_A.mem_ptr = (int*)((char*)(uio_dspreturn_mem.mem_ptr) + 0xA60000);
    uio_sensor_mem_B.mem_ptr = (int*)((char*)(uio_dspreturn_mem.mem_ptr) + 0xA70000);
    printf("uio_sensor_mem_8.mem_ptr=%x\n", uio_sensor_mem_8.mem_ptr);
    printf("uio_sensor_mem_9.mem_ptr=%x\n", uio_sensor_mem_9.mem_ptr);
    printf("uio_sensor_mem_A.mem_ptr=%x\n", uio_sensor_mem_A.mem_ptr);
    printf("uio_sensor_mem_B.mem_ptr=%x\n", uio_sensor_mem_B.mem_ptr);
    uio_init(&uio_extsensor_bram);
    memset((char*)uio_extsensor_bram.mem_ptr, 0, 2048);
    uio_init(&uio_beamangel_bram);
    //uio_Mod_sensor_0.mem_ptr = (int *)((char *)(uio_dspreturn_mem.mem_ptr) + 0x2800000);
    //uio_Mod_sensor_1.mem_ptr = (int *)((char *)(uio_dspreturn_mem2.mem_ptr) + 0x2800000);

    
    
    //uio中断
    char* uiod = "/dev/uio7";
    fd_uio6 = open(uiod, O_RDWR);
    if (fd_uio6 < 1)
    {
        printf("Invalid UIO device file:%s.\n", uiod);
    }
    //printf("uio_fpga_register_2.mem_ptr=%x\n", uio_fpga_register_2.mem_ptr);
    
    printf("uio_fpga_register.mem_ptr=%x\n", uio_fpga_register.mem_ptr);
    printf("uio_fpga_register_2.mem_ptr=%x\n", uio_fpga_register_2.mem_ptr);
    printf("uio_wet_fpga_register.mem_ptr=%x\n", uio_wet_fpga_register.mem_ptr);
    printf("uio_mark_fpga_register.mem_ptr=%x\n", uio_mark_fpga_register.mem_ptr);
    ptr_fpga_register_data = (FPGA_REGISTERS*)uio_fpga_register.mem_ptr;
    ptr_fpga_register_addr = (FPGA_rapidio_REGISTERS*)uio_fpga_register_2.mem_ptr;
    ptr_wet_fpga_register_data = (WET_FPGA_REGISTERS*)uio_wet_fpga_register.mem_ptr;
    ptr_mark_registers = (char*)uio_mark_fpga_register.mem_ptr;
    fd = open(SYSFS_GPIO_EXPORT, O_WRONLY);
    if (fd == -1)
    {
        printf("ERR: export open error.\n");
    }
    write(fd, SYSFS_GPIO_RUN_LIGHT, sizeof(SYSFS_GPIO_RUN_LIGHT));
    close(fd);
}

