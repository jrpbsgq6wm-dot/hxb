/***********************************FPGA->ARM->UPPER***************************************/

#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"
#include "tcp_tx_ring.h"

#include "sensor_log.h"
#include "write_file.h"

#include "tmp451.h"
#include "pri.h"



/******************************************全局变量***************************************************/
int flag = 0;           // 进中断次数，每秒输出这个标志位来间接反映ping率
unsigned int sensor_GNSS_num = 0;         // GNSS传感器的条数
unsigned int sensor_PASHR_num = 0;         // PASHR传感器的条数
unsigned int sensor_SVS_num = 0;         // SVP传感器的条数

unsigned int total_len = 0; // ARM发送给上位机的总字节数

char DataHead_S[4] = {'<', '<', 'S', 'T'}; // ARM与上位机通信的数据头,“<<ST”,表示上位机下发给ARM的参数设置的数据头
char DataTail_S[4] = {'E', 'D', '>', '>'}; // ARM与上位机通信的报尾“ED>>”

unsigned short send_crc_S, send_crc_tmp_S; // ARM发送给上位的机数据包进行和校验或者CRC校验的相关变量
unsigned int DDR_SONAR_DATA_SIZE = 0;        // 计算传感器数据偏移 = 回复显控参数长度 + 声学数据长度

/***********************************Send-To-Upper-Data-Array***************************************/
// char *P_ddr_sonar_data = NULL;
unsigned char ddr_sonar_data[13000000];
char GNSS_backup[SENSOR_DATA_SIZE];
char PASHT_backup[SENSOR_DATA_SIZE];

/******************************************外部文件定义的变量********************************************/
extern int fd_uio2;
extern int irq_on;
extern int Fpga_start_mod;

/**************************************时间结构体-在time.h中定义******************************************/
struct timeval end_time;
struct timeval start_time;
struct timeval AD_CPY_start_time;//测试使用
struct timeval AD_CPY_end_time;//测试使用

/**************************************与上位机通信结构体**************************************/

SEND_UPPER_SONAR_FIRST send_to_upper_sonar_first;                                   // ARM发送给上位机的整体数据包头----结构体变量
SEND_UPPER_SONAR_FIRST *ptr_send_to_upper_sonar_first = &send_to_upper_sonar_first; // ARM发送给上位机的整体数据包头----结构体指针

SEND_UPPER_SENSOR_FIRST send_to_upper_sensor;                                  // ARM发送给上位机的传感器数据包头
SEND_UPPER_SENSOR_FIRST *ptr_send_to_upper_sensor = &send_to_upper_sensor; 
/**************************************与FPGA协议相关结构体**************************************/

FPGA_DDR_FIRST fpga_ddr_frame_first;                          // 中断接收到的ddr数据帧头,结构体名
FPGA_DDR_FIRST fpga_ddr_frame_first_1;
FPGA_DDR_FIRST fpga_ddr_frame_first_2;
FPGA_DDR_FIRST *ptr_fpga_frame_first = &fpga_ddr_frame_first;       // 获取的FPGA-DDR数据帧头,结构体指针
FPGA_DDR_FIRST *ptr_fpga_frame_first_1 = &fpga_ddr_frame_first_1;   // 获取的FPGA-DDR数据帧头,结构体指针
// //传感器数据
// FPGA_DDR_SENSOR_FIRST fpga_ddr_sensor_gnss;
// FPGA_DDR_SENSOR_FIRST *ptr_fpga_ddr_sensor_gnss = &fpga_ddr_sensor_gnss;
// FPGA_DDR_SENSOR_FIRST fpga_ddr_sensor_pashr;
// FPGA_DDR_SENSOR_FIRST *ptr_fpga_ddr_sensor_pashr = &fpga_ddr_sensor_pashr;
// FPGA_DDR_SENSOR_FIRST fpga_ddr_sensor_svs;
// FPGA_DDR_SENSOR_FIRST *ptr_fpga_ddr_sensor_svs = &fpga_ddr_sensor_svs;
/******************************************函数声明***************************************************/
float PT100_Sensor_Temp(void);
void print_fpga_ddr_head(void);
static void Copy_DataTail(void);
static void print_arm_upper(void);
static void Copy_SensorData( FPGA_DDR_FIRST *fpga_frame_first_p, 
                        int *uioGNSSsensor_p,
                        int *uioPASHRseneor_p,
                        int *uioSVSsensor_p);
void print_fpga_sensor_head(void);
static void Copy_IQSonarDataHead(void);
static void DDR_Head256_To_Up128(void);
static void send_all_package_to_upper_and_clear_sensor_num(void);
extern void my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz);


