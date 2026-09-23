/***********************************FPGA->ARM->UPPER***************************************/

#include "CuBeamOne.h"
#include "network.h"
#include "fpga_init.h"
#include "upper_to_fpga.h"
#include "fpga_to_upper.h"
#include "sensor_to_upper.h"
#include "sensor_log.h"
#include "tcp_tx_ring.h"
#include "upper_to_fpga.h"

/******************************************全局变量***************************************************/
int flag = 0;           // 进中断次数，每秒输出这个标志位来间接反映ping率
int sensor_num = 0;         // 传感器的条数
unsigned int total_len = 0; // ARM发送给上位机的总字节数

char DataHead_S[4] = {'<', '<', 'S', 'T'}; // ARM与上位机通信的数据头,“<<ST”,表示上位机下发给ARM的参数设置的数据头
char DataTail_S[4] = {'E', 'D', '>', '>'}; // ARM与上位机通信的报尾“ED>>”

unsigned short send_crc_S, send_crc_tmp_S; // ARM发送给上位的机数据包进行和校验或者CRC校验的相关变量
unsigned int DDR_SONAR_DATA_SIZE = 0;          // ARM发送给上位机的声纳数据总长度+前128B

/***********************************Send-To-Upper-Data-Array***************************************/
// char *P_ddr_sonar_data = NULL;
unsigned char ddr_sonar_data[13000000];
char sensor_backup[SENSOR_DATA_SIZE];
/******************************************外部文件定义的变量********************************************/
extern int fd_uio4;
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

SEND_UPPER_SENSOR_FIRST send_to_upper_sensor;                              // ARM发送给上位机的传感器数据包头
SEND_UPPER_SENSOR_FIRST *ptr_send_to_upper_sensor = &send_to_upper_sensor; // ARM发送给上位机的传感器数据包头

/**************************************与FPGA协议相关结构体**************************************/

FPGA_DDR_FIRST fpga_ddr_frame_first;                          // 中断接收到的ddr数据帧头,结构体名
FPGA_DDR_FIRST fpga_ddr_frame_first_1;
FPGA_DDR_FIRST *ptr_fpga_frame_first = &fpga_ddr_frame_first; // 获取的FPGA-DDR数据帧头,结构体指针
FPGA_DDR_FIRST *ptr_fpga_frame_first_1 = &fpga_ddr_frame_first_1; // 获取的FPGA-DDR数据帧头,结构体指针

FPGA_DDR_SENSOR_FIRST fpga_ddr_sensor_first;
FPGA_DDR_SENSOR_FIRST *ptr_fpga_ddr_sensor_first = &fpga_ddr_sensor_first;

