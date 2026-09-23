#include "beam.h"
#include "network.h"
#include "upper_to_fpga.h"
#include "fpga_init.h"
#include "fpga_to_upper.h"
#include "sensor.h"

void gpio_write(void *gpio_base, unsigned int offset, unsigned int value)
{
    *((volatile unsigned *)(gpio_base + offset)) = value;
}

/*读GPIO，参数分别为基地址，偏移地址，函数返回该偏移地址的内容*/
unsigned int gpio_read(void *gpio_base, unsigned int offset)
{
    return *((volatile unsigned *)(gpio_base + offset));
}

/*打开GPIO*/
int gpio_export(unsigned int gpio)  
{  
    int fd, len;  
    char buf[MAX_BUF];  
    fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY); 
    if (fd < 0) 
    {  
        perror("gpio/export");  
        return fd;  
    }  
    len = snprintf(buf, sizeof(buf), "%d", gpio);  
    write(fd, buf, len);  
    close(fd);  
    return 0;  
} 

/*关闭GPIO*/ 
int gpio_unexport(unsigned int gpio)  
{  
    int fd, len;  
    char buf[MAX_BUF];     
    fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);

    if (fd < 0) 
    {  
        perror("gpio/export");  
        return fd;  
    }  
    len = snprintf(buf, sizeof(buf), "%d", gpio);  
    write(fd, buf, len);  
    close(fd);  
    return 0;  
}  

/*设置GPIO方向，参数gpio:管脚号，out_flag:非0为输出0为输入*/
int gpio_set_dir(unsigned int gpio, unsigned int out_flag)  
{  
    int fd, len;  
    char buf[MAX_BUF];     
    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);     
    fd = open(buf, O_WRONLY);
    if (fd < 0) 
    {  
        perror("gpio/direction");  
        return fd;  
    }  
    if (out_flag)  
        write(fd, "out", 4);  
    else  
        write(fd, "in", 3);  
    close(fd);  
    return 0;  
}  

/*设置gpio，参数gpio为管脚号，value:0置低1置高 */
int gpio_set_value(unsigned int gpio, unsigned int value)  
{  
    int fd, len;  
    char buf[MAX_BUF];  
    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);  
    fd = open(buf, O_WRONLY);

    if (fd < 0) 
    {  
        perror("gpio/set-value");  
        return fd;  
    }  
    if (value)  
        write(fd, "1", 2);  
    else  
        write(fd, "0", 2);     
    close(fd);  
    return 0;  
}  

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
    fscanf(size_fp, "0x%08X", &size);
    fclose(size_fp);
    return size;
}