/********************************************************************************
 * 名称：                    Debug_pritf_fpga_datahead_256byte
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void Debug_pritf_fpga_datahead_256byte(FPGA_DDR_FIRST* ptr){
    printf("***************************\n");
    printf("fpga 256字节数据头\n");
    Debug("FrameHead=0x%08x\n", ptr->FrameHead);
    Debug("Work_Mode=%d\n", ptr->Work_Mode);
    Debug("Work_Period=%d\n", ptr->Work_Period);
    Debug("ADC_sp=%d\n", ptr->ADC_sp);
    Debug("ADC_sn=%d\n", ptr->ADC_sn);
    Debug("DAC_sp=%d\n", ptr->DAC_sp);
    Debug("DAC_sn=%d\n", ptr->DAC_sn);
    Debug("PWM_FREQUENCY=%d\n", ptr->PWM_FREQUENCY);
    Debug("PWM_LFM=%d\n", ptr->PWM_LFM);
    Debug("PWM_PULSE_WIDTH=%d\n", ptr->PWM_PULSE_WIDTH);
    Debug("PWM_CF=%d\n", ptr->PWM_CF);
    Debug("ADC_sf=%d\n", ptr->ADC_sf);
    Debug("RANGE=%d\n", ptr->RANGE);
    Debug("PWM_BAND_WIDTH=%d\n", ptr->PWM_BAND_WIDTH);
    Debug("PING_rate=%d\n", ptr->PING_rate);
    Debug("PPS_s=%d\n", ptr->PPS_s);        //pps10_ns
    Debug("PPS_10ns=%d\n", ptr->PPS_10ns);  //pps_s
    Debug("FrameNumber=%d\n", ptr->FrameNumber);
    Debug("PWM_START=%d\n", ptr->PWM_START);
    Debug("SAMPL_sf_IQ=%d\n", ptr->SAMPL_sf_IQ);
    Debug("SAMPL_sf_AD=%d\n", ptr->SAMPL_sf_AD);
    Debug("SmallFrameHead=0x%08x\n", ptr->SmallFrameHead);
    printf("***************************\n");
}

/********************************************************************************
 * 名称：                    make_up_fpga_send_arm_package
 * 功能：                    组合填充发送给显控的参数包
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void make_up_fpga_send_arm_package(FPGA_DDR_FIRST* ptr){
    ptr_send_to_upper_sonar_first->Probe_mode = cmd_package.Probe_mode;
    ptr_send_to_upper_sonar_first->Probe_Logo = cmd_package.Probe_Logo;
    ptr_send_to_upper_sonar_first->Install_angle = cmd_package.Install_angle;
    ptr_send_to_upper_sonar_first->Ping_mode = cmd_package.Ping_mode;
    ptr_send_to_upper_sonar_first->INS_mode = cmd_package.INS_mode;
    ptr_send_to_upper_sonar_first->UP_DataType = cmd_package.DataType;
    ptr_send_to_upper_sonar_first->UP_Work_Mode = cmd_package.WorkMode;
    ptr_send_to_upper_sonar_first->UP_PWM_FREQUENCY = ptr->PWM_FREQUENCY;
    ptr_send_to_upper_sonar_first->UP_PWM_LFM = ptr->PWM_LFM;
    /*脉宽*/
    ptr_send_to_upper_sonar_first->UP_PWM_PULSE_WIDTH = ptr->PWM_PULSE_WIDTH / ((FPGA_CLK_FREQUENCY / 1000000.0f));
    ptr_send_to_upper_sonar_first->UP_PWM_CF = ptr->PWM_CF;
    ptr_send_to_upper_sonar_first->UP_ADC_sf = ptr->ADC_sf;
    ptr_send_to_upper_sonar_first->UP_RANGE = ptr->RANGE;
    ptr_send_to_upper_sonar_first->UP_PWM_BAND_WIDTH = ptr->PWM_BAND_WIDTH;
    /*ping率*/
    ptr_send_to_upper_sonar_first->UP_PING_rate = cmd_package.PingRate;
    ptr_send_to_upper_sonar_first->UP_PPS_10ns = ptr->PPS_s ; /* /(FPGA_CLK_FREQUENCY/1000);*/
    ptr_send_to_upper_sonar_first->UP_PPS_s = ptr->PPS_10ns;
    ptr_send_to_upper_sonar_first->UP_FrameNumber = ptr->FrameNumber;
    //printf_time(" ptr->FrameNumber %d\n", ptr->FrameNumber);
    /*读取温度*/
    ptr_send_to_upper_sonar_first->UP_PT100 = 35;
    if(cmd_package.DataType == 1){
        ptr_send_to_upper_sonar_first->UP_SAMPL_sf_AD = ptr->SAMPL_sf_IQ;
    }else{
        ptr_send_to_upper_sonar_first->UP_SAMPL_sf_AD = ptr->SAMPL_sf_AD;
    }
    /*温度*/
    ptr_send_to_upper_sonar_first->UP_TMP451 = local_temp;
}

/********************************************************************************
 * 名称：                    Debug_pri_sendto_upper_package
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void Debug_pri_sendto_upper_package(SEND_UPPER_SONAR_FIRST *ptr){
    printf("***************************\n");
    // printf("发送给显控的参数数据包\n");
    Debug("Probe_mode=%d\n", ptr->Probe_mode);
    Debug("Probe_Logo=%d\n", ptr->Probe_Logo);
    // Debug("Install_angle=%f\n", ptr->Install_angle);
    Debug("Ping_mode=%d\n", ptr->Ping_mode);
    // Debug("INS_mode=%d\n", ptr->INS_mode);
    // Debug("UP_DataType=%d\n", ptr->UP_DataType);
    // Debug("UP_Work_Mode=%d\n", ptr->UP_Work_Mode);
    // Debug("UP_PWM_FREQUENCY=%d\n", ptr->UP_PWM_FREQUENCY);
    // Debug("UP_PWM_LFM=%d\n", ptr->UP_PWM_LFM);
    // Debug("UP_PWM_PULSE_WIDTH=%f\n", ptr->UP_PWM_PULSE_WIDTH);
    // Debug("UP_PWM_CF=%d\n", ptr->UP_PWM_CF);
    // Debug("UP_ADC_sf=%d\n", ptr->UP_ADC_sf);
    // Debug("UP_PWM_START=%d\n", ptr->UP_PWM_START);
    // Debug("UP_PWM_BAND_WIDTH=%d\n", ptr->UP_PWM_BAND_WIDTH);
    // Debug("UP_PING_rate=%f\n", ptr->UP_PING_rate);
    // Debug("UP_PPS_10ns=%d\n", ptr->UP_PPS_10ns);
    // Debug("UP_PPS_s=%d\n", ptr->UP_PPS_s);
    Debug("UP_FrameNumber=%d\n", ptr->UP_FrameNumber);
    // Debug("UP_PT100=%f\n", ptr->UP_PT100);
    // Debug("UP_SAMPL_sf_AD=%d\n", ptr->UP_SAMPL_sf_AD);
    // Debug("UP_TMP451=%f\n", ptr->UP_TMP451);
    /*打印发送给显控的声学数据长度*/
    Debug("声学数据总长度:UP_SonarDataLength=%d\n", ptr->UP_SonarDataLength);
    printf("***************************\n");
}

