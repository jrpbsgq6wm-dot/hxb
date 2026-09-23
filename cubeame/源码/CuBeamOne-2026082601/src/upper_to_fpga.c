
#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"

/**************************************socket相关-全局变量**************************************/
volatile int netStatus; // socket的连接状态标志   ;放在判断连接状态的文件中
volatile int net_end_flag;

/*******************************全局变量***************************************/
int bram[8192];
// extern int flag;
int mems_UpData_len;
int timer_flag = 0;
int mems_data_cnt = 0;
int Fpga_start_mod = 0;     // FPGA开始工作标志;
char DataTail_S1[4] = {'E','D','>','>'};
static char RT[4] = {'#', '#', 'S', 'U'};
static char ZYNQ_VERSIONS[4] = {'V', '0', '0', '0'}; // zynq版本号

int fd_eeprom;

volatile int start_sdtu_flag = 0;
volatile int cmd_updata_flag = 0;

#define EEPROM_SEEK_TM 0
#define EEPROM_SEEK_VL 8
#define EEPROM_SEEK_VP 128
#define EEPROM_WR_SIZE_TM 8
#define EEPROM_WR_SIZE_VL 120
#define EEPROM_WR_SIZE_VP 120

MEMS_UP_DATA mems_up;

RECV_UPPER_CONFIG_PARAMETERS recv_upper_package;                            // 接收到上位机的参数配置包----结构体变量
RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package = &recv_upper_package; // 接收到上位机的参数配置包----结构体指针

RECV_UPPER_CONFIG_PARAMETERS cmd_package; // 接收到上位机的参数配置包保存本地----结构体变量
RECV_UPPER_CONFIG_PARAMETERS cmd_package_backup; // 接收到上位机的参数配置包保存本地----结构体变量

SEND_UPPER_SONAR_STATUS send_upper_status_information;                                       // 发送给上位机状态信息----结构体变量
SEND_UPPER_SONAR_STATUS *ptr_send_upper_status_information = &send_upper_status_information; // 发送给上位机状态信息----结构体指针

/*******************************函数声明***************************************/
static int svp_set(void);
static int mems_set(void);
static int send_svp_data(void);
static int mems_update(void);
static void mems_Reboot(void);
static int zynq_update(void);
// int crc16(char *buff, int len);
void clear_sensor_data(void);
static int send_fpga_config(void);
static void send_status_to_upper(void);
static unsigned int recv_socket(int fd, char *buf, unsigned int len);
static unsigned short crc16(void *data, unsigned int len, unsigned short crc);
static void Debug_pritf_receive_para(RECV_UPPER_CONFIG_PARAMETERS *receive_upper_package);
static void Debug_pritf_convert_fpga_parameter(FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para);
static void copy_recv_cmd(RECV_UPPER_CONFIG_PARAMETERS *ptr_cmd_des, const RECV_UPPER_CONFIG_PARAMETERS *ptr_cmd_source);
static int parsing_instructions_200k(const RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package, FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para);
/*******************************mems_update函数声明***************************************/
int mems_crc16(char *buff, int crc16_len); 
void mems_EndData(char *end_buf, int end_len);
void mems_BeginData(char *head_buf, int head_len);
void mems_FileData(char *data_buf, int send_cnt, int mems_data_len, int read_size);

int test(FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para);
/********************************************************************************
 * 名称：                    Recv_Upper_SendTo_Fpga
 * 功能：                    上位机--->ARM--->FPGA主线程
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void Recv_Upper_SendTo_Fpga(void)
{
#if 0
    ptr_fpga_register_data->wsm_con = 1;
    test(&(fpga_register_data.fpga_registers_200k));
    ptr_fpga_register_data->fpga_registers_200k = fpga_register_data.fpga_registers_200k;
    //memcpy((char *)uio_tvg_register.mem_ptr, 0, fpga_register_data.fpga_registers_200k.dac_registers_value.dac_sn * 4);
    ptr_fpga_register_data->set_pr = 1;
    usleep(1000); // 1ms
    ptr_fpga_register_data->set_pr = 0;
    while(1);
#endif

#if 1
    char DataHead1[2];
    char DataHead2[2];
    char DataHead3[5];
    
    int len_recv_head1, len_recv_head2;

    int a;
    for (a = 0; a < 1024 * 8; ++a)
    {
        bram[a] = 248;// bram[a]=248;TVG偏置1v
    }

    while (1)
    {
	    DBG("Recv Upper SendTo Fpga Ok!!!\n");
        if (netStatus == 1) // 判断网络标志位是否为1连通状态
        {
            DataHead1[0] = 0;
            DataHead1[1] = 0;
            while ((DataHead1[0] != '<') || (DataHead1[1] != '<')) // 判断数据头是否为<<
            {
                len_recv_head1 = recv(Connect_fd, DataHead1, 2, 0); // 接收缓冲区前两个字节数，成功返回接收字节数
                if(DataHead1[0] == '@' || DataHead1[1] == '@')
                    goto mems;
                if (len_recv_head1 <= 0)
                {
                    error_process(); // 网络异常处理，主要清除传感器数据，断开与fpga数据的链接
                    break;
                }
            }
        mems:
            len_recv_head2 = recv(Connect_fd, DataHead2, 2, 0); // 接受下两个字节
            if (len_recv_head2 < 0)
            {
                error_process();
                continue;
            }

            if ((DataHead2[0] == 'R') && (DataHead2[1] == 'S')) // 1.request status，请求硬件版本信息----改完
            {
                DBG("<<RS\n");
                pthread_mutex_lock(&mut);
                send_status_to_upper();
                pthread_mutex_unlock(&mut);
            }

            if ((DataHead2[0] == 'U') && (DataHead2[1] == 'P')) // 2.固件升级
            {
                DBG("<<UP\n");
                ptr_fpga_register_data->wsm_con = 0;
                Fpga_start_mod = 0;
                clear_sensor_data(); // 清除传感器数据
                usleep(5000);

                len_recv_head1 = recv(Connect_fd, DataHead3, 5, 0);
                DBG("DataHead3 = %s\n",DataHead3);
                DBG("firmware update begins!!!!\n");
                if (len_recv_head1 <= 0)
                {
                    error_process();
                    break;
                }
                if ((DataHead3[0] == '_') && (DataHead3[1] == 'Z') && (DataHead3[2] == 'Y') && (DataHead3[3] == 'N') && (DataHead3[4] == 'Q')) // ZYNQ更新
                {
                    zynq_update();
                    continue;
                }
                if ((DataHead3[0] == '_') && (DataHead3[1] == 'M') && (DataHead3[2] == 'E') && (DataHead3[3] == 'M') && (DataHead3[4] == 'S'))  //MEMS更新
                {
                    pthread_mutex_lock(&mut);
                    mems_update();
                    continue;
                }
            }

            if ((DataHead2[0] == 'B') && (DataHead2[1] == 'R')) // 3.Begin Request Beam Data，开始请求数据 (回复)----直接把fpga工作控制寄存器置一（直接操作fpga硬件地址）
            {
                printf("<<BR\n");
                pthread_mutex_unlock(&mut);
                send_svp_data();
                ptr_fpga_register_data->wsm_con = 1;
                Fpga_start_mod = 1;
                pthread_mutex_unlock(&mut);
            }

            if ((DataHead2[0] == 'E') && (DataHead2[1] == 'R')) // 4.End RequestBeam Data停止请求波形数据
            {
                printf("<<ER\n");
                pthread_mutex_lock(&mut);
                // flag = 0;
                ptr_fpga_register_data->wsm_con = 0;
                Fpga_start_mod = 0;
                clear_sensor_data();
                pthread_mutex_unlock(&mut);
            }

            if ((DataHead2[0] == 'S') && (DataHead2[1] == 'P')) // 5.set parameter，设置声呐的各项参数  (接收)
            {
                printf("<<SP\n");
                pthread_mutex_lock(&mut);                
                send_fpga_config();
                pthread_mutex_unlock(&mut);
            }
        
            if ((DataHead2[0] == 'S') && (DataHead2[1] == 'C')) // 6.配置内置mems
            {
                if(0 == mems_set())
                    continue;
                break;
            }
        
            if ((DataHead2[0] == 'V') && (DataHead2[1] == 'P')) // 7.声速、传感器温度标定
            {
                if(0 == svp_set())
                    continue;
                break;
            }
            if ((DataHead2[0] == 'H') && (DataHead2[1] == 'E')) // 8. (Hardware Version)读取eeprom中的厂商信息
            {
                if(0 == eeprom_map_flag){
                    eeprom_file_mapping();
                }
                if(0 == eeprom_option())
                    continue;
                break;
            } 
        }
        else
        {
            if ((Connect_fd = accept(Socket_fd_server, (struct sockaddr *)NULL, NULL)) == -1)
            {
                printf("accept socket error: %s(errno: %d)", strerror(errno), errno);
                continue;
            } // 阻塞
            netStatus = 1;
            DBG("======connect complete!======\n");
        }
    }
#endif
}

/************************************下发fpga配置********************************************/
void cmdBackup_to_fpga(void){
    parsing_instructions_200k(&cmd_package_backup, &(fpga_register_data.fpga_registers_200k));// 解析fpga寄存器
    ptr_fpga_register_data->fpga_registers_200k = fpga_register_data.fpga_registers_200k;                                                          // 下发FPGA参数配置信息
    memcpy((char *)uio_tvg_register.mem_ptr, (char *)&bram, SIZE_OF_TVG_MAX);             
    memcpy((char *)uio_tvg_register.mem_ptr, (char *)&cmd_package.tvgGain, fpga_register_data.fpga_registers_200k.dac_registers_value.dac_sn * 4); // 下发tvg数据
    ptr_fpga_register_data->set_pr = 1;
    usleep(1000); // 1ms
    ptr_fpga_register_data->set_pr = 0;
}


