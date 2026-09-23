#include  <fcntl.h>
#include "main.h"
#include "tempsensor.h"
#include "drytoupper.h" 

typedef unsigned char uint8;


int     fd_icc = 0;
float   dry_temp = 0.0f;
float   temprature1 = 0;
int     tem_num = 1;//当温度不变化时进行累加，累加的值
uint8   i2c_read_reg = 0;
uint8   local_high_value = 0, local_low_value = 0, remote_high_value = 0, remote_low_value = 0; //读出温度的整数位
uint8   status_register = 0;

/********************************************************************************
 * * 名称：                    NST175_Init
 * * 功能：                    NST175温度传感器初始化
 * * 入口参数：                     无
 * * 出口参数：                     uint8
 * *********************************************************************************/
uint8 NST175_Init(void)
{
    fd_icc = open("/dev/i2c-0", O_RDWR);   //允许读写 
    if (fd_icc < 0)
    {
        perror("Can't open /dev/i2c-0\n"); //打开iic设备文件失败
        exit(1);
    }
    printf("open /dev/i2c success !\n");   //打开iic设备文件成功    
    if (ioctl(fd_icc, I2C_SLAVE, IFB_ADDR) < 0)      //设置iic从器件地址
    {
        printf("fail to set i2c device slave address!\n");
        close(fd_icc);
        return -1;
    }
    printf("set slave address to 0x%x success!\n", IFB_ADDR);
    return(1);
}

/********************************************************************************
 * * 名称：                    TMP451_Init
 * * 功能：                    TMP451温度传感器初始化
 * * 入口参数：                     无
 * * 出口参数：                     uint8
 * *********************************************************************************/
uint8 TMP451_Init(void)
{
    fd_icc = open("/dev/i2c-0", O_RDWR);   //允许读写 
    if (fd_icc < 0)
    {
        perror("Can't open /dev/i2c-0\n"); //打开iic设备文件失败
        exit(1);
    }
    printf("open /dev/i2c success !\n");   //打开iic设备文件成功    
    if (ioctl(fd_icc, I2C_SLAVE, Address) < 0)      //设置iic从器件地址
    {
        printf("fail to set i2c device slave address!\n");
        close(fd_icc);
        return 0;
    }
    printf("set slave address to 0x%x success!\n", Address);
    return (1);
}
/********************************************************************************
 * * 名称：                    i2c_read
 * * 功能：                    icc读取一字节
 * * 入口参数：                     无
 * * 出口参数：                     uint8
 * *********************************************************************************/
//static uint8 i2c_read(int fd, uint8 reg, uint8* val)
uint8 i2c_read(int fd, uint8 reg, uint8* val)
{
    int retries;
    for (retries = 5; retries; retries--){
        if (write(fd, &reg, 1) == 1){
            if (read(fd, val, 1) == 1){
                return 0;
            }
        }
    }
    return -1;
}
uint8 i2c_read2(int fd, uint8 reg, uint8* val)
{
    int retries;
    for (retries = 5; retries; retries--){
        if (write(fd, &reg, 1) == 1){
            if (read(fd, val, 2) == 1){
                return 0;
            }
        }
    }
    return -1;
}
/********************************************************************************
 * * 名称：                    i2c_write
 * * 功能：                    icc写入一字节
 * * 入口参数：                     无
 * * 出口参数：                     uint8
 * *********************************************************************************/
//static uint8 i2c_write(int fd, uint8 reg, uint8 val)
uint8 i2c_write(int fd, uint8 reg, uint8 val)
{
    int retries;
    unsigned char data[2];
    data[0] = reg;
    data[1] = val;
    for (retries = 5; retries; retries--)
    {
        if (write(fd, data, 2) == 2)
            return 0;
        usleep(1000 * 10);
    }
    return -1;
}

/********************************************************************************
 * * 名称：                    get_sensor_data
 * * 功能：                    得到TMP451的值，罗经的值
 * * 入口参数：                     无
 * * 出口参数：                     无
 * *********************************************************************************/
void get_sensor_data(void)
{
    short temp = 0;
    uint8 xtemp = 0;
    uint8 utemp[2];
    i2c_read2(fd_icc, REG_TEMP, utemp);
//    i2c_read(fd_icc, REG_TEMP, &remote_low_value); //读取远程温度值-低位
//    i2c_write(fd_icc, REG_TEMP, 1);
//    i2c_read(fd_icc, REG_TEMP, 1);
    xtemp = utemp[0];
    utemp[0] = utemp[1];
    utemp[1] = xtemp;
    memcpy(&temp, &utemp, 2);
    dry_temp = (float)(temp/16)*0.0625;
    // DBG("dry_temp = %f\n",dry_temp );
    // DBG("utemp0 = %d\n",utemp[0] );
    // DBG("utemp1 = %d\n",utemp[1] );
//    dry_temp = (float)((remote_high_value - 64) + (remote_low_value >> 7) * 0.5 + pow(((remote_low_value >> 6) & 0x01) * 0.5, 2) + pow(((remote_low_value >> 5) & 0x01) * 0.5, 3) + pow(((remote_low_value >> 4) & 0x01) * 0.5, 4)); //计算温度值 
  
/*  dry_temp = (float)((remote_low_value >> 7) * 0.5 + pow(((remote_low_value >> 6) & 0x01) * 0.5, 2) + pow(((remote_low_value >> 5) & 0x01) * 0.5, 3) + pow(((remote_low_value >> 4) & 0x01) * 0.5, 4)); //计算温度值 
	DBG("dry_temp = %f\n",dry_temp );
	DBG("remote_high_value :%f  remote_low_value:%f\n",(float)remote_high_value,(float)remote_low_value);
    i2c_read(fd_icc, 0x03,&remote_low_value); //读取远程温度值-低位
	DBG("remote_low_value:%#x\n",remote_low_value);*/
    usleep(8000);
}

/********************************************************************************
 * * 名称：                    read_TMP451_sensor_and_save
 * * 功能：                    读取TMP451的值到指定的位置
 * * 入口参数：                     无
 * * 出口参数：                     无
 * *********************************************************************************/
void read_TMP451_sensor_and_save(void)
{
    //初始化温度传感器的IIC接口
    TMP451_Init();
    i2c_write(fd_icc, REG_CONFIG_WRITE, 0x04);//开启扩展测温，温度范围是-64度到191度
    setTimer(0, 500000);//500ms
    while (1)
    {
        if (i2c_read_reg != 0)
        {
            TMP451_Init();
            i2c_read_reg = 0;
        }
        else if ((flag_timer == 1) && (Fpga_start_mod == 1))
        {
            flag_timer = 0;
            setTimer(0, 500000);//500ms 
            i2c_read_reg = i2c_read(fd_icc, REG_REMOTE_HIGH, &remote_high_value);//读取远程温度值-高位
            get_sensor_data(); //从FPGA读取数据,需要知道GGA传感器协议。      
        }
        else
        {
            usleep(1000);
        }
		sleep(3);
    }
}