/********************************************************************************
 * 名称：                    Debug_pri_sendto_upper_sensor_package
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void Debug_pri_sendto_upper_sensor_package(SEND_UPPER_SENSOR_FIRST *ptr){
    printf("***************************\n");
    printf("打印传感器数据包前2字段\n");
    /*打印发送给显控的声学数据长度*/
    printf("UP_SensorData_len=%d\n", ptr->UP_SensorData_len);
    printf("UP_SynchState=%d\n", ptr->UP_SynchState);
    printf("***************************\n");
}


#if 0

#else
/********************************************************************************
 * 名称：                    make_all_send_to_upper_package
 *                          组合所有数据包发送给显控
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void make_all_send_to_upper_package(
                                        FPGA_DDR_FIRST * fpga_frame_first_p, 
                                        int *headuio_p, 
                                        int *uioiq_p,
                                        int *uioad_p,
                                        int *uiosensor_p,
                                        int *uiopashr_p,
                                        int *uiosvs_p
                                    ){
//备份传感器数据  gnss pashr 50*256
    // static int record_count = 0;
    // int result;

    //拷贝参数数据头
    memset(fpga_frame_first_p,0,sizeof(fpga_frame_first_p));
    //在UI0拷贝FPGA回复ARM的参数数据头到数据头结构体
    memcpy(fpga_frame_first_p, headuio_p, SIZE_OF_DATA_FIRST);

// /*清空缓冲区*/
    memset(GNSS_backup,0,sizeof(GNSS_backup));
    memset(PASHT_backup,0,sizeof(PASHT_backup));

    /*拷贝传感器数据*/
    memcpy(GNSS_backup,uiosensor_p,sizeof(GNSS_backup));
    memcpy(PASHT_backup,uiopashr_p,sizeof(PASHT_backup));

    //========== 异步写入数据（替换原来的文件写入代码） ==========
    // record_count++;
    // result = push_sensor_data_to_queue(GNSS_backup, PASHT_backup,fpga_frame_first_p->FrameNumber, record_count);

    // if (result == 1) {
    //     // 数据已入队，由后台线程处理
    //     // printf_time("Data queued for writing (record %d)\n", record_count);
    // } else if (result == -1) {
    //     printf_time("Queue full, dropped record %d\n", record_count);
    // } else if (result == 0) {
    //     printf_time("Max records reached, stopping data collection\n");
    // } else if (result == -2) {
    //     printf_time("Queue not initialized yet, dropped record %d\n", record_count);
    // }

//打印在UIO取到的ddr参数头
    // Debug_pritf_fpga_datahead_256byte(fpga_frame_first_p);

    //将接收到的数据填充给组合到显控的结构体
    make_up_fpga_send_arm_package(fpga_frame_first_p);
    //按照声学数据格式拷贝声学数据
    if ((fpga_frame_first_p->FrameHead == 0xAAAAAAAA) && (fpga_frame_first_p->SmallFrameHead == 0xBBBBBBBB)){       /*iq数据*/
        /*计算IQ数据的声纳数据长度*/
        ptr_send_to_upper_sonar_first->UP_SonarDataLength = (((unsigned int)(fpga_frame_first_p->ADC_sn / SAMPLE_FACTOR)-1) * SIZE_OF_CHANNEL_NUM * 4);
        my_copy((unsigned char *)(ddr_sonar_data + DDR_TO_UP_HEAD), (unsigned char *)(uioiq_p), ptr_send_to_upper_sonar_first->UP_SonarDataLength); 
    }else if((fpga_frame_first_p->FrameHead == 0xAAAAAAAA) && (fpga_frame_first_p->SmallFrameHead == 0xCCCCCCCC)){  /*原始数据*/
        /*计算原始AD数据的声纳数据长度*/
        ptr_send_to_upper_sonar_first->UP_SonarDataLength = ((unsigned int)(fpga_frame_first_p->ADC_sn/AD_SAMPLE_FACTOR) * SIZE_OF_CHANNEL_NUM * 2); 
        my_copy((unsigned char *)(ddr_sonar_data + DDR_TO_UP_HEAD), (unsigned char *)(uioad_p), ptr_send_to_upper_sonar_first->UP_SonarDataLength);
    }else{
        perror("FrameNumber ERROR");
        return ;
    }
    

/*打印发送给显控的参数部分*/
    //Debug_pri_sendto_upper_package(ptr_send_to_upper_sonar_first);

    //组合回复给显控的参数数据包
    Copy_IQSonarDataHead();

    //拷贝传感器数据包
    Copy_SensorData(fpga_frame_first_p,GNSS_backup,PASHT_backup,uiosvs_p);

    //拷贝数据尾
    Copy_DataTail();

/*打印发送给显控的传感器数据包部分*/
    // Debug_pri_sendto_upper_sensor_package(ptr_send_to_upper_sensor);

    //发送所有的包到显控并清空传感器通道
    send_all_package_to_upper_and_clear_sensor_num();
}
#endif                 