int send_fpga_config(void){
    int  len_recv;
    char DataTail[5]; // 数据尾标识“ED>>”
    int  len_recv_crc;
    unsigned short recv_crc;
    unsigned short recv_crc_calculate;

    len_recv = recv_socket(Connect_fd, (char *)(ptr_recv_upper_package), SIZE_OF_CONFIG_PARA); // 实际接收3264byte，可以考虑把校验和尾一起接收到一起
    //DBG("len_recv: %d\n", len_recv);
    if (len_recv < 0)
    {
        error_process();
        return 1;
    }
    len_recv_crc = recv_socket(Connect_fd, (char *)&recv_crc, SIZE_OF_LONG); // 接收完参数配置信息3268byte后接受4给字节的CRC校验码
    //DBG("len_recv_crc: %d\n",len_recv_crc);
    if (len_recv_crc < 0)
    {
        error_process();
        return 1;
    }
    recv_socket(Connect_fd, DataTail, SIZE_OF_LONG); // 接受尾“ED>>”

    if (strncmp(DataTail, DataTail_S1, 4) == 0)
    {
        DBG("DataTail:%c %c %c %c\n", DataTail[0], DataTail[1], DataTail[2], DataTail[3]);
    }
    else
    {
        DBG("DataTail error\n");
        return 1;
    }
    recv_crc_calculate = crc16((char *)(ptr_recv_upper_package), SIZE_OF_CONFIG_CRC16, 0xFFFF);
    if (recv_crc_calculate != recv_crc)
    {
        printf("recv_crc_calculate:%x \n", recv_crc_calculate);
        printf("recv_crc:%x \n", recv_crc);
        printf("crc error! \n");
        return 1;
    }

    if(start_sdtu_flag){
        //暂存显控设置参数 等待发送线程空闲自己执行；
        copy_recv_cmd(&cmd_package_backup, ptr_recv_upper_package);
        cmd_updata_flag = 1;
        return;
    }else{
        cmd_updata_flag = 0;
    }

    copy_recv_cmd(&cmd_package, ptr_recv_upper_package);                                    // 拷贝到本地
    // Debug_pritf_receive_para(&cmd_package);                                                 // 打印上位机下发到arm的配置信息

    parsing_instructions_200k(&cmd_package, &(fpga_register_data.fpga_registers_200k));// 解析fpga寄存器

    // Debug_pritf_convert_fpga_parameter(&(fpga_register_data.fpga_registers_200k));                                                                 // 打印

    //pthread_mutex_lock(&mut);
    ptr_fpga_register_data->fpga_registers_200k = fpga_register_data.fpga_registers_200k;                                                          // 下发FPGA参数配置信息
    memcpy((char *)uio_tvg_register.mem_ptr, (char *)&bram, SIZE_OF_TVG_MAX);  
    DBG("TVG = %d\n",cmd_package.tvgGain[0]);                    
    memcpy((char *)uio_tvg_register.mem_ptr, (char *)&cmd_package.tvgGain, fpga_register_data.fpga_registers_200k.dac_registers_value.dac_sn * 4); // 下发tvg数据
    // memset(uio_tvg_register.mem_ptr + fpga_register_data.fpga_registers_200k.dac_registers_value.dac_sn, 0, (1000/ cmd_package.PingRate - fpga_register_data.fpga_registers_200k.dac_registers_value.dac_sn)*4);//剩余tvg置0
    //pthread_mutex_unlock(&mut);

    DBG("TVG send to fpga ok!\n");
    ptr_fpga_register_data->set_pr = 1;
    usleep(1000); // 1ms
    ptr_fpga_register_data->set_pr = 0;
    return 0;
}
/************************************清空传感器数据********************************************/
void clear_sensor_data(void)
{
    memset(uio_baseddr_sensor.mem_ptr,0,uio_baseddr_sensor.mem_size);
    memset(uio_baseddr_sensor_1.mem_ptr,0,uio_baseddr_sensor_1.mem_size);
    memset(uio_baseddr_head.mem_ptr, 0, 20000);
    memset(uio_baseddr_head_1.mem_ptr, 0, 20000);
}
/************************************接收上位机命令********************************************/

