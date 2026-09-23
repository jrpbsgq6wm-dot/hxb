
#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"

#include "eeprom.h"
#include "tmp451.h"

/**************************************socket相关-全局变量**************************************/
volatile int netStatus; // socket的连接状态标志   ;放在判断连接状态的文件中
volatile int net_end_flag;

/*******************************全局变量***************************************/
int bram[8192];
// extern int flag;
int timer_flag = 0;
int Fpga_start_mod = 0;     // FPGA开始工作标志;
char DataTail_S1[4] = {'E','D','>','>'};
static char RT[4] = {'#', '#', 'S', 'U'};
static char ZYNQ_VERSIONS[4] = {'V', '0', '0', '0'}; // zynq版本号

int fd_eeprom;

#define EEPROM_SEEK_TM 0
#define EEPROM_SEEK_VL 8
#define EEPROM_SEEK_VP 128
#define EEPROM_WR_SIZE_TM 8
#define EEPROM_WR_SIZE_VL 120
#define EEPROM_WR_SIZE_VP 120


RECV_UPPER_CONFIG_PARAMETERS recv_upper_package;                            // 接收到上位机的参数配置包----结构体变量
RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package = &recv_upper_package; // 接收到上位机的参数配置包----结构体指针
//接收到上位机的参数配置包保存到本地----结构体变量
RECV_UPPER_CONFIG_PARAMETERS cmd_package; 

SEND_UPPER_SONAR_STATUS send_upper_status_information;                                       // 发送给上位机状态信息----结构体变量
SEND_UPPER_SONAR_STATUS *ptr_send_upper_status_information = &send_upper_status_information; // 发送给上位机状态信息----结构体指针

/*2026.1.08*/   
RECV_UPPER_SONAR_PROBE_CONFIG sonar_probe_config_package;                                       //接收到显控下发的探头配置
RECV_UPPER_SONAR_PROBE_CONFIG* ptr_sonar_probe_config_package = &sonar_probe_config_package;    //接收到显控下发的探头配置的指针
/*******************************函数声明***************************************/
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

/*2026.1.8*/
static void Sonar_probe_setting(void);
static void get_sonar_probe_setting_to_upper(void);
static void send_upper_probe_msg(void);