/********************************************************************************
 * 名称：                    read_fpga_send_to_upper
 * 功能：                    FPGA--->ARM--->上位机主线程
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void Copy_Fpga_SendTo_Upper(void)
{
    unsigned icount;
    int err;

    memset(uio_baseddr_head.mem_ptr, 0, uio_baseddr_head.mem_size);
    memset(uio_baseddr_head_1.mem_ptr, 0, uio_baseddr_head_1.mem_size);
    memset(uio_baseddr_ad.mem_ptr, 0, uio_baseddr_head.mem_size);
    memset(uio_baseddr_ad_1.mem_ptr, 0, uio_baseddr_head_1.mem_size);
    memset(uio_baseddr_iq.mem_ptr, 0, uio_baseddr_head.mem_size);
    memset(uio_baseddr_iq_1.mem_ptr, 0, uio_baseddr_head_1.mem_size);
    memset(uio_baseddr_sensor.mem_ptr, 0, uio_baseddr_head.mem_size);
    memset(uio_baseddr_sensor_1.mem_ptr, 0, uio_baseddr_head_1.mem_size);
    memset(uio_baseddr_pashr.mem_ptr, 0, uio_baseddr_head.mem_size);
    memset(uio_baseddr_pashr_1.mem_ptr, 0, uio_baseddr_head_1.mem_size);
    memset(uio_baseddr_svs.mem_ptr, 0, uio_baseddr_head.mem_size);
    memset(uio_baseddr_svs_1.mem_ptr, 0, uio_baseddr_head_1.mem_size);


    while (1)
    {
        err = read(fd_uio2, &icount, 4);         // 读中断
        write(fd_uio2, &irq_on, sizeof(irq_on)); // 清中断
        if (err != 4){
            perror("uio irq read err\n");
        }else{ 
            if(Fpga_start_mod == 0){
                flag = 0;
                continue;
            }
            flag++;
            // printf_time("flag %d\n",flag);
            if(flag % 2 == 0){
                //printf_time("0x33000000 start\n");
                make_all_send_to_upper_package(ptr_fpga_frame_first_1, 
                                                uio_baseddr_head_1.mem_ptr,
                                                uio_baseddr_iq_1.mem_ptr,
                                                uio_baseddr_ad_1.mem_ptr,
                                                uio_baseddr_sensor.mem_ptr,
                                                uio_baseddr_pashr.mem_ptr,
                                                uio_baseddr_svs.mem_ptr
                );
                //printf_time("0x33000000 over\n\n");
            }else if (flag % 2 == 1){
                //printf_time("0x34000000 start\n");
                make_all_send_to_upper_package(ptr_fpga_frame_first, 
                                                uio_baseddr_head.mem_ptr,
                                                uio_baseddr_iq.mem_ptr,
                                                uio_baseddr_ad.mem_ptr,
                                                uio_baseddr_sensor_1.mem_ptr,
                                                uio_baseddr_pashr_1.mem_ptr,
                                                uio_baseddr_svs_1.mem_ptr
                                        );
                //printf_time("0x33000000 over\n\n");
            }else{
                ;
            }
        }
    }
}

/************************************************************************************************
 *
 * 函数名称 :                PT100_Sensor_Temp()
 * 函数描述 :                获取温度传感器温度
 * 函数返回值 :              温度数值--->float数据类型
 * 函数修改人:               fuyanshun
 * 修改备注:                 无
 *
 *************************************************************************************************/
float PT100_Sensor_Temp(void) {
    DBG("Sensor Temp  ok!!!\n");
    
    float Rt;
    float sound;
    float temp_up;
    float Pt100_Temp = 0.0;
    char temp_data[TEMP_DATA_SIZE];
    int fd = open(PT100_SYS_PATH_TEMP_VALUE, O_RDWR);
    if (fd < 0) {
        printf("Failed to open file.\n");
        return -1.0;
    }

    while (1) {
        // usleep(1000);
        lseek(fd, 0, SEEK_SET);
        if (read(fd, temp_data, TEMP_DATA_SIZE) < TEMP_DATA_SIZE) {
            //printf("Failed to read data.\n");
            ptr_send_to_upper_sonar_first->UP_PT100 = 0;
            perror("PT100 ERROR");
            break;
        }
        temp_data[TEMP_DATA_SIZE] = '\0';
        temp_up = atof(temp_data);
	    Rt = ((temp_up/4096 + 3)/68*1000)/0.5;      //当前温度下的阻止
        Pt100_Temp = ((-(A))+sqrt(pow(A,2)-4*(B)*(1-Rt/100)))/(2*(B));//温度计算公式
        // printf("temp is %f cent\n", Pt100_Temp);
        // sound = coef2_1*(pow(Pt100_Temp,2)) + coef2_2*(Pt100_Temp) + coef2_3;//声速公式
        ptr_send_to_upper_sonar_first->UP_PT100 = Pt100_Temp;
        // printf("temp is %f cent\n", Pt100_Temp);
        lseek(fd, 0, SEEK_SET);
    }
    close(fd);
    return Pt100_Temp;
}

/********************************************************************************
 * 名称：                    Copy_DataTail
 * 功能：                    拷贝报尾ED>>四个字节到ddr_sonar_data数组的末尾
 * 入口参数：            	 若干
 * 出口参数：            	 无
 *********************************************************************************/