/*****************************************************************
 * 名称：                    crc16
 * 功能：                    计算两个字节的CRC
 * 入口参数：            	 *data：数组的起始地址  len：数组的长度  crc：crc的初值
 * 出口参数：            	 两字节的crc
 *****************************************************************/
unsigned short crc16(void *data, unsigned int len, unsigned short crc)
{
    unsigned char *pBuff = (unsigned char *)data;
    unsigned int i;
    for (i = 0; i < len; i++)
    {
        crc = crc ^ (unsigned short)pBuff[i];
        crc = (crc >> 8) ^ crc16_tab[crc & 0x0FF];
    }
    return crc;
}

/********************************************************************************
 * 名称：                    send_status_to_upper
 * 功能：                    ARM向上位机发送硬件状态信息
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void send_status_to_upper(void)
{
    char *ptr_to_send;
    char DataHead[4] = {'<', '<', 'S', 'S'}; // 数据头标示“<<SS”,表示数据头
    DBG("<<SS\n");

    ptr_send_upper_status_information->FPGAVersion = (unsigned int)ptr_fpga_version_data->fpga_sta.date; // FPGA版本信息,版本时间
    ptr_send_upper_status_information->LinuxVersion = LINUX_VERSION;                                // ARM版本号信息---后续要改成从eeprom读取

    ptr_to_send = (char *)ptr_send_upper_status_information;
    if (send(Connect_fd, DataHead, SIZE_OF_LONG, 0) < 0) // 4byte
    {
        error_process();
        return;
    }
    if (send(Connect_fd, ptr_to_send, SIZE_OF_STATUS_SEND, 0) < 0) // 8byte，实际结构体8byte
    {
        error_process();
        return;
    }
    if (send(Connect_fd, DataTail_S1, SIZE_OF_LONG, 0) < 0) // 4byte
    {
        error_process();
        return;
    }
    DBG("ok\n");
}

/********************************************************************************
 * 名称：                    copy_recv_cmd
 * 功能：                    将接收的上位机下发的数据保存本地
 * 入口参数：            	 *ptr_cmd_des ---保存到ARM本地的参数 *ptr_cmd_source----上位机下发的参数
 * 出口参数：            	 无
 *********************************************************************************/
void copy_recv_cmd(RECV_UPPER_CONFIG_PARAMETERS *ptr_cmd_des, const RECV_UPPER_CONFIG_PARAMETERS *ptr_cmd_source)
{
    memcpy((char *)(&ptr_cmd_des->TotalBytes), (char *)(&(ptr_cmd_source->TotalBytes)), SIZE_OF_CONFIG_PARA); // 首地址拷贝
}

/********************************************************************************
 * 名称：                    recv_socket
 * 功能：                    socket接收函数（接收socket长度很大时用此函数能保证接受全）
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
unsigned int recv_socket(int fd, char *buf, unsigned int len)
{
    unsigned int lenth_recv = len;
    unsigned int lenth_temp = 0;
    char *buftem = buf;
    unsigned int num = 0;
    // printf("len=%d\n", len);
    do
    {
        lenth_temp = recv(fd, buftem, lenth_recv, 0);
        if (lenth_temp <= 0)
        {
            break;
        }
        lenth_recv = lenth_recv - lenth_temp; // 实际是3264byte---3264-实际接受数据=剩余接收大小
        buftem = buftem + lenth_temp;         // 结构体首地址+实际接收的大小=buftem
        num = num + lenth_temp;               // 最后给num
    } while (lenth_recv > 0);
    //printf("num=%d\n", num); // num==3264byte
    return num;
}

/********************************************************************************
 * 名称：         parsing_instructions_200k
 * 功能：         接收到的上位机数据转化为FPGA寄存器值
 * 入口参数：      *ptr_recv_upper_package：接收到的上位机的结构体值 *ptr_fpga_config_para：转化为FPGA的值
 * 出口参数：      正确为0，失败是-1
 *********************************************************************************/