int test(FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para);
/********************************************************************************
 * 名称：                    Recv_Upper_SendTo_Fpga
 * 功能：                    上位机--->ARM--->FPGA主线程
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void Recv_Upper_SendTo_Fpga(void)
{
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
	    // printf("Recv Upper SendTo Fpga Ok!!!\n");
        /*网络标志位*/
        if (netStatus == 1)
        {
            /*清空数据头*/
            DataHead1[0] = 0;
            DataHead1[1] = 0;
            /*判断数据头是否为<<*/
            while ((DataHead1[0] != '<') || (DataHead1[1] != '<')) {
                /*接收缓冲区前两个字节数，成功返回接收字节数*/
                len_recv_head1 = recv(Connect_fd, DataHead1, 2, 0);
                if(DataHead1[0] == '@' || DataHead1[1] == '@')
                    goto mems;
                if (len_recv_head1 <= 0){
                    /*网络异常处理*/
                    error_process();
                    break;
                }
            }
        mems:
            /*接收实际的指令内容*/
            len_recv_head2 = recv(Connect_fd, DataHead2, 2, 0);
            if (len_recv_head2 < 0){
                error_process();
                continue;
            }
            /*请求硬件版本信息*/
            if ((DataHead2[0] == 'R') && (DataHead2[1] == 'S')){
                /*接收实际的指令内容*/
                len_recv_head2 = recv(Connect_fd, DataHead2, 1, 0);
                if (len_recv_head2 < 0){
                    error_process();
                    continue;
                }
                if((DataHead2[0] == '1')){
                    pthread_mutex_lock(&mut);
                    DBG("RECV upper read status start\r\n");
                    send_status_to_upper();
                    DBG("RECV upper read status over\r\n");
                    pthread_mutex_unlock(&mut);
                }else{
                    continue;
                }
                
            }
            /*固件升级*/
            if ((DataHead2[0] == 'U') && (DataHead2[1] == 'P')){
                ptr_fpga_register_data->wsm_con = 0;
                Fpga_start_mod = 0;
                clear_sensor_data(); // 清除传感器数据
                usleep(5000);
                len_recv_head1 = recv(Connect_fd, DataHead3, 5, 0);
                if (len_recv_head1 <= 0){
                    error_process();
                    break;
                }
                if ((DataHead3[0] == '_') && (DataHead3[1] == 'Z') && (DataHead3[2] == 'Y') && (DataHead3[3] == 'N') && (DataHead3[4] == 'Q')){
                    pthread_mutex_lock(&mut);
                    zynq_update();
                    pthread_mutex_unlock(&mut);
                }
                printf("update successfully\r\n");
                ptr_fpga_register_data->wsm_con = 1;
                Fpga_start_mod = 1;
                printf("fpga workmode is ok\r\n");
            }
            /*开始请求数据*/
            if ((DataHead2[0] == 'B') && (DataHead2[1] == 'R')){
                pthread_mutex_lock(&mut);
                printf("**************recv upper BR**************\r\n");
                /*FPGA 工作控制寄存器*/
                ptr_fpga_register_data->wsm_con = 1;
                /*FPGA开始工作标志;*/
                Fpga_start_mod = 1;
                pthread_mutex_unlock(&mut);
                
            }
            /*停止请求波形数据*/
            if ((DataHead2[0] == 'E') && (DataHead2[1] == 'R')){
                pthread_mutex_lock(&mut);
                printf("**************recv upper ER**************\r\n"  );
                /*FPGA 工作控制寄存器*/
                ptr_fpga_register_data->wsm_con = 0;
                /*FPGA开始工作标志;*/
                Fpga_start_mod = 0;
                clear_sensor_data();
                // sleep(1);
                // net_end_flag = 1;
                pthread_mutex_unlock(&mut);
            }
            /*获取当前IP配置和端口号配置*/
            if ((DataHead2[0] == 'I') && (DataHead2[1] == 'P')){ 
                pthread_mutex_lock(&mut);
                get_sonar_probe_setting_to_upper();
                pthread_mutex_unlock(&mut);
            }
            /*设置声呐的各项参数(接收)*/
            if ((DataHead2[0] == 'S') && (DataHead2[1] == 'P')){
                pthread_mutex_lock(&mut);
                send_fpga_config();
                pthread_mutex_unlock(&mut);
                if(net_end_flag == 1)
                {
                    netStatus = 0;
                    net_end_flag = 0;
                    close(Connect_fd);
                }
            }
            /*配置声纳探头参数配置*/
            if ((DataHead2[0] == 'S') && (DataHead2[1] == 'C')){
                pthread_mutex_lock(&mut);
                Sonar_probe_setting();
                pthread_mutex_unlock(&mut);
            }
            /*读/写eeprom中的厂商信息*/
            if ((DataHead2[0] == 'H') && (DataHead2[1] == 'E')){
                printf("<<HE eeprom_fd%d\r\n",eeprom_fd);
                pthread_mutex_lock(&mut);
                if(eeprom_fd != -1){
                    eeprom_id_func();
                }
                pthread_mutex_unlock(&mut);
            } 
            /*<<SI”表示获取内置惯导信息*/
            if ((DataHead2[0] == 'S') && (DataHead2[1] == 'I')){
                pthread_mutex_lock(&mut);
                /*获取内置惯导信息*/
                si_send_upper_probe_msg();
                pthread_mutex_unlock(&mut);
            } 
            /*<<SO”表示获取外置惯导信息*/
            if ((DataHead2[0] == 'S') && (DataHead2[1] == 'O')){
                pthread_mutex_lock(&mut);
                /*获取外置惯导信息*/
                so_send_upper_probe_msg();
                pthread_mutex_unlock(&mut);
            } 
        }else{
            if ((Connect_fd = accept(Socket_fd_server, (struct sockaddr *)NULL, NULL)) == -1){
                printf("accept socket error: %s(errno: %d)", strerror(errno), errno);
                continue;
            } // 阻塞
            netStatus = 1;
            DBG("\n======printf accept successfully======\n");
            //获取当前已经建立tcp连接的设备IP
            if(get_peer_ip(Connect_fd,upper_ip,sizeof(upper_ip)) == 0){
                DBG("client ip is %s\n",upper_ip);
            }else{
                DBG("client ip get failed \n");
            }
            setkeepalive(Connect_fd);
        }
    }
#endif
}