void Copy_DataTail(void)
{
    /*
        数据尾位置偏移地址 = 
            发送显控参数总长度
            + 声学数据长度
            + 传感器数据总长度占用4byte
            + 传感器数据长度
            + crc占用2byte
    */
    memcpy(ddr_sonar_data + DDR_SONAR_DATA_SIZE + SIZE_OF_SENSOR +  ptr_send_to_upper_sensor->UP_SensorData_len + SIZE_OF_USHORT, DataTail_S, SIZE_OF_LONG);
}

void print_mapped_memory(int *addr, int count) {
    if (addr == NULL) {
        printf("错误：映射地址为空！\n");
        return;
    }
    printf("===== 映射地址 %p 的内存值 =====\n", addr);
    // 打印count个int大小的内存值（可根据需要改为char/unsigned int等类型）
    for (int i = 0; i < count; i++) {
        // 打印：偏移量 | 绝对地址 | 对应的值
        printf("偏移 %d: 地址 %p → 值 = 0x%08X\n",   i, addr + i, *(addr + i));
    }
}


/************************************************************************************************
 *
 * 函数名称 :                SensorData_Num
 * 函数描述 :                计算传感器数据条数
 * 函数返回值 :              无，计算的值给了一个全局变量 sensor_num
 * 函数修改人:               fuyanshun
 * 修改备注:                fpga_frame_first_p 数据头指针，ptr_sensor uio sensor数据地址
 *
 *************************************************************************************************/
// sensor_data.c - 精简版（不打印丢弃信息）
#include "pri.h"
#include <stdio.h>
#include <string.h>

void SensorData_Num(FPGA_DDR_FIRST *fpga_frame_first_p, const int *ptr_sensor, int *sensor_num)
{
    int m,n;
    
    /*
        记录当前的ping率设置和上一次ping率设置
        如果相同则不做改变
            n = 110 / now_ping_rate + 5;
        如果不同则取传感器数据按照上一次ping率取
            n = 110 / last_ping_rate + 5;
    */
    //n = 110 / cmd_package.PingRate + 5;
    unsigned int sensordata_count = 0;
    unsigned int pps_count = 0;
    for (m = 0; m < 60; m++) {
        const int *current_ptr = ptr_sensor + m * 64;
        const int *FrameNumber_ptr = current_ptr + 1;
        const int *PPS_C = FrameNumber_ptr + 1;
        // printf("IQ帧=%u, 传感器帧=%u, 地址=0x%X\n",fpga_frame_first_p->FrameNumber,*FrameNumber_ptr,(unsigned int)current_ptr);
        if ((*current_ptr == 0xDDDDDDDD)) {
            if (cmd_package.Probe_mode == 1) {  //双探头
                if(cmd_package.Ping_mode == 0){ //交替
                    if ((fpga_frame_first_p->FrameNumber-2) == (*FrameNumber_ptr)) {
                        if((*PPS_C) - pps_count <= 1 || (pps_count == 0)){
                            (*sensor_num)++;
                            pps_count = *PPS_C;
                        }
                    }
                }else if(cmd_package.Ping_mode){ //全ping
                    if ((fpga_frame_first_p->FrameNumber-1) == (*FrameNumber_ptr)) {
                        if((*PPS_C) - pps_count <= 1 || (pps_count == 0)){
                            (*sensor_num)++;
                            pps_count = *PPS_C;
                        }
                    }
                }
            } else if (cmd_package.Probe_mode == 0) {   //单探头
                if ((fpga_frame_first_p->FrameNumber-1) == (*FrameNumber_ptr)) {

                    if((*PPS_C) - pps_count <= 1 || (pps_count == 0)){
                        (*sensor_num)++;
                        pps_count = *PPS_C;
                    }
                    // 组装消息并放入队列
                    // snprintf(msg, MAX_MSG_SIZE, 
                    //          "IQ帧=%u, 传感器帧=%u, 地址=0x%X - %u\n",
                    //          fpga_frame_first_p->FrameNumber,
                    //          *FrameNumber_ptr,
                    //          (unsigned int)current_ptr,sensordata_count++);
                    
                    // queue_push_nonblock(msg);  // 非阻塞放入
                    // memset(msg,0,sizeof(msg));
                }
            }
        }
    }
}

/************************************************************************************************
 *
 * 函数名称 :                cal_sensor_count_or_copy_sensor_upperbuf
 * 函数描述 :                计算传感器条数并拷贝数据到发送给显控的buf中
 * 函数返回值 :              无
 * 函数修改人:               
                            fpga_ddr_sensor_p   用来存放fpga ddr中的传感器数据结构体
                            uio_sensor_p        用来计算和拷贝的传感器数据
                            offset_len          偏移长度
                            sensor_num          传感器数据计数
 *************************************************************************************************/