void uio_init(UIO_CONFIG_PARAMETER *uio_parameter)
{
    uio_parameter->fd = open(uio_parameter->uiod, O_RDWR);
    if (uio_parameter->fd < 0) 
    {
        fprintf(stderr, "   open %s failed: %s\n",
                uio_parameter->uiod, strerror(errno));
        uio_parameter->mem_ptr = MAP_FAILED;
        uio_parameter->mem_size = 0;
        return;
    }
    uio_parameter->mem_size = get_memory_size(uio_parameter->sysfs_path_file);
    uio_parameter->mem_ptr = mmap(NULL, uio_parameter->mem_size,
                                   PROT_READ | PROT_WRITE, MAP_SHARED,
                                   uio_parameter->fd, 0);
    if (uio_parameter->mem_ptr == MAP_FAILED) 
    {
        fprintf(stderr, "   mmap %s failed (size=0x%x): %s\n",
                uio_parameter->uiod, uio_parameter->mem_size,
                strerror(errno));
        uio_parameter->mem_size = 0;
    }
    else
    {
        printf("    %s: mem_ptr=%p, mem_size=0x%x\n",
               uio_parameter->uiod, uio_parameter->mem_ptr,
               uio_parameter->mem_size);
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
    DBG("INIT IQ_A_DDR:\n");
    uio_init(&uio_iq_mem_a);
    DBG("INIT IQ_B_DDR:\n");
    uio_init(&uio_iq_mem_b);
    DBG("INIT DATA_HEAD_DDR_A:\n");
    uio_init(&data_head_mem_a); 
    DBG("INIT DATA_HEAD_DDR_B:\n");
    uio_init(&data_head_mem_b); 
    DBG("INIT SENSOR_0_DDR:\n");
    uio_init(&sensor_mem_0); 
    DBG("INIT SENSOR_8_DDR:\n");
    uio_init(&sensor_mem_1);
     

    DBG("INIT SENSOR_A_DDR:\n");
    uio_sensor_mem_1.mem_ptr = (int *)((char *)(sensor_mem_0.mem_ptr) + 0x100000); 
    Debug(" uio_sensor_mem_1.mem_ptr : %p\n", uio_sensor_mem_1.mem_ptr);
    uio_sensor_mem_2.mem_ptr = (int *)((char *)(sensor_mem_0.mem_ptr) + 0x200000);
    Debug(" uio_sensor_mem_2.mem_ptr : %p\n", uio_sensor_mem_2.mem_ptr); 
    uio_sensor_mem_3.mem_ptr = (int *)((char *)(sensor_mem_0.mem_ptr) + 0x300000);
    Debug(" uio_sensor_mem_3.mem_ptr : %p\n", uio_sensor_mem_3.mem_ptr);
    uio_sensor_mem_4.mem_ptr = (int *)((char *)(sensor_mem_0.mem_ptr) + 0x400000);
    Debug(" uio_sensor_mem_4.mem_ptr : %p\n", uio_sensor_mem_4.mem_ptr);
    DBG("INIT SENSOR_B_DDR:\n");
    uio_sensor_mem_9.mem_ptr = (int *)((char *)(sensor_mem_1.mem_ptr) + 0x100000); 
    Debug(" uio_sensor_mem_9.mem_ptr : %p\n", uio_sensor_mem_9.mem_ptr);
    uio_sensor_mem_A.mem_ptr = (int *)((char *)(sensor_mem_1.mem_ptr) + 0x200000);
    Debug(" uio_sensor_mem_A.mem_ptr : %p\n", uio_sensor_mem_A.mem_ptr);
    uio_sensor_mem_B.mem_ptr = (int *)((char *)(sensor_mem_1.mem_ptr) + 0x300000);
    Debug(" uio_sensor_mem_B.mem_ptr : %p\n", uio_sensor_mem_B.mem_ptr);
    uio_sensor_mem_C.mem_ptr = (int *)((char *)(sensor_mem_1.mem_ptr) + 0x400000); 
    Debug(" uio_sensor_mem_C.mem_ptr : %p\n", uio_sensor_mem_C.mem_ptr);

    DBG("INIT ORIGINAL_DDR:\n");
    uio_init(&original_mem); 
    DBG("INIT TVG_REGISTER_DDR:\n");
    uio_init(&tvg_register_mem); 
    DBG("INIT FPGA_REGISTER_DDR:\n");
    uio_init(&fpga_register_mem); 

    char *uiod = "/dev/uio9";		    
    fd_uio6 = open(uiod, O_RDWR);
    if (fd_uio6 < 1) 
    {
        printf("Invalid UIO device file:%s.\n", uiod);
    }       
    if (fpga_register_mem.mem_ptr == MAP_FAILED ||
        fpga_register_mem.mem_size < sizeof(FPGA_REGISTERS))
    {
        fprintf(stderr,
                "FPGA register UIO mapping is invalid: ptr=%p size=0x%x need=0x%zx\n",
                fpga_register_mem.mem_ptr, fpga_register_mem.mem_size,
                sizeof(FPGA_REGISTERS));
        ptr_fpga_register_data = NULL;
        return;
    }

    ptr_fpga_register_data = (FPGA_REGISTERS *)fpga_register_mem.mem_ptr;
}

/********************************************************************************
 * 名称：                  init_EPLD_MIO32_bit_low
 * 功能：                  初始化EPLD管脚MIO32为低电平
 * 入口参数：               无 
 * 出口参数：               无
 *********************************************************************************/
void init_EPLD_MIO32_bit_low(void)
{
    gpio_export(938);  //打开端口938
    gpio_set_dir(938, 1); //端口938为输出模式
    gpio_set_value(938, 0); //低电平
    usleep(10);
}

/********************************************************************************
 * 名称：                  init_EPLD_MIO33_bit_hign
 * 功能：                  初始化EPLD管脚MIO33为高电平
 * 入口参数：               无 
 * 出口参数：               无
 *********************************************************************************/
void init_EPLD_MIO33_bit_hign(void)
{
    gpio_export(939);  //打开端口938
    gpio_set_dir(939, 1); //端口938为输出模式
    gpio_set_value(939, 1); //高电平
    usleep(10);
}

/********************************************************************************
 * 名称：                  init_EPLD_MIO28_bit_hign
 * 功能：                  初始化EPLD管脚MIO28为输出高电平
 * 入口参数：               无 
 * 出口参数：               无
 *********************************************************************************/
void init_EPLD_MIO28_bit_hign(void)
{
    gpio_export(934);  //打开端口938
    gpio_set_dir(934, 1); //端口938为输出模式
    gpio_set_value(934, 1); //高电平
    usleep(10);
}