/************************************下发fpga配置********************************************/
int send_fpga_config(void){
    int  len_recv;
    char DataTail[5]; // 数据尾标识“ED>>”
    int  len_recv_crc;
    unsigned short recv_crc;
    unsigned short recv_crc_calculate;

#if 0
    /* 暂存FPGA工作状态 */
    int fpga_works = ptr_fpga_register_data->wsm_con;
    int alter_flag = 0;
    printf("ptr_fpga_register_data->wsm_con %d",fpga_works);
#endif

    //填充参数配置结构体
    len_recv = recv_socket(Connect_fd, (char *)(ptr_recv_upper_package), SIZE_OF_CONFIG_PARA); 
    if (len_recv < 0)
    {
        error_process();
        return 1;
    }
    //接收CRC校验
    len_recv_crc = recv_socket(Connect_fd, (char *)&recv_crc, SIZE_OF_LONG); 
    // DBG("len_recv_crc: %d\n",len_recv_crc);
    if (len_recv_crc < 0){
        error_process();
        return 1;
    }

    recv_socket(Connect_fd, DataTail, SIZE_OF_LONG); // 接受尾“ED>>”
    if (strncmp(DataTail, DataTail_S1, 4) == 0){
        //printf("DataTail:%c %c %c %c\n", DataTail[0], DataTail[1], DataTail[2], DataTail[3]);
    }else{
        printf("DataTail error%s\n", DataTail);
        return 1;
    }

    recv_crc_calculate = crc16((char *)(ptr_recv_upper_package), SIZE_OF_CONFIG_PARA, 0xFFFF);
    if (recv_crc_calculate != recv_crc){
        printf("recv_crc_calculate:%x \n", recv_crc_calculate);
        printf("recv_crc:%x \n", recv_crc);
        printf("crc error! \n");
        return 1;
    }

    if(ptr_recv_upper_package->INS_mode != cmd_package.INS_mode){
        //线控下发指令与当前设置的惯导模式不相等 - 清空缓冲区
        //printf("ptr_recv_upper_package->INS_mode %d\r\n",ptr_recv_upper_package->INS_mode);
        //printf("cmd_package.INS_mode %d\r\n",cmd_package.INS_mode);
        memset(uio_baseddr_sensor.mem_ptr,0,uio_baseddr_sensor.mem_size);
        memset(uio_baseddr_sensor_1.mem_ptr,0,uio_baseddr_sensor_1.mem_size);
        memset(uio_baseddr_pashr.mem_ptr,0,uio_baseddr_pashr.mem_size);
        memset(uio_baseddr_pashr_1.mem_ptr,0,uio_baseddr_pashr_1.mem_size);
    }

# if 0
    /*
        判断当前ping率与上一次ping率是否相等
        判断当前ping模式与上一次ping模式是否相等
    */
    if((ptr_recv_upper_package->Ping_mode != cmd_package.Ping_mode)){
        //暂停FPGA
        ptr_fpga_register_data->wsm_con = 0;
        Fpga_start_mod = 0;
        alter_flag = 1;
        printf("recv Alter Pingmode - ptr_fpga_register_data->wsm_con:%d - delay 100ms\n",ptr_fpga_register_data->wsm_con);
        //延时100ms
        usleep(100000); // 100ms
    }else{
        alter_flag = 0;
    }
#endif

    //刷新保存在本地的参数配置包
    copy_recv_cmd(&cmd_package, ptr_recv_upper_package);                                    // 拷贝到本地

#if 1
    Debug_pritf_receive_para(&cmd_package);                                                 // 打印上位机下发到arm的配置信息
#endif    
    //接收到的上位机数据转化为FPGA寄存器值
    parsing_instructions_200k(&cmd_package, &(fpga_register_data.fpga_registers_200k));// 解析fpga寄存器
    ptr_fpga_register_data->fpga_registers_200k = fpga_register_data.fpga_registers_200k; 
    // 下发FPGA参数tvg配置信息                    
    memcpy((char *)uio_tvg_register.mem_ptr, (char *)cmd_package.tvgGain, 3200); 
#if 1
    //打印发送给fpga的参数信息
    Debug_pritf_convert_fpga_parameter(&(fpga_register_data.fpga_registers_200k)); 
#endif

#if 0
    if(alter_flag){
        ptr_fpga_register_data->wsm_con = fpga_works;
        Fpga_start_mod = fpga_works;
        printf("Resume fpga work status\n");
    }
#endif

    //DBG("TVG send to fpga ok!\n");
    ptr_fpga_register_data->set_pr = 1;
    usleep(1000); // 1ms
    ptr_fpga_register_data->set_pr = 0;
    return 0;
}
/************************************清空传感器数据********************************************/
void clear_sensor_data(void)
{
    int m;
    for (m = 0; m <= 50; m++)
    {
        *(uio_baseddr_sensor.mem_ptr + m * 64) = 0;
        *(uio_baseddr_sensor.mem_ptr + m * 64 + 1) = 0;
        *(uio_baseddr_sensor_1.mem_ptr + m * 64) = 0;
        *(uio_baseddr_sensor_1.mem_ptr + m * 64 + 1) = 0;
    }
    memset(uio_baseddr_head.mem_ptr, 0, 256);
    memset(uio_baseddr_head_1.mem_ptr, 0, 256);
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
    char DataHead[5] = {'#', 'S', 'S','1'};  
    /*填充硬件信息字段*/
    ptr_send_upper_status_information->FPGAVersion = (unsigned int)ptr_fpga_version_data->fpga_sta.date;    // FPGA版本信息,版本时间
    printf("PL:%d\r\n", ptr_send_upper_status_information->FPGAVersion);
    //根据主从 传回不同版本号
    if(m_s_flag){
        ptr_send_upper_status_information->LinuxVersion = MASTER_VER;// ARM版本号信息
    }else{
        ptr_send_upper_status_information->LinuxVersion = SLAVE_VER;// ARM版本号信息
    }
    printf("PS:%d\r\n", ptr_send_upper_status_information->LinuxVersion);
    
    /*板载温度 舱内温度待添加*/
    ptr_send_upper_status_information->Onboard_temp = (unsigned int)0;
    ptr_send_upper_status_information->Cabin_temp = (unsigned int)local_temp;
    memset(ptr_send_upper_status_information->Reserved,0,16);
    ptr_to_send = (char *)ptr_send_upper_status_information;
    //发送数据头
    if (send(Connect_fd, DataHead, SIZE_OF_STATUS_HEAD_LEN, 0) < 0) {
        error_process();
        return;
    }
    //发送数据
    if (send(Connect_fd, ptr_to_send, SIZE_OF_STATUS_SEND, 0) < 0){        
        error_process();
        return;
    }
    //发送数据尾
    if (send(Connect_fd, DataTail_S1, SIZE_OF_STATUS_TAIL_LEN, 0) < 0){   
        error_process();
        return;
    }
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
    //工作模式寄存器
    ptr_fpga_config_para->wsm_registers_value.wsm_mod = (ptr_recv_upper_package->WorkMode) | (ptr_recv_upper_package->LFMMode << 1) | (ptr_recv_upper_package->DataType << 2);
    //工作周期寄存器 1/ping率*FPGA时钟频率
    if (ptr_recv_upper_package->PingRate != 0){
        ptr_fpga_config_para->wsm_registers_value.wsm_ct = (unsigned int)(FPGA_CLK_FREQUENCY / ptr_recv_upper_package->PingRate); // 1/ping_rate*fpga时钟频率=fpga时钟频率/ping_rate
    }else{
        printf("revc upper PingRate is 0!\n");
        return -1;
    }
    //ADC 采样周期寄存器
    ptr_fpga_config_para->adc_registers_value.adc_sct = (unsigned int)((FPGA_CLK_FREQUENCY / (ptr_recv_upper_package->SamplingRate * 1000)) - 1);              // adc采样周期=(fpga时钟频率/显控下发的采样率)-1
    //ADC 采样次数寄存器
    ptr_fpga_config_para->adc_registers_value.adc_sn =
                 (unsigned int)(((ptr_recv_upper_package->Range * 2 * ptr_recv_upper_package->SamplingRate * 1000) / V_SOUND) - 1); // adc采样次数=（量程×2×采样率)/1500 -1
    //DAC 采样周期寄存器
    ptr_fpga_config_para->dac_registers_value.dac_sct = 99999;
    //DAC 采样点数寄存器
    ptr_fpga_config_para->dac_registers_value.dac_sn = (unsigned int)((ptr_recv_upper_package->Range * 2 * 1000 / V_SOUND) + 1);                                                                                                    //  dac采样次数=TVG采样点数
    //PWM脉冲时宽寄存器
    ptr_fpga_config_para->pwm_registers_value.pwm_pulse = (unsigned int)(ptr_recv_upper_package->PulseWidth * (FPGA_CLK_FREQUENCY / 1000000.0f)); // 脉宽=显控脉宽×(时钟频率/1000000.0f)应该是为了us转换成s
    //PWM中心频率
    ptr_fpga_config_para->pwm_registers_value.pwm_frequency = (unsigned int)(ptr_recv_upper_package->PWMFreq);    // pwm中心频率
    //AD采样率
    ptr_fpga_config_para->pwm_registers_value.sample_rate = (unsigned int)(ptr_recv_upper_package->SamplingRate); // AD采样率
    //量程
    ptr_fpga_config_para->pwm_registers_value.range = (unsigned int)(ptr_recv_upper_package->Range);              // 量程
    //带宽
    ptr_fpga_config_para->pwm_registers_value.band_width = (unsigned int)(ptr_recv_upper_package->PWMBandWidth);  // pwm带宽
    //PING率
    ptr_fpga_config_para->pwm_registers_value.ping_rate = (unsigned int)(ptr_recv_upper_package->PingRate);//帧率（ping）
    // pwm开关 PWM开始标志位
    ptr_fpga_config_para->pwm_registers_value.pwm_start = (unsigned int)ptr_recv_upper_package->PwmStart;
    //通道控制字
    // ptr_fpga_config_para->pwm_registers_value.channel_cfg = ptr_recv_upper_package->SourceChannelC;
    /*
        IQ个数 = ADC 采样次数（（（量程 * 2 * 采样率 *1000 ）/ 1500 ）- 1） / 抽样因子
    */
    ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_iq = ((unsigned int)(ptr_fpga_config_para->adc_registers_value.adc_sn/SAMPLE_FACTOR)-1);
    // ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_iq = ((unsigned int)(ptr_fpga_config_para->adc_registers_value.adc_sn/SAMPLE_FACTOR));
    /*
        AD个数 = ADC 采样次数（（（量程 * 2 * 采样率 *1000 ）/ 1500 ）- 1）/ 抽样因子 + 1
    */
    ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_ad = ((unsigned int)(ptr_fpga_config_para->adc_registers_value.adc_sn/AD_SAMPLE_FACTOR)+1);//改加一

    if (ptr_recv_upper_package->WorkMode == 0) // CW：模式
    {
        //PWM基频寄存器
        ptr_fpga_config_para->pwm_registers_value.pwm_bf = (unsigned int)((double)(ptr_recv_upper_package->PWMFreq * 1000 * N2_32) / FPGA_CLK_FREQUENCY);
        //PWM调频寄存器
        ptr_fpga_config_para->pwm_registers_value.pwm_lfm = 0;
    }
    if (ptr_recv_upper_package->WorkMode == 1) // LFM：模式
    {
        //PWM调频寄存器
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

    /*新增 4070 探头配置*/
    ptr_fpga_config_para->pwm_registers_value.Freq_value = (unsigned int )(ptr_recv_upper_package->PWMFreq) * 1000;
    /*0x64 内外惯导选择*/
    ptr_fpga_config_para->pwm_registers_value.Sensor_select = (unsigned int )(ptr_recv_upper_package->INS_mode);
    // //探头模式配置 bit1主从探头Probe_Logo  bit0单双探头Probe_mode
    char probemode,probelogo;
    if(ptr_recv_upper_package->Probe_mode == 1){   //双探头
        if(strncmp(arm_ipaddr,MASTER_IPADDR,strlen(arm_ipaddr)) == 0){
            printf("配置为双探头 主设备\r\n");
            ptr_fpga_config_para->pwm_registers_value.Double_sonar = 3;
        }else if(strncmp(arm_ipaddr,SLAVE_IPADDR,strlen(arm_ipaddr)) == 0){
            ptr_fpga_config_para->pwm_registers_value.Double_sonar = 1;
            printf("配置为双探头 从设备\r\n");
        }
        if(ptr_recv_upper_package->Ping_mode == 1){
            ptr_fpga_config_para->pwm_registers_value.Double_sonar = (ptr_fpga_config_para->pwm_registers_value.Double_sonar & 0xFFFF00FF) | 0x00000100;
        }else if(ptr_recv_upper_package->Ping_mode == 0){
            ptr_fpga_config_para->pwm_registers_value.Double_sonar = ptr_fpga_config_para->pwm_registers_value.Double_sonar & 0xFFFF00FF;
        }
    }else if(ptr_recv_upper_package->Probe_mode == 0){  //单探头
        ptr_fpga_config_para->pwm_registers_value.Double_sonar = 2;
        printf("配置为单探头 主设备\r\n");
    }

    /* AD SDO 延时 */
    ptr_fpga_config_para->pwm_registers_value.AD_SDO_delay = (unsigned int)(ptr_recv_upper_package->AD_SDO_delay);
    /* 功率*/
    ptr_fpga_config_para->pwm_registers_value.power = (unsigned int)(ptr_recv_upper_package->Power);
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
/************************************更新ZYNQ********************************************/
int zynq_update(void){
    char send_head[4] = {'#','#','S','U'};
    int ret = 0;
    uint32_t updata_flag = 0;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "cd /home/root && /home/root/upgrade.sh %s 5", upper_ip);
    printf("exec cmd: %s\n", cmd);
    ret = system(cmd);
    
    if(ret == 0){
        //所有文件下载成功
        updata_flag =1;
    }else{
        //固件下载失败
        updata_flag = 0;
    }
    //发送数据头
    if (send(Connect_fd, send_head, 4, 0) < 0) {
        error_process();
        return -1;
    }
    //发送数据
    if (send(Connect_fd, &updata_flag, 4, 0) < 0){        
        error_process();
        return -1;
    }
    return 0;
}
/********************************************************************************
 * 名称：                    Debug_pritf_convert_fpga_parameter
 * 功能：                    打印FPGA的寄存器的内容
 * 入口参数：            	 *ptr_fpga_config_para ARM下发给FPGA的值
 * 出口参数：            	 无
 *********************************************************************************/
void Debug_pritf_convert_fpga_parameter(FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para)
{
    printf("***************************\n");
    printf("Recv upper package forward to FPGA registers\n");
    Debug("wsm_con=%d\n", ptr_fpga_register_data->wsm_con);
    Debug("set_pr=%d\n", ptr_fpga_register_data->set_pr);
    Debug("WSM_REGISTERS\n");
    Debug("wsm_mod: %d\n", ptr_fpga_config_para->wsm_registers_value.wsm_mod);
    Debug("wsm_ct: %d\n", ptr_fpga_config_para->wsm_registers_value.wsm_ct);
    Debug("ADC_REGISTERS\n");
    Debug("adc_sct: %d\n", ptr_fpga_config_para->adc_registers_value.adc_sct);
    Debug("adc_sn: %d\n", ptr_fpga_config_para->adc_registers_value.adc_sn);
    Debug("DAC_REGISTERS\n");
    Debug("dac_sct: %d\n", ptr_fpga_config_para->dac_registers_value.dac_sct);
    Debug("dac_sn: %d\n", ptr_fpga_config_para->dac_registers_value.dac_sn);
    Debug("PWM_REGISTERS\n");
    Debug("pwm_bf: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_bf);
    Debug("pwm_lfm: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_lfm);
    Debug("pwm_pulse: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_pulse);
    Debug("pwm_frequency %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_frequency);
    Debug("sample_rate %d\n", ptr_fpga_config_para->pwm_registers_value.sample_rate);
    Debug("range %d\n", ptr_fpga_config_para->pwm_registers_value.range);
    Debug("band_width %d\n", ptr_fpga_config_para->pwm_registers_value.band_width);
    Debug("ping_rate %d\n", ptr_fpga_config_para->pwm_registers_value.ping_rate);
    Debug("pwm_start %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_start);
    Debug("pwm_start %d\n", ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_iq);
    Debug("pwm_start %d\n", ptr_fpga_config_para->pwm_registers_value.ADC_SN_af_ad);
    Debug("uart_cfg %d\n", ptr_fpga_config_para->pwm_registers_value.uart_cfg);
    Debug("uart_cfg_length %d\n", ptr_fpga_config_para->pwm_registers_value.uart_cfg_length);
    Debug("uart_cfg_baud %d\n", ptr_fpga_config_para->pwm_registers_value.uart_cfg_baud);
    Debug("mems_up_cfg %d\n", ptr_fpga_config_para->pwm_registers_value.mems_up_cfg);
    Debug("mems_up_send_cfg %d\n", ptr_fpga_config_para->pwm_registers_value.mems_up_send_cfg);
    Debug("Freq_value %d\n", ptr_fpga_config_para->pwm_registers_value.Freq_value);
    Debug("Sensor_select %d\n", ptr_fpga_config_para->pwm_registers_value.Sensor_select);
    Debug("Double_sonar %d\n", ptr_fpga_config_para->pwm_registers_value.Double_sonar);
    Debug("AD_SDO_delay %d\n", ptr_fpga_config_para->pwm_registers_value.AD_SDO_delay);
    Debug("power %d\n", ptr_fpga_config_para->pwm_registers_value.power);
    printf("***************************\n");
}

/********************************************************************************
 * 名称：                    Debug_pritf_receive_para
 * 功能：                    打印显控下发给ARM的内容
 * 入口参数：            	 *receive_upper_package ARM接收上位机的值
 * 出口参数：            	 无
 *********************************************************************************/
void Debug_pritf_receive_para(RECV_UPPER_CONFIG_PARAMETERS *receive_upper_package)
{
    printf("***************************\n");
    printf("Recv upper package\n");
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
    Debug("Launch_gear: %d\n", receive_upper_package->Launch_gear);
    Debug("PulseWidth: %f\n", receive_upper_package->PulseWidth);
    Debug("PingRate: %f\n", receive_upper_package->PingRate);
    Debug("PwmStart: %d\n", receive_upper_package->PwmStart);
    Debug("AD_NUM: %d\n", receive_upper_package->AD_NUM);
    Debug("Power: %f\n", receive_upper_package->Power);
    Debug("Probe_mode: %d\n", receive_upper_package->Probe_mode);
    Debug("Probe_Logo: %d\n", receive_upper_package->Probe_Logo);
    Debug("Install_angle: %f\n", receive_upper_package->Install_angle);
    Debug("Ping_mode: %d\n", receive_upper_package->Ping_mode);
    Debug("INS_mode: %d\n", receive_upper_package->INS_mode);
    Debug("AD_SDO_delay: %d\n", receive_upper_package->AD_SDO_delay);
    printf("***************************\n");
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


/********************************************************************************
 * 名称：         Sonar_probe_setting
 * 功能：         声纳探头配置 
 * 入口参数：      
 * 出口参数：      
 *********************************************************************************/
static void Sonar_probe_setting(void){
    char DataHead[4] = {'<', '<', 'S', 'C'};    // 数据头标示“<<SS”,表示数据头
    char DataTail[4] = {'E','D','>','>'};       //数据尾
    char Databuf[sizeof(RECV_UPPER_SONAR_PROBE_CONFIG)] = {0};
    char ipaddr[15] = {0};
    /*接收显控下发配置的总字节数*/
    int  len_recv = 0,settinglen = 0,offsetptr = 0;
    len_recv = recv_socket(Connect_fd,(char*)ptr_sonar_probe_config_package, SIZE_OF_SONAR_PROBE_LEN);
    if (len_recv < 0){
        error_process();
        return 1;
    }
    /*检查arm ip配置是否与下发配置相同*/
    System_IPconfig("eth0",ipaddr);
    DBG("System_IPconfig(): %s",ipaddr);
    DBG("Recv_upper_config: %s",ptr_sonar_probe_config_package->IPaddr);
    if(strncmp(ipaddr,ptr_sonar_probe_config_package->IPaddr,strlen(ipaddr)) != 0 ){
        //备份 更新interface文件 和 ip
        Alter_IPaddress("eth0",ptr_sonar_probe_config_package->IPaddr);
    }//IP配置相同 无需备份修改

    /*状态更新FPGA*/
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.Sensor_select = (unsigned int)ptr_sonar_probe_config_package->INS_mode;
    ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.Double_sonar = (unsigned int)ptr_sonar_probe_config_package->Probe_Logo;
    
    //将配置更新到EEPROM
    //eeprom_Sonar_probe_setting_w(SONAR_PROBE_CONFIG_START_ADDR,ptr_sonar_probe_config_package,sizeof(ptr_sonar_probe_config_package));

    /*构建发送返回信息*/
    //发送数据头 4byte
    if (send(Connect_fd, DataHead, SIZE_OF_LONG, 0) < 0) {
        error_process();
        return;
    }
    //发送数据
    if (send(Connect_fd, (char*)ptr_sonar_probe_config_package, SIZE_OF_SONAR_PROBE_LEN, 0) < 0){        
        error_process();
        return;
    }
    //发送数据尾 4byte
    if (send(Connect_fd, DataTail, SIZE_OF_STATUS_TAIL_LEN, 0) < 0){   
        error_process();
        return;
    }
}

/********************************************************************************
 * 名称：         get_sonar_probe_setting_to_upper
 * 功能：         获取当前设备的声纳ip等参数配置 发送给显控
 * 入口参数：      
 * 出口参数：      
 *********************************************************************************/
static void get_sonar_probe_setting_to_upper(void){
    char DataHead[4] = {'#', '#', 'I', 'P'};    //数据头
    char DataTail[4] = {'E','D','>','>'};       //数据尾
    char sdbuf[1024];
    // 声明结构体并初始化（避免随机值）
    SEND_UPPER_IP_CONFIG ipconfig = {0};
    int ret,index;
    // 1. 获取IP地址并确保安全（填充前清空数组）
    memset(ipconfig.IPaddr, 0, sizeof(ipconfig.IPaddr));
    System_IPconfig("eth0", ipconfig.IPaddr);
    // 截断过长的IP地址，避免越界（最多15个有效字符 + 1个\0）
    ipconfig.IPaddr[15] = '\0';

    // 2. 填充端口号（明确类型转换）
    ipconfig.port = 7000;  // unsigned int 直接赋值，无需(int)转换
    // 3. 计算IP长度（确保不超过15）
    ipconfig.IPaddr_len = strlen(ipconfig.IPaddr);
    if (ipconfig.IPaddr_len > 15) {
        ipconfig.IPaddr_len = 15;  // 限制最大长度
    }
    // 注释说明：0=主探头，1=从探头
    if(strncmp(ipconfig.IPaddr, MAIN_SONAR_DEF_IP, ipconfig.IPaddr_len) == 0 ){
        ipconfig.Probe_Logo = 0;  // 主探头赋值0（匹配注释定义）
    }else if(strncmp(ipconfig.IPaddr, MINOR_SONAR_DEF_IP, ipconfig.IPaddr_len) == 0 ){
        ipconfig.Probe_Logo = 1;  // 从探头赋值1（匹配注释定义）
    }else{
        // IP地址错误，赋值错误标识+打印日志
        ipconfig.Probe_Logo = 0xFF;  // 用0xFF表示无效探头
        printf("Error: IP address %s is not main/minor sonar IP!\n", ipconfig.IPaddr);
    }
/*
    printf("get_sonar_probe_setting_to_upper_ipconfig.Probe_Logo:%d\n",ipconfig.Probe_Logo);
    printf("get_sonar_probe_setting_to_upper_ipconfig.IPaddr_len:%d\n",ipconfig.IPaddr_len);
    printf("get_sonar_probe_setting_to_upper_ipconfig.IPaddr:%s\n",ipconfig.IPaddr);
    printf("get_sonar_probe_setting_to_upper_ipconfig.port:%d\n",ipconfig.port);
*/
    //发送数据头 4byte
    if (send(Connect_fd, DataHead, SIZE_OF_GENERAL_HEAD_LEN, 0) < 0) {
        error_process();
        return;
    }
    //发送数据
    if (send(Connect_fd, (char*)&ipconfig, SIZE_OF_IP_CONFIG_LEN, 0) < 0){        
        error_process();
        return;
    }
    //发送数据尾 4byte
    if (send(Connect_fd, DataTail, SIZE_OF_GENERAL_TAIL_LEN, 0) < 0){   
        error_process();
        return;
    }
/*
    memcpy(sdbuf,&ipconfig,SIZE_OF_IP_CONFIG_LEN);
    printf("\n===== 最终发送的指令内容 =====\n");
    printf("可打印字符形式：");
    for (int i = 0; i < SIZE_OF_IP_CONFIG_LEN; i++) {
            printf("%02X ", (unsigned char)sdbuf[i]);
    }
    printf("\n");
*/
}

 /********************************************************************************
 * 名称：                    Sonar_probe_config_init() - 暂未使用
 * 功能：                    eeprom探头参数初始化
 * 入口参数：            	 无
 * 出口参数：            	 
 *********************************************************************************/
 void Sonar_probe_config_init(void){
    //eeprom_Sonar_probe_setting_r(SONAR_PROBE_CONFIG_START_ADDR,ptr_sonar_probe_config_package,sizeof(ptr_sonar_probe_config_package));
    //eeprom 探头参数没有进行过设置
    if(ptr_sonar_probe_config_package->IPaddr_len == 0){
        ptr_sonar_probe_config_package->IPaddr_len = (int)11;
        memcpy(ptr_sonar_probe_config_package->IPaddr,MAIN_SONAR_DEF_IP,strlen(MAIN_SONAR_DEF_IP));
        ptr_sonar_probe_config_package->port = (int)7000;
    }
}

/*
    提取传感器数据（适配发给PC显控，含\0）
    返回：成功返回传输的总字节数（含\0），失败返回-1
*/
static int Extract_serson_buf_msg(const char* buf, int len, char* returnbuf)
{
    if (buf == NULL || returnbuf == NULL || len <= 0) {
        return -1;
    }

    int dollar_pos = -1;
    int star_pos = -1;
    int i, out_idx = 0;

    // 找$的位置
    for (i = 0; i < len; i++) {
        if (buf[i] == '$') {
            dollar_pos = i;
            break;
        }
    }
    if (dollar_pos == -1) return -1;

    // 找*的位置（必须在$之后）
    for (i = dollar_pos + 1; i < len; i++) {
        if (buf[i] == '*') {
            star_pos = i;
            break;
        }
    }
    if (star_pos == -1 || star_pos <= dollar_pos) return -1;

    // 写入$
    returnbuf[out_idx++] = '$';
    // 提取中间有效字符（跳过00）
    for (i = dollar_pos + 1; i < star_pos; i++) {
        if (buf[i] != 0x00) {
            returnbuf[out_idx++] = buf[i];
        }
    }
    // 写入*
    returnbuf[out_idx++] = '*';

    // 返回总传输字节数（含\0）
    return out_idx;
}

/*
    通用函数：提取传感器数据（兼容双缓冲区）
    返回：提取到的字节数（含\0），失败返回0
*/
static int extract_sensor_data(const char *buf1, const char *buf2, int len, char *outbuf)
{
    int ret = Extract_serson_buf_msg(buf1, len, outbuf);
    if (ret == -1) {
        ret = Extract_serson_buf_msg(buf2, len, outbuf);
        if (ret == -1) {
            return 0; // 两次提取都失败，返回0
        }
    }
    return ret;
}

/********************************************************************************
 * 名称：                    send_upper_probe_msg()
 * 功能：                    获取传感器数据并发送给PC显控
 * 说明：                    数据包格式：[总长度(4字节)] + [GNSS数据] + [\n] + [PASHR数据]
 *********************************************************************************/
void send_upper_probe_msg(void)
{
    char sendbuf[1024] = {0}; // 初始化缓冲区，避免脏数据
    char gnssbuf[256] = {0};  // GNSS数据缓冲区
    char pashrbuf[256] = {0}; // PASHR数据缓冲区
    int gnsslen = 0;          // 初始化为0，避免脏数据
    int pashrlen = 0;
    uint32_t tmp_len;         // 用于转换长度的临时变量
    int total_len;            // 总发送长度
    char newline = '\n';      // 换行符变量

    // 1. 提取GNSS数据（双缓冲区容错）
    gnsslen = extract_sensor_data(uio_baseddr_sensor.mem_ptr,
                                  uio_baseddr_sensor_1.mem_ptr,
                                  256, gnssbuf);

    // 2. 提取PASHR数据（双缓冲区容错）
    pashrlen = extract_sensor_data(uio_baseddr_pashr.mem_ptr,
                                   uio_baseddr_pashr_1.mem_ptr,
                                   256, pashrbuf);

    // 3. 组合数据包
    // 3.1 拷贝总长度（GNSS长度+PASHR长度，4字节小端）
    tmp_len = (uint32_t)(gnsslen + pashrlen + 1);
    memcpy(sendbuf, &tmp_len, 4);

    // 3.2 拷贝GNSS数据（仅当长度>0时拷贝）
    if (gnsslen > 0) {
        memcpy(sendbuf + 4, gnssbuf, gnsslen);
    }

    // 3.3 插入换行符
     memcpy(sendbuf + 4 + gnsslen, &newline, 1);

    // 3.4 拷贝PASHR数据（仅当长度>0时拷贝）
    if (pashrlen > 0) {
        memcpy(sendbuf + 4 + gnsslen + 1, pashrbuf, pashrlen);
    }

    // 4. 计算总发送长度并检查越界
    total_len = 4 + gnsslen + pashrlen + 1;

    // 调试打印（修复格式化输出错误）

    printf("gnsslen = %d\n", gnsslen);
    printf("pashrlen = %d\n", pashrlen);
    printf("gnssmsg: %s\n", gnssbuf);   
    printf("pashrbuf: %s\n", pashrbuf);

    // 5. 发送数据包到PC显控
    if (send(Connect_fd, sendbuf, total_len, 0) < 0) {   
        error_process();
        return;
    }
}
void si_send_upper_probe_msg(void){
    char headbuf[4] = {'#','#','S','I'};
    if (send(Connect_fd, headbuf, 4, 0) < 0) {
        error_process();
        return;
    }
    send_upper_probe_msg();
}

void so_send_upper_probe_msg(void){
    char headbuf[4] = {'#','#','S','0'};
    if (send(Connect_fd, headbuf, 4, 0) < 0) {
        error_process();
        return;
    }
    send_upper_probe_msg();
}