void cal_sensor_count_or_copy_sensor_upperbuf(
                                            FPGA_DDR_FIRST *fpga_frame_first_p, 
                                            unsigned int *uio_sensor_p,
                                            unsigned int *offset_len,
                                            unsigned int *sensor_num
                                        ){
    //计算传感器总条数
    SensorData_Num(fpga_frame_first_p, uio_sensor_p,sensor_num);
    //判断传感器数据条数)
    if(sensor_num > 0){
        //传感器数据条数 DDR偏移 =  回复显控参数长度 + 声学数据长度 + （传感器数据总长度4byte + 传感器数据.同步状态4byte + 传感器数据.预留字节4byte = sizeof(ptr_send_to_upper_sensor->UP_SensorData_len)）
        my_copy((unsigned char *)(ddr_sonar_data + DDR_SONAR_DATA_SIZE +  sizeof(ptr_send_to_upper_sensor->UP_SensorData_len)) + (*offset_len), 
                (unsigned char *)sensor_num, 
                4);
        (*offset_len) += 4;
        //传感器数据 DDR偏移 =  回复显控参数长度 + 声学数据长度 + 传感器数据总长度4byte + 传感器数据.同步状态4byte + 传感器数据.预留字节4byte = sizeof(ptr_send_to_upper_sensor->UP_SensorData_len) + 传感器数据.GNSS数据条数4byte
        my_copy((unsigned char *)(ddr_sonar_data + DDR_SONAR_DATA_SIZE + sizeof(ptr_send_to_upper_sensor->UP_SensorData_len)) + (*offset_len), 
                (unsigned char *)(uio_sensor_p), 
                (*sensor_num) * SIZE_OF_ONE_PING_SENSOR);
        //更新传感器数据偏移
        (*offset_len) +=  (*sensor_num)  * SIZE_OF_ONE_PING_SENSOR;
    }else{
        /*没有传感器数据 sensor_num=0*/
        //传感器数据条数 DDR偏移 =  回复显控参数长度 + 声学数据长度 + 传感器数据总长度4byte + 传感器数据.同步状态4byte + 传感器数据.预留字节4bytesizeof=(ptr_send_to_upper_sensor->UP_SensorData_len)
        //116 + 511680 + 4 + 4 + 4 + 0
        my_copy((unsigned char *)(ddr_sonar_data + DDR_SONAR_DATA_SIZE + sizeof(ptr_send_to_upper_sensor->UP_SensorData_len)) + (*offset_len), 
                (unsigned char *)sensor_num, 
                4);
        (*offset_len) += 4;
    }
}


/************************************************************************************************
 *
 * 函数名称 :                Copy_SensorData
 * 函数描述 :                拷贝传感器数据到DDR_SONAR_DATA数组中的指定位置
 * 函数返回值 :              无
 * 函数修改人:               fuyanshun
 * 修改备注:                参数：fpga_frame_first_p -> fpga数据头结构体指针
 *                                uioGNSSsensor_p,  GNSS
                                  uioPASHRseneor_p, PASHR
                                  uioSVSsensor_p    SVS
 *************************************************************************************************/
void Copy_SensorData(
                        FPGA_DDR_FIRST *fpga_frame_first_p, 
                        int *uioGNSSsensor_p,
                        int *uioPASHRseneor_p,
                        int *uioSVSsensor_p
                    )
{
    //暂存传感器数据总长度
    unsigned int sensor_data_len=8;

    /*计算传感器数据总长度字段偏移 = 回复显控参数长度 + 声学数据长度 */
    DDR_SONAR_DATA_SIZE = DDR_TO_UP_HEAD + ptr_send_to_upper_sonar_first->UP_SonarDataLength;

#if 0
#endif
    /* GNSS字节长度 + GNSS */
    cal_sensor_count_or_copy_sensor_upperbuf(fpga_frame_first_p,uioGNSSsensor_p,&sensor_data_len,&sensor_GNSS_num);
    /* 4byte预留*/
    memset(ddr_sonar_data + DDR_SONAR_DATA_SIZE +  sizeof(ptr_send_to_upper_sensor->UP_SensorData_len) + sensor_data_len, 0 , 4);
    sensor_data_len += 4;

    /* PASHR字节长度 + PASHR */
#if 0
#endif
    cal_sensor_count_or_copy_sensor_upperbuf(fpga_frame_first_p,uioPASHRseneor_p,&sensor_data_len,&sensor_PASHR_num);
    /*姿态暂无 4byte条数长度+0字节内容*/
    memset(ddr_sonar_data + DDR_SONAR_DATA_SIZE +  sizeof(ptr_send_to_upper_sensor->UP_SensorData_len) + sensor_data_len, 0 , 4);
    sensor_data_len += 4;

#if 0
#endif
    /* 4SVS字节长度 + SVS */
    cal_sensor_count_or_copy_sensor_upperbuf(fpga_frame_first_p,uioSVSsensor_p,&sensor_data_len,&sensor_SVS_num);

    //printf("COPY SUCCESSFULLY\n");
    /*填充传感器数据结构体*/
    ptr_send_to_upper_sensor->UP_SensorData_len = sensor_data_len;
    ptr_send_to_upper_sensor->UP_SynchState = 1;
    ptr_send_to_upper_sensor->UP_Sensor_RSV[0] = 0;
    ptr_send_to_upper_sensor->UP_Sensor_RSV[1] = 0;
    ptr_send_to_upper_sensor->UP_Sensor_RSV[2] = 0;
    ptr_send_to_upper_sensor->UP_Sensor_RSV[3] = 0;
    memcpy(ddr_sonar_data + DDR_SONAR_DATA_SIZE, ptr_send_to_upper_sensor, sizeof(SEND_UPPER_SENSOR_FIRST));

}


/********************************************************************************
 * 名称：                    checksum
 * 功能：                    和校验         //目前没有用到
 * 入口参数：            	 char *buf, unsigned int nword
 * 出口参数：            	 无
 *********************************************************************************/
unsigned short checksum(char *buf, unsigned int nword)
{
    unsigned long sum;
    for (sum = 0; nword > 0; nword--)
        sum += *buf++;                  /*获取buf的累加和*/
    sum = (sum >> 16) + (sum & 0xffff); /*获取sum高16位与低16位的和*/
    sum += (sum >> 16);
    return ~sum;
}

// 分包发送，每次8K，总时间≈原来一次send，永不暴卡！
static int send_chunk_8k(int fd, char *data, int total_len)
{
    int offset = 0;
    const int CHUNK = 16*1024;  // 8K

    while (offset < total_len)
    {
        int remain = total_len - offset;
        int send_size = remain > CHUNK ? CHUNK : remain;

        ssize_t ret = send(fd, data + offset, send_size, MSG_NOSIGNAL);
        if (ret <= 0)
        {
            return -1;
        }
        offset += ret;
    }
    return offset;
}