int parsing_instructions_200k(const RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package, FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para)
{
    ptr_fpga_config_para->wsm_registers_value.wsm_mod = (ptr_recv_upper_package->WorkMode) | (ptr_recv_upper_package->LFMMode << 1) | (ptr_recv_upper_package->DataType << 2);
    if (ptr_recv_upper_package->PingRate != 0)
    {
        ptr_fpga_config_para->wsm_registers_value.wsm_ct = (unsigned int)(FPGA_CLK_FREQUENCY / ptr_recv_upper_package->PingRate); // 1/ping_rate*fpga时钟频率=fpga时钟频率/ping_rate
    }
    else
    {
        printf("revc upper PingRate is 0!\n");
        return -1;
    }
    ptr_fpga_config_para->adc_registers_value.adc_sct = (unsigned int)((FPGA_CLK_FREQUENCY / (ptr_recv_upper_package->SamplingRate * 1000)) - 1);              // adc采样周期=(fpga时钟频率/显控下发的采样率)-1
    ptr_fpga_config_para->adc_registers_value.adc_sn = (unsigned int)(((ptr_recv_upper_package->Range * 2 * ptr_recv_upper_package->SamplingRate * 1000) / V_SOUND) - 1); // adc采样次数=量程×2×采样率/1500 -1
    
    ptr_fpga_config_para->dac_registers_value.dac_sct = 99999;
    ptr_fpga_config_para->dac_registers_value.dac_sn = (unsigned int)((ptr_recv_upper_package->Range * 2 * 1000 / V_SOUND) + 1);                                                                                                    //  dac采样次数=TVG采样点数

    ptr_fpga_config_para->pwm_registers_value.pwm_pulse = (unsigned int)(ptr_recv_upper_package->PulseWidth * (FPGA_CLK_FREQUENCY / 1000000.0f)); // 脉宽=显控脉宽×(时钟频率/1000000.0f)应该是为了us转换成s
    // pwm 频率 AD采样率 量程 pwm带宽 ping率 (fpga回传使用)
    ptr_fpga_config_para->pwm_registers_value.pwm_frequency = (unsigned int)(ptr_recv_upper_package->PWMFreq);    // pwm中心频率
    ptr_fpga_config_para->pwm_registers_value.sample_rate = (unsigned int)(ptr_recv_upper_package->SamplingRate); // AD采样率
    ptr_fpga_config_para->pwm_registers_value.range = (unsigned int)(ptr_recv_upper_package->Range);              // 量程
    ptr_fpga_config_para->pwm_registers_value.band_width = (unsigned int)(ptr_recv_upper_package->PWMBandWidth);  // pwm带宽
    ptr_fpga_config_para->pwm_registers_value.ping_rate = (unsigned int)(ptr_recv_upper_package->PingRate);//帧率（ping）
    // pwm开关
    ptr_fpga_config_para->pwm_registers_value.pwm_start = (unsigned int)ptr_recv_upper_package->PwmStart;
    //通道控制字
    // ptr_fpga_config_para->pwm_registers_value.channel_cfg = ptr_recv_upper_package->SourceChannelC;

    ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_iq = ((unsigned int)(ptr_fpga_config_para->adc_registers_value.adc_sn/SAMPLE_FACTOR)-1);
    // ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_iq = ((unsigned int)(ptr_fpga_config_para->adc_registers_value.adc_sn/SAMPLE_FACTOR));
    ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_ad = ((unsigned int)(ptr_fpga_config_para->adc_registers_value.adc_sn/AD_SAMPLE_FACTOR)+1);//改加一

    if (ptr_recv_upper_package->WorkMode == 0) // CW：模式
    {
        ptr_fpga_config_para->pwm_registers_value.pwm_bf = (unsigned int)((double)(ptr_recv_upper_package->PWMFreq * 1000 * N2_32) / FPGA_CLK_FREQUENCY);
        ptr_fpga_config_para->pwm_registers_value.pwm_lfm = 0;
    }
    if (ptr_recv_upper_package->WorkMode == 1) // LFM：模式
    {
        ptr_fpga_config_para->pwm_registers_value.pwm_lfm = (unsigned int)((PWM_LFM_FACTOR / ptr_recv_upper_package->PulseWidth) * ptr_recv_upper_package->PWMBandWidth);
        if (ptr_recv_upper_package->LFMMode == 0)
        {
            ptr_fpga_config_para->pwm_registers_value.pwm_bf = (unsigned int)(N2_32 / (FPGA_CLK_FREQUENCY / 1000) * (ptr_recv_upper_package->PWMFreq - ptr_recv_upper_package->PWMBandWidth / 2) - ptr_fpga_config_para->pwm_registers_value.pwm_lfm / 2 / N2_23);
        }
        else
        {
            ptr_fpga_config_para->pwm_registers_value.pwm_bf = (unsigned int)(N2_32 / (FPGA_CLK_FREQUENCY / 1000) * (ptr_recv_upper_package->PWMFreq + ptr_recv_upper_package->PWMBandWidth / 2) - ptr_fpga_config_para->pwm_registers_value.pwm_lfm / 2 / N2_23);
        }
    }
    return 0;
}

/****************************打开eeprom设备文件  eeprom使用了256字节***********************************/
void open_eeprom()
{
    fd_eeprom = open("/sys/devices/soc0/amba/e0004000.i2c/i2c-0/0-0055/eeprom", O_RDWR);
    printf("fp_eeprom:%d\n",fd_eeprom);
    if (fd_eeprom < 0)
    {
        perror("Can't open /dev/i2c-0\n"); //打开iic设备文件失败
        exit(1);
    }
}
/********************************************************************************
 * 名称：                    vp_write
 * 功能：                    写入eeprom数据
 * 入口参数：            	 1. 读文件位置，2. 读参数大小，3. 发给显控的头字符
 * 出口参数：            	 无
 *********************************************************************************/
void read_eeprom_send(int seek_size,int read_size,char svp_head[4])
{
    //对接收数据进行反码和补码处理，因为温度系数有可能为负数和0.
    int ret;
    int send_size;
    send_size = read_size;
    char Svp_Head[4] = { '#','#','T','M' };

    float tem_floatArray[read_size/sizeof(float)];//获取eeprom中内容转换成float
    char read_eeprom_Array[read_size];//发送给显控的数据,实际传输8个字节

    lseek(fd_eeprom, seek_size, SEEK_SET);
    read(fd_eeprom,read_eeprom_Array,read_size);

    memcpy(tem_floatArray,read_eeprom_Array,read_size);
    if ((ret = send(Connect_fd, svp_head, SIZE_OF_LONG, 0)) < 0) //4byte
    {
        printf("ret :%d\n", ret);
        error_process();
    }
    if ((ret = send(Connect_fd, read_eeprom_Array, send_size, 0)) < 0) //4byte
    {
        printf("ret :%d\n", ret);
        error_process();
    }
#if 0
    int num;
    for(num = 0; num < read_size/sizeof(float); num++)
        printf("read_eeprom_floatArray: %.7f\n", tem_floatArray[num]);
#endif
}
/************************************读取eeprom声速系数上传***************************/
int send_svp_data(void)
{
    // printf("send_svp!!!\n");
    char Svp_Head_TM[4] = { '#','#','T','M' };
    char Svp_Head_VL[4] = { '#','#','V','L' };
    char Svp_Head_VP[4] = { '#','#','V','P' };

    open_eeprom();

    // printf("##TM\n");
    read_eeprom_send(EEPROM_SEEK_TM,EEPROM_WR_SIZE_TM,Svp_Head_TM);
    // printf("##VL\n");
    read_eeprom_send(EEPROM_SEEK_VL,EEPROM_WR_SIZE_VL,Svp_Head_VL);
    // printf("##VP\n");
    read_eeprom_send(EEPROM_SEEK_VP,EEPROM_WR_SIZE_VP,Svp_Head_VP);

    close(fd_eeprom);
    return 0;
}

/********************************************************************************
 * 名称：                    vp_write
 * 功能：                    写入eeprom数据
 * 入口参数：            	 1. 写文件位置，2. 写入参数大小
 * 出口参数：            	 无
 *********************************************************************************/