/******************************************函数声明***************************************************/
float PT100_Sensor_Temp(void);
void print_fpga_ddr_head(void);
static void Copy_DataTail(void);
static void print_arm_upper(void);
static void Copy_SensorData(FPGA_DDR_FIRST *fpga_frame_first_p, int *uiosensor_p);
void print_fpga_sensor_head(void);
static void Copy_IQSonarDataHead(void);
static void Copy_OriginalSonarDataHead(void);
static void send_all_package_to_upper_and_clear_sensor_num(void);
extern void my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz);

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
 * 名称：                    make_up_fpga_send_arm_package
 * 功能：                    组合填充发送给显控的参数包
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void make_up_fpga_send_arm_package(FPGA_DDR_FIRST* ptr){
    ptr_send_to_upper_sonar_first->UP_DataType = (ptr->Work_Mode >> 2);
    ptr_send_to_upper_sonar_first->UP_Work_Mode = ((ptr->Work_Mode >> 1) & 0x01);
    ptr_send_to_upper_sonar_first->UP_Work_Period = ptr->Work_Period;
    ptr_send_to_upper_sonar_first->UP_ADC_sp = ptr->ADC_sp;
    ptr_send_to_upper_sonar_first->UP_ADC_sn = ptr->ADC_sn;
    ptr_send_to_upper_sonar_first->UP_DAC_sp = ptr->DAC_sp;
    ptr_send_to_upper_sonar_first->UP_DAC_sn = ptr->DAC_sn;

    ptr_send_to_upper_sonar_first->UP_PWM_FREQUENCY = ptr->PWM_FREQUENCY;
    ptr_send_to_upper_sonar_first->UP_PWM_LFM = ptr->PWM_LFM;
    ptr_send_to_upper_sonar_first->UP_PWM_PULSE_WIDTH = (float)(ptr->PWM_PULSE_WIDTH / 100.0f); // 脉宽/
    ptr_send_to_upper_sonar_first->UP_PWM_CF = ptr->PWM_CF;
    ptr_send_to_upper_sonar_first->UP_PWM_BAND_WIDTH = ptr->PWM_BAND_WIDTH;

    ptr_send_to_upper_sonar_first->UP_ADC_sf = ptr->ADC_sf;
    ptr_send_to_upper_sonar_first->UP_RANGE = ptr->RANGE;

    ptr_send_to_upper_sonar_first->UP_PING_rate = (float)ptr->PING_rate;
    ptr_send_to_upper_sonar_first->UP_PPS_10ns = (unsigned int)((float)ptr->PPS_10ns / (FPGA_CLK_FREQUENCY / 1000)); // 时间戳单位是ns
    ptr_send_to_upper_sonar_first->UP_PPS_s = ptr->PPS_s;                                     // PPS 计数
    ptr_send_to_upper_sonar_first->UP_FrameNumber = ptr->FrameNumber;                         // 帧号--帧计数
    ptr_send_to_upper_sonar_first->UP_PWM_START = (char)(ptr->PWM_START & 0xFF);
    ptr_send_to_upper_sonar_first->UP_SAMPL_sf_IQ = ptr->SAMPL_sf_IQ;        //IQ抽样后采样次数
    ptr_send_to_upper_sonar_first->UP_SAMPL_sf_AD = ptr->SAMPL_sf_AD;      //AD抽样后采样次数车票。/
    ptr_send_to_upper_sonar_first->pps_status = (unsigned int)ptr->PWM_START >> 16;           //pps 状态
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

    ret = tcp_tx_ring_enqueue((const unsigned char *)DataHead_S,
                              ddr_sonar_data,
                              total_len);
    if (ret < 0)
    {
        printf("tcp tx enqueue failed, ret=%d total_len=%u\n", ret, total_len);
    }
    return;
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
    if (Fpga_start_mod == 1){
        Send_AllDataToUpper();
    }
    /*清空传感器条数*/
    sensor_num = 0;
}

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
                                        int *uiosensor_p
                                    ){
    //拷贝参数数据头
    memset(fpga_frame_first_p,0,sizeof(fpga_frame_first_p));
    //在UI0拷贝FPGA回复ARM的参数数据头到数据头结构体
    memcpy(fpga_frame_first_p, headuio_p, SIZE_OF_DATA_FIRST);

    memset(sensor_backup,0,SENSOR_DATA_SIZE);
    memcpy(sensor_backup,uiosensor_p,SENSOR_DATA_SIZE);
                                
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

    //组合回复给显控的参数数据包
    Copy_IQSonarDataHead();
    
    //拷贝传感器数据包
    Copy_SensorData(fpga_frame_first_p,sensor_backup);
    
    //拷贝数据尾
    Copy_DataTail();

    //发送所有的包到显控并清空传感器通道
    send_all_package_to_upper_and_clear_sensor_num();
}



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
    double timeuse;
    double AD_CPY_TIMEUSE;//测试使用

    memset(uio_baseddr_head.mem_ptr, 0, 20000);
    memset(uio_baseddr_head_1.mem_ptr, 0, 20000);
    while (1){
	    // DBG("Copy Fpga SendTo Upper ok!!!\n");
        gettimeofday(&start_time, NULL);         // 获取时间
        err = read(fd_uio4, &icount, 4);         // 读中断
        write(fd_uio4, &irq_on, sizeof(irq_on)); // 清中断
        if (err != 4){
            perror("uio irq read err\n");
        }else{
            if(Fpga_start_mod == 0){
                flag = 0;
                continue;
            }
            flag++;
            //printf ("flag = %d\n",flag);
/*
            20260717 - 修改传感器逻辑
*/
            if(flag % 2 == 0){
                make_all_send_to_upper_package(ptr_fpga_frame_first_1, 
                                                uio_baseddr_head_1.mem_ptr,
                                                uio_baseddr_iq_1.mem_ptr,
                                                uio_baseddr_original_1.mem_ptr,
                                                uio_baseddr_sensor.mem_ptr
                );
            }else if (flag % 2 == 1){
                make_all_send_to_upper_package(ptr_fpga_frame_first, 
                                                uio_baseddr_head.mem_ptr,
                                                uio_baseddr_iq.mem_ptr,
                                                uio_baseddr_original.mem_ptr,
                                                uio_baseddr_sensor_1.mem_ptr
                                        );
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

/************************************************************************************************
 *
 * 函数名称 :                SensorData_Num
 *************************************************************************************************/

void SensorData_Num(FPGA_DDR_FIRST *fpga_frame_first_p, const int *ptr_sensor, int *sensor_num)
{
    int m;
    int i;
    char *char_mems;
    int n;
    //n = 220 / cmd_package.PingRate + 5;
    for (m = 0; m <  60; m++)
    {
        if ((*(ptr_sensor + m * 256) == 0xDDDDDDDD) && (*(ptr_sensor + m * 256 + 1) == (fpga_frame_first_p->FrameNumber - 1)))
        {
            (*sensor_num)++;
        }else{
            printf("*(ptr_sensor + m * 256)     =  %x\r\n",*(ptr_sensor + m * 256));
            printf("*(ptr_sensor + m * 256 + 1) =  %d\r\n",*(ptr_sensor + m * 256 + 1));
            printf("(fpga_frame_first_p->FrameNumber - 1) = %d\r\n",(fpga_frame_first_p->FrameNumber - 1));
        }
    }
}

/************************************************************************************************
 *
 * 函数名称 :                Copy_SensorData
 * 函数描述 :                通过乒乓缓存拷贝传感器数据到DDR_SONAR_DATA数组中的指定位置
 * 函数返回值 :              无
 * 函数修改人:               fuyanshun
 * 修改备注:                 传感器内容主要是位置信息以及声速信息
 *
 *************************************************************************************************/
void Copy_SensorData(FPGA_DDR_FIRST *fpga_frame_first_p, int *uiosensor_p)
{
    DDR_SONAR_DATA_SIZE = DDR_TO_UP_HEAD + ptr_send_to_upper_sonar_first->UP_SonarDataLength; // ARM发给显控的头128B+声学数据长度

    memcpy(ptr_fpga_ddr_sensor_first, uiosensor_p, sizeof(int)*4);
    if(ptr_fpga_ddr_sensor_first->Sensor_FrameHead == 0xDDDDDDDD)
    {
        printf("sensor buff\n");
        SensorData_Num(fpga_frame_first_p, uiosensor_p, &sensor_num);
        my_copy((unsigned char *)(ddr_sonar_data + DDR_SONAR_DATA_SIZE + SIZE_OF_SENSOR_FIRST), (unsigned char *)(uiosensor_p), sensor_num * SIZE_OF_ONE_PING_SENSOR);
    }

    ptr_send_to_upper_sensor->UP_SensorData_len = SIZE_OF_SENSOR_FIRST - SIZE_OF_SENSOR + sensor_num * SIZE_OF_ONE_PING_SENSOR;
    printf("ptr_send_to_upper_sensor->UP_SensorData_len=%d\n", ptr_send_to_upper_sensor->UP_SensorData_len);
    ptr_send_to_upper_sensor->UP_SynchState = 1;
    ptr_send_to_upper_sensor->UP_MEMS_LEN = sensor_num;
    printf("sensor_num=%d\n", sensor_num);
    memcpy(ddr_sonar_data + DDR_SONAR_DATA_SIZE, ptr_send_to_upper_sensor, SIZE_OF_SENSOR_FIRST); // 先拷贝传感器总字节数大小，同步状态，预留字节，mems条数这16字节的信息
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

void print_fpga_sensor_head(void)
{
    //printf("Sensor_FrameHead=%x\n", ptr_fpga_ddr_sensor_first->Sensor_FrameHead);
    printf("Sensor_FrameNum=%d\n", ptr_fpga_ddr_sensor_first->Sensor_FrameNum);
    printf("Sensor_PPS_s=%d\n", ptr_fpga_ddr_sensor_first->Sensor_PPS_s);
    // printf("Sensor_PPS_10ns=%d\n", ptr_fpga_ddr_sensor_first->Sensor_PPS_10ns);
    // printf("fpga_PPS_s=%d\n", ptr_fpga_frame_first->PPS_s);
    // printf("fpga_PPS_10ns=%d\n", ptr_fpga_frame_first->PPS_10ns);
    // printf("fpga_up_10ns= %d\n",ptr_send_to_upper_sonar_first->UP_PPS_10ns);
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