/********************************************************************************
 * 名称：                    Send_AllDataToUpper
 * 功能：                    ARM向上位机发送声呐数据和传感器数据等总包
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void Send_AllDataToUpper(void)
{
    int ret;

    /*
     * 这里不再直接send大包，避免TCP阻塞拖住UIO中断处理线程。
     * 当前线程只把已经组好的ddr_sonar_data复制到TCP发送环形队列，
     * 然后尽快返回继续等待下一次PL中断。
     *
     * TCP发送线程最终发出的字节流仍保持原协议格式：
     *   DataHead_S(4) + total_len(4) + ddr_sonar_data(total_len)
     */
    ret = tcp_tx_ring_enqueue((const unsigned char *)DataHead_S,
                              ddr_sonar_data,
                              total_len);
    if (ret < 0)
    {
        printf("tcp tx enqueue failed, ret=%d total_len=%u\n", ret, total_len);
    }
    return;

#if 0
    //printf_time("6\n");
    //  发送报头4字节
    int send_upper;
    if ((send_upper = send_chunk_8k(Connect_fd, DataHead_S, 4)) < 0)
    {
        printf("send_upper_head error = %d\n", send_upper);
        error_process();
        return;
    }
    //printf_time("7\n");
    // 发送4字节总长度
    if ((send_upper = send_chunk_8k(Connect_fd, (char *)&total_len, 4)) < 0)
    {
        printf("send_upper total len error = %d\n", send_upper);
        error_process();
        return;
    }
    //printf_time("8\n");
    // 发送声呐数据前参数+声纳数据+传感器前16+传感器数据+crc+报尾
    if ((send_upper = send_chunk_8k(Connect_fd, ddr_sonar_data, total_len)) < 0)
    {
        printf("send_upper sonar data error = %d\n", send_upper);
        error_process();
        return;
    }
    //printf_time("9\n");
    // printf("send_upper sonar data ok = %d\n", send_upper);
#endif
}




/********************************************************************************
 * 名称：                    send_all_package_to_upper_and_clear_sensor_num
 * 功能：                    ARM向上位机发送整个数据包，其中包括声呐数据和传感器数据等所有，然后清传感器条数
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void send_all_package_to_upper_and_clear_sensor_num(void)
{
    /*
        发送给显控包总长度 = 
            显控参数总长度 116
            + 声学数据长度 511680
            + 传感器数据总长度占用4byte
            + 传感器数据长度 28
            + crc占用2byte 
            + 4byte脚
    */
    total_len = DDR_SONAR_DATA_SIZE + SIZE_OF_SENSOR + ptr_send_to_upper_sensor->UP_SensorData_len + SIZE_OF_USHORT + SIZE_OF_LONG;
    // printf_time("   input queue :%d\n",total_len);
    if (Fpga_start_mod == 1){
        Send_AllDataToUpper();
    }
    /*清空传感器条数*/
    sensor_GNSS_num = 0;         // GNSS传感器的条数
    sensor_PASHR_num = 0;         // PASHR传感器的条数
    sensor_SVS_num = 0;         // SVP传感器的条数
}

/********************************************************************************
 * 名称：                    Copy_IQSonarDataHead
 * 功能：                    计算IQ数据的声呐数据长度，并组合回复给显控的参数数据包
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void Copy_IQSonarDataHead(void)
{
    memcpy(ddr_sonar_data, ptr_send_to_upper_sonar_first, DDR_TO_UP_HEAD);
}

/********************************************************************************
 * 名称：                    Copy_OriginalSonarDataHead
 * 功能：                    计算原始数据的声呐数据长度，并组合回复给显控的参数数据包
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void Copy_OriginalSonarDataHead(void)
{
    memcpy(ddr_sonar_data, ptr_send_to_upper_sonar_first, DDR_TO_UP_HEAD);                                              // 拷贝原始数据前参数数据
}

/************************************************************************************************
 *
 * 函数名称 :            print_arm_upper(void)
 * 函数描述 :            打印arm->upper的数据信息
 * 函数返回值 :          无
 * 函数修改人:           fuyanshun
 *
 *************************************************************************************************/