void vp_write(int seek_size,int write_size)
{
    int recv_date_ret;
    float tem_Array[write_size/sizeof(float)];//获取eeprom中内容转换成float
    char recv_up_Array[write_size];//获取上位机内容后转换为字符的数据，保存到eeprom的数据
 
    recv_date_ret = recv(Connect_fd, recv_up_Array, write_size, 0);
    if (recv_date_ret <= 0)
	{
		error_process();
		return -1;
	}

    memcpy(tem_Array, recv_up_Array, write_size);
    lseek(fd_eeprom, seek_size, SEEK_SET);
    write(fd_eeprom,tem_Array,sizeof(tem_Array));
#if 0
    int i;
    for(i = 0; i < write_size/sizeof(float); i++)
    {
        printf("recv_up_floatArray: %.2f\n", tem_Array[i]);
    }
#endif
}
/************************************存储声速系数存入eeprom********************************************/
int svp_set(void)
{
    printf("<<VP\n");

    char DataHead[6];
    int svp_recv_head;

    svp_recv_head = recv(Connect_fd, DataHead, 6, 0);
    if (svp_recv_head <= 0)
	{
		error_process();
		return -1;
	}

    open_eeprom();
    if ((DataHead[0] == '_') && (DataHead[1] == 'S') && (DataHead[2] == 'V') && (DataHead[3] == 'P') && (DataHead[4] == '_') && (DataHead[5] == '_'))
    {
        printf("_SVP\n");
        vp_write(EEPROM_SEEK_VP,EEPROM_WR_SIZE_VP);//1. 写文件位置，2. 写入参数大小
    }
    if ((DataHead[0] == '_') && (DataHead[1] == 'V') && (DataHead[2] == 'A') && (DataHead[3] == 'L') && (DataHead[4] == 'U') && (DataHead[5] == 'E'))
    {
        printf("_VALUE\n");
        vp_write(EEPROM_SEEK_VL,EEPROM_WR_SIZE_VL);//1. 写文件位置，2. 写入参数大小
    }
    if ((DataHead[0] == '_') && (DataHead[1] == 'T') && (DataHead[2] == 'E') && (DataHead[3] == 'M') && (DataHead[4] == 'P') && (DataHead[5] == '_'))
    {
        printf("_TEMP\n");
        vp_write(EEPROM_SEEK_TM,EEPROM_WR_SIZE_TM);//1. 写文件位置，2. 写入参数大小
    }

    close(fd_eeprom);
    return 0;
}
/************************************配置mems********************************************/
int mems_set(void)
{
    int num;
    char DataHead[6];
    int mems_recv_head;
    int sersor_lenth, Sersor_Lenth;
    char Sersor_Config[100];
    int Sersor_Return[4000];
    char Sersor_Head[4] = { '#','#','S','E' };

	mems_recv_head = recv(Connect_fd, DataHead, 6, 0);
	if (mems_recv_head <= 0)
	{
		error_process();
		return -1;
	}
	if ((DataHead[0] == '_') && (DataHead[1] == 'M') && (DataHead[2] == 'E') && (DataHead[3] == '_') && (DataHead[4] == 'B') && (DataHead[5] == 'G'))
	{
		DBG("<<SC_ME_BG\n");
		mems_recv_head = recv(Connect_fd, (char*)&sersor_lenth, 4, 0);
		if (mems_recv_head <= 0)
		{
			error_process();
			return -1;
		}
		mems_recv_head = recv(Connect_fd, Sersor_Config, sersor_lenth, 0);
		if (mems_recv_head <= 0)
		{
			error_process();
			return -1;
		}

		int m;
		for (m = 0; m < sersor_lenth; ++m)
		{
			printf("sersor_lenth[%d]=%c,%d\n", m, Sersor_Config[m], Sersor_Config[m]);
		}
		printf("1\n");
		memset(uio_mems_register.mem_ptr, 0, 4000); 
		printf("2\n");
		memcpy((char*)uio_mems_register.mem_ptr, Sersor_Config, sersor_lenth);
		printf("3\n");
		ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.uart_cfg_length = sersor_lenth;
		ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.uart_cfg = 1;
		ptr_fpga_register_data->set_pr = 1;//fpga更新中断
		usleep(1000);
		ptr_fpga_register_data->set_pr = 0;
		sleep(1);
		ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.uart_cfg = 0;
		ptr_fpga_register_data->set_pr = 1;
		usleep(1000);
		ptr_fpga_register_data->set_pr = 0;
		memcpy(Sersor_Return, (char*)uio_mems_register.mem_ptr, 1000 * 4);
        // int i;
        // for(i = 0; i <= 10; i++){
        //     printf("mems[%d] = %x\n",i,Sersor_Return[i]);//mems打印信息
        // }
		pthread_mutex_lock(&mut);
		if ((num = send(Connect_fd, Sersor_Head, SIZE_OF_LONG, 0)) < 0) //4byte
		{
			printf("num%d\n", num);
			error_process();
			pthread_mutex_unlock(&mut);
			return -1;
		}
		Sersor_Lenth = 4000;

		if ((num = send(Connect_fd, (char*)&Sersor_Lenth, SIZE_OF_LONG, 0)) < 0) //4byte
		{
			printf("num%d\n", num);
			error_process();
			pthread_mutex_unlock(&mut);
			return -1;
		}
		if ((num = send(Connect_fd, Sersor_Return, 4000, 0)) < 0) //400byte
		{
			printf("num%d\n", num);
			error_process();
			pthread_mutex_unlock(&mut);
			return -1;
		}
		pthread_mutex_unlock(&mut);
        return 0;
	}
}
/************************************更新MEMS********************************************/
int mems_update(void)
{
    printf("<<MEMS\n");

    FILE* fb;
    int i = 0;
    int mems_update_flag; // 更新标志为，发送标志为
    unsigned int ACK_FLAG;
    int mems_success_flag = 0;

    if (send(Connect_fd, RT, 4, 0) < 0) // 4byte
    {
        error_process();
        pthread_mutex_unlock(&mut);
        return -1;
    }
    system("tftp -g -r mems.bin 192.168.0.6");
    sleep(1);

    if (access("./mems.bin", F_OK) == 0)
    {
        ++mems_update_flag;
        DBG("mems.bin exist");
        DBG("mems_update_flag=%d\n", mems_update_flag);
        // pthread_mutex_unlock(&mut);

    }
    if (mems_update_flag == 1)
    {
        // fb = fopen("mems.bin", "rb");
        fb = fopen("mems.bin", "rb");
        if (fb == NULL) {
            DBG("open mems.bin failed !!!\n");
            pthread_mutex_unlock(&mut);
            return -1;
        }
        DBG("open mems.bin success !!!\n");
        fseek(fb, 0, SEEK_END);
        mems_UpData_len = ftell(fb);
        DBG("len=%d\n", mems_UpData_len);
        DBG("uio_mems_register.mem_ptr=%x\n", uio_mems_register.mem_ptr);
        //重启mems命令0x05
        mems_Reboot();
        sleep(5);
        //mems更新标志位
    #if 0
        ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 1;
        usleep(200000);
        ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 0;
        usleep(200000);
        ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 1;
        usleep(200000);
        ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 0;
        usleep(200000);
        ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 1;
        sleep(5);
    #endif
        int up_flag = 0;
        while(!up_flag)
        {
            if(i == 0)
            {
                mems_BeginData(mems_up.mems_head, sizeof(mems_up.mems_head));
 
                sleep(3);//本延长时间比较重要，第一包发送后如果时间过短容易导致后续数据更新不进去
                ACK_FLAG = (unsigned int)ptr_fpga_version_data->fpga_sta.wstatus;
                printf("ACK FLAG = %d\n",ACK_FLAG);
                if(ACK_FLAG == 1)
                {
                    send(Connect_fd, &mems_success_flag, 4, 0);
                    printf("MEMS HEAD UP FAIL!!!\n");
                    return -1;
                    pthread_mutex_unlock(&mut);
                }
            } 
            else 
            {
                int read_size = (i - 1) * 1024;
                if(mems_UpData_len < read_size) 
                {
                    mems_EndData(mems_up.mems_end, sizeof(mems_up.mems_end));
 
                    ACK_FLAG = (unsigned int)ptr_fpga_version_data->fpga_sta.wstatus;
                    printf("ACK FLAG = %d\n",ACK_FLAG);
                    if(ACK_FLAG == 1)
                    {
                        send(Connect_fd, &mems_success_flag, 4, 0);
                        printf("MEMS END UP FAIL!!!");
                        return -1;
                        pthread_mutex_unlock(&mut);
                    }
                    mems_success_flag = 1;
                    send(Connect_fd, &mems_success_flag, 4, 0);
                    ptr_fpga_register_data->wsm_con = 1;
                    Fpga_start_mod = 1;
                    pthread_mutex_unlock(&mut);
                    up_flag = 1;
                    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 0;
                    return 0;
                } 
                else 
                {
                    fseek(fb, read_size, SEEK_SET);
                    fread(mems_up.mems_data, 1, 1024, fb);
                    mems_FileData(mems_up.mems_data, i, mems_UpData_len, read_size);

                    ACK_FLAG = (unsigned int)ptr_fpga_version_data->fpga_sta.wstatus;
                    printf("ACK FLAG = %d\n",ACK_FLAG);
                    if(ACK_FLAG == 1)
                    {
                        send(Connect_fd, &mems_success_flag, 4, 0);
                        printf("MEMS DATA UP FAIL!!!");
                        return -1;
                        pthread_mutex_unlock(&mut);
                    }
                }
            }
            i++;
        }           
    }
}
/************************************更新ZYNQ********************************************/
int zynq_update(void)
{
    printf("<<ZYNQ\n");

    int send_flag = 0;
    // int tftp_flag = 0;
    int update_flag = 0;
    pthread_mutex_lock(&mut);
    
    if (send(Connect_fd, RT, 4, 0) < 0) // 4byte
    {
        error_process();
        pthread_mutex_unlock(&mut);
        return -1;
    }
    // tftp_flag = system ("tftp -g -r update_system.sh 192.168.0.6");

    update_flag = system("./update_system.sh");
    // printf("WIFEXITED(update_flag) = %d\n",WIFEXITED(update_flag));
    // printf("WEXITSTATUS(update_flag) = %d\n",WEXITSTATUS(update_flag));
    if(-1 == update_flag)
    {
        DBG("system failed!!!\n");
        DBG("update_flag = %d\n",update_flag);
        send_flag = 0;
        send(Connect_fd, &send_flag, 4, 0);
        return -1;
        pthread_mutex_unlock(&mut);
    }
    else
    {
        if(WIFEXITED(update_flag))//为真代表system中执行命令为正确命令
        {
            //./update_system.sh success
            if(0 == WEXITSTATUS(update_flag))//WEXITSTATUS为0说明脚本成功运行完成
            {
                send_flag = 1;
                if (send(Connect_fd, &send_flag, 4, 0) < 0) // 4byte
                {
                    error_process();
                    pthread_mutex_unlock(&mut);
                }
                ptr_fpga_register_data->wsm_con = 1;
                Fpga_start_mod = 1;
                pthread_mutex_unlock(&mut);
                return 0;
            }
            else
            {
                DBG("WEXITSTATUS(update_flag) = %d\n",WEXITSTATUS(update_flag));//脚本运行失败返回的错误码
                send_flag = 0;
                if (send(Connect_fd, &send_flag, 4, 0) < 0) // 4byte
                {
                    error_process();
                    pthread_mutex_unlock(&mut);
                }
                printf("ZYNQ UPdate failed!\n");
                return -1;
            } 
        }
        else
        {
            DBG("WIFEXITED(update_flag) = %d\n",WIFEXITED(update_flag));//执行脚本错误返回的值
            send_flag = 0;
            if (send(Connect_fd, &send_flag, 4, 0) < 0) // 4byte
            {
                error_process();
                pthread_mutex_unlock(&mut);
            }
            printf("ZYNQ UPdate failed!\n");
            return -1;
        }
    }
#if 0
    sleep(2);
    system("tftp  -g -r BOOT.BIN 192.168.0.6"); // 系统函数，执行括号内的命令
    sleep(5);
    if (access("./BOOT.BIN", F_OK) == 0) // 判断文件是否存在
    {
        ++update_flag;
        printf("BOOT.BIN exist");
        printf("update_flag=%d\n", update_flag);
    }
    system("tftp  -g -r uImage 192.168.0.6");
    sleep(4);
    if (access("./uImage", F_OK) == 0)
    {
        ++update_flag;
        printf("uImage exist");
        printf("update_flag=%d\n", update_flag);
    }
    system("tftp  -g -r devicetree.dtb 192.168.0.6");
    sleep(4);
    if (access("./devicetree.dtb", F_OK) == 0)
    {
        ++update_flag;
        printf("devicetree.dtb exist");
        printf("update_flag=%d\n", update_flag);
    }
    system("tftp  -g -r uramdisk.image.gz 192.168.0.6");
    sleep(4);
    if (access("./uramdisk.image.gz", F_OK) == 0)
    {
        ++update_flag;
        printf("uramdisk.image.gz exist");
        printf("update_flag=%d\n", update_flag);
    }
    pthread_mutex_lock(&mut);
    // 准备更新发送'##SU'
    len_recv_head1 = send(Connect_fd, RT, 4, 0);
    if (len_recv_head1 < 0) // 4byte
    {
        error_process();
        pthread_mutex_unlock(&mut);
        continue;
    }
    if (update_flag == 4)
    {
        send_flag = 1;
        system("./update_system.sh");
        len_recv_head1 = send(Connect_fd, &send_flag, 4, 0);
        if (len_recv_head1 < 0) // 4byte
        {
            error_process();
            pthread_mutex_unlock(&mut);
            continue;
        }
        printf("len_recv_head1=%d,send_flag=%s\n", len_recv_head1, send_flag);
        printf("ZYNQ UPdate success!\n");
    }
    else
    {
        send_flag = 0;
        if (send(Connect_fd, &send_flag, 4, 0) < 0) // 4byte
        {
            error_process();
            pthread_mutex_unlock(&mut);
            continue;
        }
        printf("ZYNQ UPdate failed!\n");
    }
    pthread_mutex_unlock(&mut);
    ptr_fpga_register_data->wsm_con = 1;
    Fpga_start_mod = 1;
#endif
}
/************************************mems_crc校验********************************************/
int mems_crc16(char *buff, int crc16_len) 
{
    int  crc = 0;
    int  i;
    while(crc16_len--) 
    {
        crc ^= (int)(*(buff++)) << 8;
        for(i = 0; i < 8; i++) 
        {
            if(crc & 0x8000) 
            {
                crc = (crc << 1) ^0x1021;
            } 
            else 
            {
                crc = crc << 1;
            }
        }
    }
    return crc;
}
/************************************mems重启命令0x05********************************************/
void mems_Reboot(void) 
{
    char reboot_flag[63] = {
        0xAA, 0x44, 0x13, 0x05, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8A};

    memset((char *)uio_mems_register.mem_ptr, 0, sizeof(reboot_flag));
    memcpy((char *)uio_mems_register.mem_ptr,reboot_flag, sizeof(reboot_flag));

    DBG("MEMS reboot  OK!!!\n");
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.uart_cfg_length = 63;
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_send_cfg = 1;
    // ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.uart_cfg = 1;
    ptr_fpga_register_data->set_pr = 1;//fpga更新中断
    usleep(1000);
    ptr_fpga_register_data->set_pr = 0;
    usleep(200000);
    // ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.uart_cfg = 0;
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_send_cfg = 0;
    ptr_fpga_register_data->set_pr = 1;
    usleep(1000);
    ptr_fpga_register_data->set_pr = 0;
    sleep(3);
    //mems更新标志位
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 1;
    usleep(200000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 0;
    usleep(200000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 1;
    usleep(200000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 0;
    usleep(200000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 1;
    usleep(200000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 0;
    usleep(200000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_cfg = 1;
}
/************************************mems_head数据********************************************/
void mems_BeginData(char *head_buf, int head_len) 
{
    int head_crc;
    char file_size[5];
    char filename[8] = "mems.bin";
    int filename_len = sizeof(filename);

    memset(head_buf,0,head_len);
    memset(uio_mems_register.mem_ptr, 0, head_len);
    head_buf[0] = SOH;
    head_buf[1] = 0x00;
    head_buf[2] = ~head_buf[1];
    DBG("head_len = %d\n",head_len);

    memcpy(&head_buf[3], filename, filename_len);
    head_buf[3 + filename_len] = 0x00;

    sprintf(file_size,"%d", mems_UpData_len);
    memcpy(&head_buf[3 + filename_len + 1], file_size, sizeof(file_size));

    head_crc = mems_crc16(&head_buf[3],128);

    head_buf[mems_HeadEnd_len - 2] = (char)(head_crc >> 8);
    head_buf[mems_HeadEnd_len - 1] = (char)(head_crc >> 0);
    // int cnt;
    // for(cnt = 0; cnt <= 132; cnt++)
    // {
    //     printf("head_buf[%d] = %x\n",cnt,head_buf[cnt]);//mems打印信息
    // }

    memcpy((char *)uio_mems_register.mem_ptr,head_buf,133);

    DBG("MEMS UP HEAD WRITE FPGA OK!!!\n");
    sleep(1);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.uart_cfg_length = 133;
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_send_cfg = 1;
    ptr_fpga_register_data->set_pr = 1;//fpga更新中断
    usleep(1000);
    ptr_fpga_register_data->set_pr = 0;
    usleep(200000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_send_cfg = 0;
    ptr_fpga_register_data->set_pr = 1;
    usleep(1000);
    ptr_fpga_register_data->set_pr = 0;
}
/************************************mems_data更新包数据********************************************/
void mems_FileData(char *data_buf, int send_cnt, int mems_data_len,int send_size) 
{
    int data_crc;
    char data_buf_send[1029];
    
    memset(data_buf_send,0,sizeof(data_buf_send));
    memset(uio_mems_register.mem_ptr, 0, 1029);
    data_buf_send[0] = STX;
    data_buf_send[1] = send_cnt;
    data_buf_send[2] = ~data_buf_send[1];

    if(mems_data_len - send_size >= 1024)
    {
        memcpy(&data_buf_send[3], data_buf, 1024);
    } 
    else
    {
        memcpy(&data_buf_send[3], data_buf, mems_data_len - send_size);
    }
    //  crc
    data_crc = mems_crc16(&data_buf_send[3], 1024);
    data_buf_send[mems_Data_len - 2] = (char)(data_crc >> 8);
    data_buf_send[mems_Data_len - 1] = (char)(data_crc >> 0);
    
    memcpy((char *)uio_mems_register.mem_ptr,data_buf_send,1029);

    mems_data_cnt++;
    printf("mems cnt = %d\n",mems_data_cnt);

    DBG("MEMS UP DATA WRITE FPGA OK!!!\n");
    usleep(200000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.uart_cfg_length = 1029;
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_send_cfg = 1;
    ptr_fpga_register_data->set_pr = 1;//fpga更新中断
    usleep(1000);
    ptr_fpga_register_data->set_pr = 0;
    usleep(100000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_send_cfg = 0;
    ptr_fpga_register_data->set_pr = 1;
    usleep(1000);
    ptr_fpga_register_data->set_pr = 0;
    usleep(5000);
}
/************************************mems_end更新包数据********************************************/
void mems_EndData(char *end_buf, int end_len) 
{
    int end_crc;
    mems_data_cnt = 0;
    memset(end_buf,0,sizeof(end_buf));
    memset(uio_mems_register.mem_ptr, 0, end_len);
    end_buf[0] = SOH;
    end_buf[1] = 0x00;
    end_buf[2] = ~end_buf[1];
    //  crc
    end_crc = mems_crc16(&end_buf[3], 128);
    end_buf[mems_HeadEnd_len - 2] = (char)(end_crc >> 8);
    end_buf[mems_HeadEnd_len - 1] = (char)(end_crc >> 0);
    // int cnt;
    // for(cnt = 0; cnt <= 132; cnt++)
    // {
    //     printf("data_buf[%d] = %x\n",cnt,end_buf[cnt]);//mems打印信息
    // }

    memcpy((char *)uio_mems_register.mem_ptr,end_buf,133);

    DBG("MEMS UP END WRITE FPGA OK!!!\n");
    usleep(100000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.uart_cfg_length = 133;
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_send_cfg = 1;
    ptr_fpga_register_data->set_pr = 1;//fpga更新中断
    usleep(1000);
    ptr_fpga_register_data->set_pr = 0;
    usleep(200000);
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.mems_up_send_cfg = 0;
    ptr_fpga_register_data->set_pr = 1;
    usleep(1000);
    ptr_fpga_register_data->set_pr = 0;
}

/********************************************************************************
 * 名称：                    Debug_pritf_convert_fpga_parameter
 * 功能：                    打印FPGA的寄存器的内容
 * 入口参数：            	 *ptr_fpga_config_para ARM下发给FPGA的值
 * 出口参数：            	 无
 *********************************************************************************/
void Debug_pritf_convert_fpga_parameter(FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para)
{
    Debug("wsm_con=%d\n", ptr_fpga_register_data->wsm_con);
    Debug("set_pr=%d\n", ptr_fpga_register_data->set_pr);
    Debug("wsm_mod: %d\n", ptr_fpga_config_para->wsm_registers_value.wsm_mod);
    Debug("wsm_ct: %d\n", ptr_fpga_config_para->wsm_registers_value.wsm_ct);
    Debug("adc_sct: %d\n", ptr_fpga_config_para->adc_registers_value.adc_sct);
    Debug("adc_sn: %d\n", ptr_fpga_config_para->adc_registers_value.adc_sn);
    Debug("dac_sct: %d\n", ptr_fpga_config_para->dac_registers_value.dac_sct);
    Debug("dac_sn: %d\n", ptr_fpga_config_para->dac_registers_value.dac_sn);
    Debug("pwm_bf: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_bf);
    Debug("pwm_lfm: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_lfm);
    Debug("pwm_pulse: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_pulse);
    Debug("pwm_start %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_start);
    Debug("pwm_frequency %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_frequency);
    Debug("ADC_SN_after %d\n", ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_iq);
    Debug("ADC_SN_after %d\n", ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_ad);
    // Debug("channel_cfg %d\n",ptr_fpga_config_para->pwm_registers_value.channel_cfg);
}

/********************************************************************************
 * 名称：                    Debug_pritf_receive_para
 * 功能：                    打印显控下发给ARM的内容
 * 入口参数：            	 *receive_upper_package ARM接收上位机的值
 * 出口参数：            	 无
 *********************************************************************************/
void Debug_pritf_receive_para(RECV_UPPER_CONFIG_PARAMETERS *receive_upper_package)
{
    DBG("receive_upper_package\n");
    Debug("TotalBytes: %d\n", receive_upper_package->TotalBytes);
    Debug("DataType: %d\n", receive_upper_package->DataType);
    Debug("WorkMode: %d\n", receive_upper_package->WorkMode);
    Debug("LFMMode: %d\n", receive_upper_package->LFMMode);
    Debug("PWMFreq: %d\n", receive_upper_package->PWMFreq);
    Debug("PWMBandWidth: %d\n", receive_upper_package->PWMBandWidth);
    Debug("SamplingRate: %d\n", receive_upper_package->SamplingRate);
    Debug("Range: %d\n", receive_upper_package->Range);
    Debug("ManualGain: %d\n", receive_upper_package->ManualGain);         // 手动增益的值，-40dB 至40dB
    Debug("AbsorbGainCoef: %d\n", receive_upper_package->AbsorbGainCoef); // 吸收系数值，0 至50 dB/Km
    Debug("SpreadGainCoef: %d\n", receive_upper_package->SpreadGainCoef);
    Debug("PulseWidth: %f\n", receive_upper_package->PulseWidth);
    Debug("PingRate: %f\n", receive_upper_package->PingRate);
    Debug("U_pwm_start: %d\n", receive_upper_package->PwmStart);
    // Debug("SourceChannelC: %d\n", receive_upper_package->SourceChannelC);
}

/*************************test*******************************/
int test(FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para)
{
    ptr_fpga_config_para->wsm_registers_value.wsm_mod = 0;

    ptr_fpga_config_para->wsm_registers_value.wsm_ct = 9433962; // 1/ping_rate*fpga时钟频率=fpga时钟频率/ping_rate
    
    ptr_fpga_config_para->adc_registers_value.adc_sct = 70;              // adc采样周期=(fpga时钟频率/显控下发的采样率)-1
    ptr_fpga_config_para->adc_registers_value.adc_sn = 93332; // adc采样次数=量程×2×采样率/1500 -1
    
    ptr_fpga_config_para->dac_registers_value.dac_sct = 99999;
    ptr_fpga_config_para->dac_registers_value.dac_sn = 67;                                                                                                    //  dac采样次数=TVG采样点数

    ptr_fpga_config_para->pwm_registers_value.pwm_pulse = 5000; // 脉宽=显控脉宽×(时钟频率/1000000.0f)应该是为了us转换成s
    // pwm 频率 AD采样率 量程 pwm带宽 ping率 (fpga回传使用)
    ptr_fpga_config_para->pwm_registers_value.pwm_frequency = 400000;    // pwm中心频率
    ptr_fpga_config_para->pwm_registers_value.sample_rate = 1400; // AD采样率
    ptr_fpga_config_para->pwm_registers_value.range = 50;              // 量程
    ptr_fpga_config_para->pwm_registers_value.band_width = 0;  // pwm带宽
    ptr_fpga_config_para->pwm_registers_value.ping_rate = 10;//帧率（ping）
    // pwm开关
    ptr_fpga_config_para->pwm_registers_value.pwm_start = 0;
    //通道控制字
    // ptr_fpga_config_para->pwm_registers_value.channel_cfg = ptr_recv_upper_package->SourceChannelC;

    ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_iq = 2665;
    // ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_iq = ((unsigned int)(ptr_fpga_config_para->adc_registers_value.adc_sn/SAMPLE_FACTOR));
    ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_ad = 23333;//改加一

    ptr_fpga_config_para->pwm_registers_value.pwm_bf = 400000 * pow(2,32) / 100000000;
    ptr_fpga_config_para->pwm_registers_value.pwm_lfm = 0;

    return 0;
}

