

#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"

int fd_uio2; // 中断的驱动设备名称;

static void Read_Fpga_Register_Data(void);
static void print_uio_baseddr(void);

/**************************************UIO结构体**************************************/
UIO_CONFIG_PARAMETER uio_fpga_register =            {.uiod = "/dev/uio0",   .sysfs_path_file = "/sys/class/uio/uio0/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_tvg_register =             {.uiod = "/dev/uio1",   .sysfs_path_file = "/sys/class/uio/uio1/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_head =             {.uiod = "/dev/uio3",   .sysfs_path_file = "/sys/class/uio/uio3/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_head_1 =           {.uiod = "/dev/uio4",   .sysfs_path_file = "/sys/class/uio/uio4/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_ad =               {.uiod = "/dev/uio5",   .sysfs_path_file = "/sys/class/uio/uio5/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_ad_1 =             {.uiod = "/dev/uio6",   .sysfs_path_file = "/sys/class/uio/uio6/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_iq =               {.uiod = "/dev/uio7",   .sysfs_path_file = "/sys/class/uio/uio7/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_iq_1 =             {.uiod = "/dev/uio8",   .sysfs_path_file = "/sys/class/uio/uio8/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_sensor =           {.uiod = "/dev/uio9",   .sysfs_path_file = "/sys/class/uio/uio9/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_sensor_1 =         {.uiod = "/dev/uio10",  .sysfs_path_file = "/sys/class/uio/uio10/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_pashr =            {.uiod = "/dev/uio11",  .sysfs_path_file = "/sys/class/uio/uio11/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_pashr_1 =          {.uiod = "/dev/uio12",  .sysfs_path_file = "/sys/class/uio/uio12/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_svs =              {.uiod = "/dev/uio13",  .sysfs_path_file = "/sys/class/uio/uio13/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_baseddr_svs_1 =            {.uiod = "/dev/uio14",  .sysfs_path_file = "/sys/class/uio/uio14/maps/map0/size"};
UIO_CONFIG_PARAMETER uio_mems_register =            {.uiod = "/dev/uio15",  .sysfs_path_file = "/sys/class/uio/uio15/maps/map0/size"};
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
    uio_init(&uio_fpga_register);  
    uio_init(&uio_tvg_register);  
    uio_init(&uio_baseddr_head);  
    uio_init(&uio_baseddr_head_1);  
    uio_init(&uio_baseddr_ad);  
    uio_init(&uio_baseddr_ad_1);  
    uio_init(&uio_baseddr_iq);  
    uio_init(&uio_baseddr_iq_1);  
    uio_init(&uio_baseddr_sensor);  
    uio_init(&uio_baseddr_sensor_1);  
    uio_init(&uio_baseddr_pashr);  
    uio_init(&uio_baseddr_pashr_1);  
    uio_init(&uio_baseddr_svs);  
    uio_init(&uio_baseddr_svs_1);  
    uio_init(&uio_mems_register);  

    pri_fpga_uio_init();

    char *uiod = "/dev/uio2"; 
    fd_uio2 = open(uiod, O_RDWR);
    if (fd_uio2 < 1)
    {
        printf("Invalid UIO device file:%s.\n", uiod);
    }
    
    // 初始化的fpga参数寄存器的地址给ptr_fpga_register_data
    ptr_fpga_register_data = (FPGA_REGISTERS *)(uio_fpga_register.mem_ptr); 
    ptr_fpga_version_data = (FPGA_REGISTERS *)((int *)(char *)(uio_fpga_register.mem_ptr) + 0x100);
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

void pri_fpga_uio_init(void){
    printf("%s:init_uio:%s - %p - 0x%x \n",__func__,uio_fpga_register.uiod ,uio_fpga_register.mem_ptr,uio_fpga_register.mem_size);
    printf("%s:init_uio:%s - %p - 0x%x \n",__func__,uio_tvg_register.uiod ,uio_tvg_register.mem_ptr,uio_tvg_register.mem_size);
    printf("%s:init_uio:%s - %p(0x30000000) - 0x%x \n",__func__,uio_baseddr_head.uiod ,uio_baseddr_head.mem_ptr,uio_baseddr_head.mem_size); 
    printf("%s:init_uio:%s - %p(0x30010000) - 0x%x \n",__func__,uio_baseddr_head_1.uiod ,uio_baseddr_head_1.mem_ptr,uio_baseddr_head_1.mem_size);
    printf("%s:init_uio:%s - %p(0x31000000) - 0x%x \n",__func__,uio_baseddr_ad.uiod ,uio_baseddr_ad.mem_ptr,uio_baseddr_ad.mem_size);
    printf("%s:init_uio:%s - %p(0x32000000) - 0x%x \n",__func__,uio_baseddr_ad_1.uiod ,uio_baseddr_ad_1.mem_ptr,uio_baseddr_ad_1.mem_size);
    printf("%s:init_uio:%s - %p(0x33000000) - 0x%x \n",__func__,uio_baseddr_iq.uiod ,uio_baseddr_iq.mem_ptr,uio_baseddr_iq.mem_size);
    printf("%s:init_uio:%s - %p(0x34000000) - 0x%x \n",__func__,uio_baseddr_iq_1.uiod ,uio_baseddr_iq_1.mem_ptr,uio_baseddr_iq_1.mem_size);
    printf("%s:init_uio:%s - %p(0x35000000) - 0x%x \n",__func__,uio_baseddr_sensor.uiod ,uio_baseddr_sensor.mem_ptr,uio_baseddr_sensor.mem_size);
    printf("%s:init_uio:%s - %p(0x36000000) - 0x%x \n",__func__,uio_baseddr_sensor_1.uiod ,uio_baseddr_sensor_1.mem_ptr,uio_baseddr_sensor_1.mem_size);
    printf("%s:init_uio:%s - %p(0x37000000) - 0x%x \n",__func__,uio_baseddr_pashr.uiod ,uio_baseddr_pashr.mem_ptr,uio_baseddr_pashr.mem_size);
    printf("%s:init_uio:%s - %p(0x38000000) - 0x%x \n",__func__,uio_baseddr_pashr_1.uiod ,uio_baseddr_pashr_1.mem_ptr,uio_baseddr_pashr_1.mem_size);
    printf("%s:init_uio:%s - %p(0x39000000) - 0x%x \n",__func__,uio_baseddr_svs.uiod ,uio_baseddr_svs.mem_ptr,uio_baseddr_svs.mem_size);
    printf("%s:init_uio:%s - %p(0x3a000000) - 0x%x \n",__func__,uio_baseddr_svs_1.uiod ,uio_baseddr_svs_1.mem_ptr,uio_baseddr_svs_1.mem_size);
    printf("%s:init_uio:%s - %p - 0x%x \n",__func__,uio_mems_register.uiod ,uio_mems_register.mem_ptr,uio_mems_register.mem_size);
}