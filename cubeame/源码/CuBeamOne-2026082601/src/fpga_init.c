

#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"

int fd_uio4; // 中断的驱动设备名称;

static void Read_Fpga_Register_Data(void);
static void print_uio_baseddr(void);

/**************************************UIO结构体**************************************/
UIO_CONFIG_PARAMETER uio_fpga_register = {/*.physical_addr = (unsigned int *)FPGA_REGISTER_BASEADDR, */.uiod = "/dev/uio0", .sysfs_path_file = "/sys/class/uio/uio0/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_mems_register = {/*.physical_addr = (unsigned int *)MEMS_REGISTER_BASEADDR, */.uiod = "/dev/uio1", .sysfs_path_file = "/sys/class/uio/uio1/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_tvg_register = {/*.physical_addr = (unsigned int *)TVG_REGISTER_BASEADDR, */.uiod = "/dev/uio2", .sysfs_path_file = "/sys/class/uio/uio2/maps/map0/size"};

UIO_CONFIG_PARAMETER uio_baseddr_head = {/*.physical_addr = (unsigned int *)DDR_REGISTER_BASEADDR, */.uiod = "/dev/uio3", .sysfs_path_file = "/sys/class/uio/uio3/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_original = {/*.physical_addr = (unsigned int *)DDR_ORIGINAL_BASEADDR,*/ .uiod = "/dev/uio5", .sysfs_path_file = "/sys/class/uio/uio5/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_iq = {/*.physical_addr = (unsigned int *)DDR_IQ_BASEADDR,*/ .uiod = "/dev/uio6", .sysfs_path_file = "/sys/class/uio/uio6/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_sensor = {/*.physical_addr = (unsigned int *)DDR_SENSOR_BASEADDR,*/ .uiod = "/dev/uio7", .sysfs_path_file = "/sys/class/uio/uio7/maps/map0/size"};

UIO_CONFIG_PARAMETER uio_baseddr_head_1 = {/*.physical_addr = (unsigned int *)DDR_REGISTER_BASEADDR_1,*/ .uiod = "/dev/uio8", .sysfs_path_file = "/sys/class/uio/uio8/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_original_1 = {/*.physical_addr = (unsigned int *)DDR_ORIGINAL_BASEADDR_1, */.uiod = "/dev/uio9", .sysfs_path_file = "/sys/class/uio/uio9/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_iq_1 = {/*.physical_addr = (unsigned int *)DDR_IQ_BASEADDR_1, */.uiod = "/dev/uio10", .sysfs_path_file = "/sys/class/uio/uio10/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_sensor_1 = {/*.physical_addr = (unsigned int *)DDR_SENSOR_BASEADDR_1,*/ .uiod = "/dev/uio11", .sysfs_path_file = "/sys/class/uio/uio11/maps/map0/size"};

/**************************************FPGA指令地址空间寄存器协议--结构体**************************************/

FPGA_REGISTERS fpga_register_data;                            // 下发给FPGA的参数配置寄存器；结构体变量
FPGA_REGISTERS *ptr_fpga_register_data = &fpga_register_data; // 结构体指针

FPGA_REGISTERS fpga_version_data;
FPGA_REGISTERS *ptr_fpga_version_data = &fpga_version_data;

/*****************************************************************
 * 名称：                    get_memory_size
 * 功能：                    获取内存长度值
 * 入口参数：            	 *sysfs_path_file:路径
 * 出口参数：            	 内存长度，类型为unsigned int
 *****************************************************************/
unsigned int get_memory_size(const char *sysfs_path_file)
{
    FILE *size_fp;
    unsigned int size;
    size_fp = fopen(sysfs_path_file, "r");
    if (!size_fp)
    {
        printf("unable to open the uio size file\n");
        exit(-1);
    }
    fscanf(size_fp, "0x%08X", &size); // 读文件的大小，%08x表示为8位格式，不够左边补齐，然后复制给size
    fclose(size_fp);
    return size;
}

/*****************************************************************
 * 名称：                    uio_init
 * 功能：                    初始化uio
 * 入口参数：            	 无
 * 出口参数：            	 无
 *****************************************************************/
void uio_init(UIO_CONFIG_PARAMETER *uio_parameter) // 用形参做指针指向结构体变量地址，然后通过函数中操作指针给结构体变量赋值，比如说&uio_fpga_register
{
    uio_parameter->fd = open(uio_parameter->uiod, O_RDWR);
    if (uio_parameter->fd < 1)
    {
        printf("Invalid UIO device file:%s.\n", uio_parameter->uiod);
    }
    uio_parameter->mem_size = get_memory_size(uio_parameter->sysfs_path_file);
    uio_parameter->mem_ptr = mmap(/*uio_parameter->physical_addr*/NULL, uio_parameter->mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, uio_parameter->fd, 0);
 
    if (uio_parameter->mem_ptr == MAP_FAILED)
    {
        printf("Mmap call failure1.\n");
        perror("UIO mmap");
    }
}

/********************************************************************************
 * 名称：                    fpga_interface_init
 * 功能：                    FPGA接口初始化
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void fpga_interface_init(void)
{
    DBG("uio init fpga register!\n");
    DBG("uio init fpga DMA register!\n");
    DBG("uio init fpga TVG register!\n");
    DBG("uio init fpga DDR register!\n");

    uio_init(&uio_fpga_register);
    uio_init(&uio_mems_register);
    uio_init(&uio_tvg_register);

    uio_init(&uio_baseddr_head);
    uio_init(&uio_baseddr_original);
    uio_init(&uio_baseddr_iq);
    uio_init(&uio_baseddr_sensor);

    uio_init(&uio_baseddr_head_1);
    uio_init(&uio_baseddr_original_1);
    uio_init(&uio_baseddr_iq_1);
    uio_init(&uio_baseddr_sensor_1);
    //打印地址
    //print_uio_baseddr();

    char *uiod = "/dev/uio4"; // 中断的配置uio4
    fd_uio4 = open(uiod, O_RDWR);
    // printf("fd_uio=%d\n", fd_uio4);
    if (fd_uio4 < 1)
    {
        printf("Invalid UIO device file:%s.\n", uiod);
    }

    ptr_fpga_register_data = (FPGA_REGISTERS *)(uio_fpga_register.mem_ptr); // 初始化的fpga参数寄存器的地址给ptr_fpga_register_data
    // ptr_fpga_register_data->fpga_sta = (FPGA_STATUS *)((int *)(char *)(uio_fpga_register.mem_ptr) + 0x100);
    ptr_fpga_version_data = (FPGA_REGISTERS *)((int *)(char *)(uio_fpga_register.mem_ptr) + 0x100);
    
    // printf("ptr_fpga_register_data->fpga_sta.date=%d\n", ptr_fpga_version_data->fpga_sta.date);
    // printf("ptr_fpga_register_data->fpga_sta.wstatus=%d\n", ptr_fpga_version_data->fpga_sta.wstatus);
    // printf("ptr_fpga_register_data->fpga_sta.version=%d\n", ptr_fpga_version_data->fpga_sta.version);

    //打印配置信息
    //Read_Fpga_Register_Data();
}
void Read_Fpga_Register_Data(void)
{
    //memcpy(ptr_fpga_register_data, uio_fpga_register.mem_ptr, SIZE_OF_DATA_FIRST);

    printf("ptr_fpga_register_data->wsm_con=%d\n", ptr_fpga_register_data->wsm_con);
    printf("ptr_fpga_register_data->set_pr=%d\n", ptr_fpga_register_data->set_pr);

    printf("ptr_fpga_register_data->fpga_registers_200k.wsm_registers_value.wsm_mod=%d\n", ptr_fpga_register_data->fpga_registers_200k.wsm_registers_value.wsm_mod);
    printf("ptr_fpga_register_data->fpga_registers_200k.wsm_registers_value.wsm_ct=%d\n", ptr_fpga_register_data->fpga_registers_200k.wsm_registers_value.wsm_ct);
    printf("ptr_fpga_register_data->fpga_registers_200k.adc_registers_value.adc_sct=%d\n", ptr_fpga_register_data->fpga_registers_200k.adc_registers_value.adc_sct);
    printf("ptr_fpga_register_data->fpga_registers_200k.adc_registers_value.adc_sn=%d\n", ptr_fpga_register_data->fpga_registers_200k.adc_registers_value.adc_sn);
    printf("ptr_fpga_register_data->fpga_registers_200k.dac_registers_value.dac_sct=%d\n", ptr_fpga_register_data->fpga_registers_200k.dac_registers_value.dac_sct);
    printf("ptr_fpga_register_data->fpga_registers_200k.dac_registers_value.dac_sn=%d\n", ptr_fpga_register_data->fpga_registers_200k.dac_registers_value.dac_sn);
    printf("ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.pwm_bf=%d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.pwm_bf);
    printf("ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.pwm_lfm=%d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.pwm_lfm);
    printf("ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.pwm_pulse=%d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.pwm_pulse);
    printf("ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.pwm_frequency=%d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.pwm_frequency);
    printf("ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sample_rate=%d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sample_rate);
    printf("ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.range=%d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.range);
    printf("ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.band_width=%d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.band_width);
    printf("ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.ping_rate=%d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.ping_rate);
    printf("ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.pwm_start=%d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.pwm_start);

    printf("ptr_fpga_register_data->fpga_sta.date=%d\n", ptr_fpga_version_data->fpga_sta.date);
    printf("ptr_fpga_register_data->fpga_sta.wstatus=%d\n", ptr_fpga_version_data->fpga_sta.wstatus);
    printf("ptr_fpga_register_data->fpga_sta.version=%d\n", ptr_fpga_version_data->fpga_sta.version);

}

void print_uio_baseddr(void)
{
    printf("uio_fpga_register.mem_ptr=%x\n", uio_fpga_register.mem_ptr);
    // printf("uio_dma_register.mem_ptr=%x\n", uio_dma_register.mem_ptr);
    printf("uio_tvg_register.mem_ptr=%x\n", uio_tvg_register.mem_ptr);

    printf("uio_baseddr_head.mem_ptr=%x\n", uio_baseddr_head.mem_ptr);
    printf("uio_baseddr_original.mem_ptr=%x\n", uio_baseddr_original.mem_ptr);
    printf("uio_baseddr_iq.mem_ptr=%x\n", uio_baseddr_iq.mem_ptr);
    printf("uio_baseddr_sensor.mem_ptr=%x\n", uio_baseddr_sensor.mem_ptr);

    printf("uio_baseddr_head_1.mem_ptr=%x\n", uio_baseddr_head_1.mem_ptr);
    printf("uio_baseddr_original_1.mem_ptr=%x\n", uio_baseddr_original_1.mem_ptr);
    printf("uio_baseddr_iq_1.mem_ptr=%x\n", uio_baseddr_iq_1.mem_ptr);
    printf("uio_baseddr_sensor_1.mem_ptr=%x\n", uio_baseddr_sensor_1.mem_ptr);
}