void print_arm_upper(void)
{
#if 0
    printf("ptr_send_to_upper_sonar_first->UP_DataType=%d\n", ptr_send_to_upper_sonar_first->UP_DataType);
    printf("ptr_send_to_upper_sonar_first->UP_Work_Mode=%d\n", ptr_send_to_upper_sonar_first->UP_Work_Mode);
    printf("ptr_send_to_upper_sonar_first->UP_Work_Period=%d\n", ptr_send_to_upper_sonar_first->UP_Work_Period);
    printf("ptr_send_to_upper_sonar_first->UP_ADC_sp=%d\n", ptr_send_to_upper_sonar_first->UP_ADC_sp);
    printf("ptr_send_to_upper_sonar_first->UP_ADC_sn=%d\n", ptr_send_to_upper_sonar_first->UP_ADC_sn);
    printf("ptr_send_to_upper_sonar_first->UP_DAC_sp=%d\n", ptr_send_to_upper_sonar_first->UP_DAC_sp);
    printf("ptr_send_to_upper_sonar_first->UP_DAC_sn=%d\n", ptr_send_to_upper_sonar_first->UP_DAC_sn);
    printf("ptr_send_to_upper_sonar_first->UP_PWM_FREQUENCY=%d\n", ptr_send_to_upper_sonar_first->UP_PWM_FREQUENCY);
    printf("ptr_send_to_upper_sonar_first->UP_PWM_LFM=%d\n", ptr_send_to_upper_sonar_first->UP_PWM_LFM);
    printf("ptr_send_to_upper_sonar_first->UP_PWM_PULSE_WIDTH=%f\n", ptr_send_to_upper_sonar_first->UP_PWM_PULSE_WIDTH);
    printf("ptr_send_to_upper_sonar_first->UP_PWM_CF=%d\n", ptr_send_to_upper_sonar_first->UP_PWM_CF);
    printf("ptr_send_to_upper_sonar_first->UP_ADC_sf=%d\n", ptr_send_to_upper_sonar_first->UP_ADC_sf);
    printf("ptr_send_to_upper_sonar_first->UP_RANGE=%d\n", ptr_send_to_upper_sonar_first->UP_RANGE);
    printf("ptr_send_to_upper_sonar_first->UP_PWM_BAND_WIDTH=%d\n", ptr_send_to_upper_sonar_first->UP_PWM_BAND_WIDTH);
    printf("ptr_send_to_upper_sonar_first->UP_PING_rate=%f\n", ptr_send_to_upper_sonar_first->UP_PING_rate);
    printf("ptr_send_to_upper_sonar_first->UP_PPS_10ns=%d\n", ptr_send_to_upper_sonar_first->UP_PPS_10ns);
    printf("ptr_send_to_upper_sonar_first->UP_PPS_s=%d\n", ptr_send_to_upper_sonar_first->UP_PPS_s);
    printf("ptr_send_to_upper_sonar_first->UP_FrameNumber=%d\n", ptr_send_to_upper_sonar_first->UP_FrameNumber);
    printf("ptr_send_to_upper_sonar_first->UP_PT100=%f\n", ptr_send_to_upper_sonar_first->UP_PT100);
    printf("ptr_send_to_upper_sonar_first->UP_PWM_START=%f\n", ptr_send_to_upper_sonar_first->UP_PWM_START);
    printf("ptr_send_to_upper_sonar_first->UP_SAMPL_sf_IQ=%d\n", ptr_send_to_upper_sonar_first->UP_SAMPL_sf_IQ);
    printf("ptr_send_to_upper_sonar_first->UP_SAMPL_sf_AD=%d\n", ptr_send_to_upper_sonar_first->UP_SAMPL_sf_AD);
    printf("ptr_send_to_upper_sonar_first->UP_SonarDataLength=%d\n", ptr_send_to_upper_sonar_first->UP_SonarDataLength);
#endif
}

/************************************************************************************************
 *
 * 函数名称 :            print_fpga_ddr_head(void)
 * 函数描述 :            打印fpga_ddr头数据256字节
 * 函数返回值 :          无
 * 函数修改人:           fuyanshuan
 *
 *************************************************************************************************/
void print_fpga_ddr_head(void)
{
    printf("ptr_fpga_frame_first->FrameHead=%x\n", ptr_fpga_frame_first->FrameHead);
    printf("ptr_fpga_frame_first->Work_Mode=%d\n", ptr_fpga_frame_first->Work_Mode);
    printf("ptr_fpga_frame_first->Work_Period=%d\n", ptr_fpga_frame_first->Work_Period);
    printf("ptr_fpga_frame_first->ADC_sp=%d\n", ptr_fpga_frame_first->ADC_sp);
    printf("ptr_fpga_frame_first->ADC_sn=%d\n", ptr_fpga_frame_first->ADC_sn);
    printf("ptr_fpga_frame_first->DAC_sp=%d\n", ptr_fpga_frame_first->DAC_sp);
    printf("ptr_fpga_frame_first->DAC_sn=%d\n", ptr_fpga_frame_first->DAC_sn);
    printf("ptr_fpga_frame_first->PWM_FREQUENCY=%d\n", ptr_fpga_frame_first->PWM_FREQUENCY);
    printf("ptr_fpga_frame_first->PWM_LFM=%d\n", ptr_fpga_frame_first->PWM_LFM);
    printf("ptr_fpga_frame_first->PWM_PULSE_WIDTH=%d\n", ptr_fpga_frame_first->PWM_PULSE_WIDTH);
    printf("ptr_fpga_frame_first->PWM_CF=%d\n", ptr_fpga_frame_first->PWM_CF);
    printf("ptr_fpga_frame_first->ADC_sf=%d\n", ptr_fpga_frame_first->ADC_sf);
    printf("ptr_fpga_frame_first->RANGE=%d\n", ptr_fpga_frame_first->RANGE);
    printf("ptr_fpga_frame_first->PING_rate=%d\n", ptr_fpga_frame_first->PING_rate);
    printf("ptr_fpga_frame_first->PPS_10ns=%d\n", ptr_fpga_frame_first->PPS_10ns);
    printf("ptr_fpga_frame_first->PPS_s=%d\n", ptr_fpga_frame_first->PPS_s);
    printf("ptr_fpga_frame_first->FrameNumber=%d\n", ptr_fpga_frame_first->FrameNumber);
    printf("ptr_fpga_frame_first->PWM_START=%d\n", ptr_fpga_frame_first->PWM_START);
    printf("ptr_fpga_frame_first->SAMPL_sf_IQ=%d\n", ptr_fpga_frame_first->SAMPL_sf_IQ);
    printf("ptr_fpga_frame_first->SmallFrameHead=%x\n", ptr_fpga_frame_first->SmallFrameHead);
    printf("ptr_send_to_upper_sonar_first->UP_PT100=%f\n", ptr_send_to_upper_sonar_first->UP_PT100);
}
