#include "beam.h"
#include "network.h"
#include "upper_to_fpga.h"
#include "fpga_init.h"
#include "fpga_to_upper.h"
#include "sensor.h"

void my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz)
{
    if (sz & 63) {
        sz = (sz & -64) + 64;
    }
    
    asm volatile (
            "1:                                    \n"
            "    VLDM %[src]!,{d0-d7}                 \n"
            "    VSTM %[dst]!,{d0-d7}                 \n"
            "    SUBS %[sz],%[sz],#0x40                 \n"
            "    BGT 1b                            \n"
            : [dst]"+r"(dst), [src]"+r"(src), [sz]"+r"(sz) : : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
}

/********************************************************************************
 * 名称：                    checksum
 * 功能：                    和校验
 * 入口参数：            	 char *buf, unsigned int nword
 * 出口参数：            	 无
 *********************************************************************************/
unsigned short checksum(char *buf, unsigned int nword)  
{  
    unsigned long sum;    
    for(sum = 0; nword > 0; nword--)  
        sum += *buf++;              /*获取buf的累加和*/     
    sum  = (sum>>16) + (sum&0xffff);/*获取sum高16位与低16位的和*/  
    sum += (sum>>16);     
    return ~sum;  
}  
/********************************************************************************
 * 名称：                    send_all_package_to_upper
 * 功能：                    ARM向上位机发送声呐数据和传感器数据等总包
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void send_all_package_to_upper(void)
{
    if (send(Connect_fd, DataHead_S, SIZE_OF_LONG, 0) < 0) 
    {       
        error_process();
        return; 
    }
    // 发送4字节总长度
    if (send(Connect_fd,(char*)&total_len, SIZE_OF_LONG, 0) < 0) 
    {       
        error_process();
        return; 
    }
    //发送46+整包ddr+传感器+crc+报尾
    if (send(Connect_fd, (char *)(ddr_sonar_data+SONAR_DATA_OFFSET-SIZE_OF_SEND_UPPER_PACKAGE_FIRST), total_len, 0)< 0) 
    {       
        error_process();
        return; 
    }  
}
/********************************************************************************
 * 名称：                    lookfor_five_sensor_num
 * 功能：                    确定几个传感器各自的条数
 * 入口参数：            	 const int *ptr_sensor1,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5
 * 出口参数：            	 无
 *********************************************************************************/
void lookfor_five_sensor_num(FPGA_DDR_FIRST* ptr_fpga_frame_first_p,const int *ptr_sensor1,const int *ptr_sensor2,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5)
{
    int m;
    for (m=0;m<100;m++)
    {
        if ((*(ptr_sensor1+m*64)==0xDDDDDDDD)&&(*(ptr_sensor1+m*64+1)==(ptr_fpga_frame_first_p->FrameNumber-1)))
        {
            sensor1_num++;
        }
        if ((*(ptr_sensor2+m*64)==0xDDDDDDDD)&&(*(ptr_sensor2+m*64+1)==(ptr_fpga_frame_first_p->FrameNumber-1)))
        {
            sensor2_num++;
        }
        if ((*(ptr_sensor3+m*64)==0xDDDDDDDD)&&(*(ptr_sensor3+m*64+1)==(ptr_fpga_frame_first_p->FrameNumber-1)))
        {
            sensor3_num++;
        }
        if ((*(ptr_sensor4+m*64)==0xDDDDDDDD)&&(*(ptr_sensor4+m*64+1)==(ptr_fpga_frame_first_p->FrameNumber-1)))
        {
            sensor4_num++;
        }
        if ((*(ptr_sensor5+m*64)==0xDDDDDDDD)&&(*(ptr_sensor5+m*64+1)==(ptr_fpga_frame_first_p->FrameNumber-1)))
        {
            sensor5_num++;
        }
    }
}

/********************************************************************************
 * 名称：                    copy_sensor_data
 * 功能：                    将传感器信息拷贝到ddr_sonar_data数组的指定位置中
 * 入口参数：            	 若干
 * 出口参数：            	 无
 *********************************************************************************/
void copy_sensor_data(FPGA_DDR_FIRST* ptr_fpga_frame_first_p,const int *ptr_sensor1,const int *ptr_sensor2,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5)
{
    lookfor_five_sensor_num(ptr_fpga_frame_first_p,ptr_sensor1,ptr_sensor2,ptr_sensor3,ptr_sensor4,ptr_sensor5);
    my_copy((unsigned char *)(ddr_sonar_data + ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST), (unsigned char *)ptr_sensor1, sensor1_num*SIZE_OF_ONE_PING_SENSOR); 
    my_copy((unsigned char *)(ddr_sonar_data + ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG + sensor1_num * SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor2, sensor2_num * SIZE_OF_ONE_PING_SENSOR); 
    my_copy((unsigned char *)(ddr_sonar_data + ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG *2 + sensor1_num * SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor3, sensor3_num * SIZE_OF_ONE_PING_SENSOR); 
    my_copy((unsigned char *)(ddr_sonar_data + ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG *3+(sensor1_num + sensor2_num + sensor3_num)*SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor4, sensor4_num*SIZE_OF_ONE_PING_SENSOR);
    my_copy((unsigned char *)(ddr_sonar_data + ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG *4+(sensor1_num+sensor3_num+sensor4_num)*SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor5,sensor5_num*SIZE_OF_ONE_PING_SENSOR);
}

/********************************************************************************
 * 名称：                    copy_letf_three_sensor_num_to_ddr
 * 功能：                    将除了GGA_ZDA的另外3个传感器信息的条数信息拷贝到ddr_sonar_data数组的指定位置中
 * 入口参数：            	 若干
 * 出口参数：            	 无
 *********************************************************************************/
void copy_letf_three_sensor_num_to_ddr(void)
{
    *((int *)(ddr_sonar_data+ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + sensor1_num*SIZE_OF_ONE_PING_SENSOR))=sensor2_num;//gga条数

    *((int *)(ddr_sonar_data+ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG + (sensor1_num+sensor2_num)*SIZE_OF_ONE_PING_SENSOR))=sensor3_num;//heading条数

    *((int *)(ddr_sonar_data+ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG*2 + (sensor1_num+sensor2_num+sensor3_num) * SIZE_OF_ONE_PING_SENSOR)) = sensor4_num; //motion条数

    *((int *)(ddr_sonar_data+ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG*3+(sensor1_num+sensor2_num+sensor3_num+sensor4_num)*SIZE_OF_ONE_PING_SENSOR)) =sensor5_num;//SVT条数
}

/********************************************************************************
 * 名称：                    copy_sensor_data_from_pingpang_buf
 * 功能：                    这个函数调用上述几个函数完成从乒乓缓存拷贝传感器数据到ddr_sonar_data数组的指定位置中
 * 入口参数：            	 若干
 * 出口参数：            	 无
 *********************************************************************************/
void copy_sensor_data_from_pingpang_buf(FPGA_DDR_FIRST* ptr_fpga_frame_first_p,int *uiosensor_p,int *uiosensor1_p,int *uiosensor2_p,int *uiosensor3_p,int *uiosensor4_p)
{
    copy_sensor_data(ptr_fpga_frame_first_p,uiosensor_p,uiosensor1_p,uiosensor2_p,uiosensor3_p,uiosensor4_p);
    ptr_send_to_upper_sensor->Senor_total_len=SIZE_OF_SENSOR_WITHOUT_SENSOR_DATA + SIZE_OF_ONE_PING_SENSOR * (sensor1_num+sensor2_num+sensor3_num+sensor4_num+sensor5_num);
    ptr_send_to_upper_sensor->SynchState = 1;
    ptr_send_to_upper_sensor->GGA_ZDA_NUM = sensor1_num; 
    memcpy(ddr_sonar_data+ptr_send_to_upper_package_first->SonarDataLength+SONAR_DATA_OFFSET,ptr_send_to_upper_sensor, SIZE_OF_SENSOR_FIRST); //先拷贝传感器总字节数，同步状态，预留字节，GGA个数这16字节的信息
    copy_letf_three_sensor_num_to_ddr(); 
}
/********************************************************************************
 * 名称：                    copy_tail_to_sonar_data
 * 功能：                    拷贝报尾ED>>四个字节到ddr_sonar_data数组的末尾
 * 入口参数：            	 若干
 * 出口参数：            	 无
 *********************************************************************************/
void copy_tail_to_sonar_data(void)
{
    memcpy(ddr_sonar_data + SONAR_DATA_OFFSET + ptr_send_to_upper_package_first->SonarDataLength + SIZE_OF_LEN_OF_SENSOR_DATA + SIZE_OF_SENSOR_WITHOUT_SENSOR_DATA + (sensor1_num+sensor2_num+sensor3_num+sensor4_num+sensor5_num) * SIZE_OF_ONE_PING_SENSOR + SIZE_OF_USHORT, DataTail_S, SIZE_OF_LONG); 
}

/********************************************************************************
 * 名称：                    send_all_package_to_upper_and_clear_sensor_num
 * 功能：                    ARM向上位机发送整个数据包，其中包括声呐数据和传感器数据等所有，然后清传感器条数
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void send_all_package_to_upper_and_clear_sensor_num(void)
{
    total_len = SIZE_OF_SEND_UPPER_PACKAGE_FIRST + ptr_send_to_upper_package_first->SonarDataLength + SIZE_OF_LEN_OF_SENSOR_DATA + ptr_send_to_upper_sensor->Senor_total_len + SIZE_OF_USHORT + SIZE_OF_LONG;
    if (Fpga_start_mod == 1) //开始
    {
        pthread_mutex_lock(&mut);
        // Send_data_status=1; 
        send_all_package_to_upper();
        pthread_mutex_unlock(&mut);
        sensor1_num=0;
        sensor2_num=0;
        sensor3_num=0;
        sensor4_num=0;
        sensor5_num=0;    
    }
}

/********************************************************************************
 * 名称：                    prepare_first_128_byte_data
 * 功能：                    准备ARM向上位机发送整个数据包中的前128个参数信息
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void prepare_first_128_byte_data(FPGA_DDR_FIRST *ptr_fpga_frame_first_p,int *headuio_p)
{
    //拷贝帧头
    memcpy(ptr_fpga_frame_first_p, headuio_p, SIZE_OF_DATA_FIRST); 
    //打印功率系数
    ptr_send_to_upper_package_first->power_factor = (float)(ptr_fpga_frame_first_p->POWER_FACTOR>>16)/10.0f;//new add
    ptr_send_to_upper_package_first->TimeStamp  = (unsigned int)((float)ptr_fpga_frame_first_p->PPS_10ns/(FPGA_CLK_FREQUENCY/1000)); //时间戳单位是毫秒
    ptr_send_to_upper_package_first->PPS_number = ptr_fpga_frame_first_p->PPS_s;  //PPS 计数
    ptr_send_to_upper_package_first->PingCount = ptr_fpga_frame_first_p->FrameNumber; //帧号
    ptr_send_to_upper_package_first->DataType = (unsigned short)ptr_fpga_frame_first_p->Data_Type;
    ptr_send_to_upper_package_first->WorkMode = (unsigned short)ptr_fpga_frame_first_p->Work_Mode;
    ptr_send_to_upper_package_first->LFMMode = (unsigned short)ptr_fpga_frame_first_p->LFM_Mode;
    ptr_send_to_upper_package_first->PWMFreq = ptr_fpga_frame_first_p->PWM_FREQUENCY;
    ptr_send_to_upper_package_first->PWMBandWidth = ptr_fpga_frame_first_p->PWM_BAND_WIDTH;
    ptr_send_to_upper_package_first->Range = (unsigned short)ptr_fpga_frame_first_p->RANGE;
    ptr_send_to_upper_package_first->Sampling = (unsigned short)ptr_fpga_frame_first_p->SAMPLING_RATE;
    ptr_send_to_upper_package_first->PWMPulseWidth = (float)((float)ptr_fpga_frame_first_p->PWM_PULSE_WIDTH)/100.0f;
    ptr_send_to_upper_package_first->pwm_start = (unsigned short)ptr_fpga_frame_first_p->PWM_START;
    
    memset(ptr_send_to_upper_package_first->INIT_PHASE,0,sizeof(ptr_send_to_upper_package_first->INIT_PHASE));

    //Debug("INIT_PHASE[2]: %f\n", ptr_send_to_upper_package_first->INIT_PHASE[2]);
    ptr_send_to_upper_package_first->TransGear = (char)ptr_fpga_frame_first_p->TransGear;
}

/********************************************************************************
 * 名称：                    prepare_IQ_data_len
 * 功能：                    准备IQ数据的声呐数据长度，并拷贝IQ数据的前128字节
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void prepare_IQ_data_len(FPGA_DDR_FIRST* ptr_fpga_frame_first_p)
{
    ptr_send_to_upper_package_first->SonarDataLength = ((ptr_fpga_frame_first_p->AD_sn + 1) / SAMPLE_FACTOR * SIZE_OF_CHANNEL_NUM * SIZE_OF_LONG); //声呐数据总数
    memcpy(ddr_sonar_data+ SONAR_DATA_OFFSET -SIZE_OF_SEND_UPPER_PACKAGE_FIRST,ptr_send_to_upper_package_first,SIZE_OF_SEND_UPPER_PACKAGE_FIRST);//128字节的参数信息
}

/********************************************************************************
 * 名称：                    prepare_original_data_len
 * 功能：                    准备原始数据的声呐数据长度，并拷贝IQ数据的前128字节
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void prepare_original_data_len(FPGA_DDR_FIRST* ptr_fpga_frame_first_p)
{
    ptr_send_to_upper_package_first->SonarDataLength = ((ptr_fpga_frame_first_p->AD_sn + 1) *SIZE_OF_CHANNEL_NUM_ORIGINAL * SIZE_OF_USHORT); //声呐数据总数
    memcpy(ddr_sonar_data+ SONAR_DATA_OFFSET -SIZE_OF_SEND_UPPER_PACKAGE_FIRST,ptr_send_to_upper_package_first,SIZE_OF_SEND_UPPER_PACKAGE_FIRST);//128字节的参数信息
}


/********************************************************************************
 * 名称：                    make_all_send_to_upper_package
 *                          组合所有数据包发送给显控
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void make_all_send_to_upper_package(
                                        FPGA_DDR_FIRST* ptr_fpga_frame_first_p,
                                        int *headuio_p, 
                                        int *uioiq_p,
                                        int *uioad_p,
                                        int *uiosensor_p,
                                        int *uiosensor1_p,
                                        int *uiosensor2_p,
                                        int *uiosensor3_p,
                                        int *uiosensor4_p
                                    ){
    prepare_first_128_byte_data(ptr_fpga_frame_first_p,headuio_p);  
    //根据帧头判断IQ数据还是原始数据
    if ((ptr_fpga_frame_first_p->FrameHead == 0xAAAAAAAA) && (ptr_fpga_frame_first_p->SmallFrameHead == 0xCCCCCCCC))//IQ数据
    {
        /*计算IQ数据的声纳数据长度 并拷贝128字节发送给显控的数据头*/
        prepare_IQ_data_len(ptr_fpga_frame_first_p);
        my_copy((unsigned char *)(ddr_sonar_data + SONAR_DATA_OFFSET), (unsigned char *)(uioiq_p), ptr_send_to_upper_package_first->SonarDataLength); 
    } 
    else if ((ptr_fpga_frame_first_p->FrameHead == 0xAAAAAAAA) && (ptr_fpga_frame_first_p->SmallFrameHead == 0xBBBBBBBB))//原始数据
    {
        prepare_original_data_len(ptr_fpga_frame_first_p);
        my_copy((unsigned char *)(ddr_sonar_data + SONAR_DATA_OFFSET), (unsigned char *)(uioad_p), ptr_send_to_upper_package_first->SonarDataLength); 
    }else{
        return ;
    }  
    copy_sensor_data_from_pingpang_buf(ptr_fpga_frame_first_p,uiosensor_p,uiosensor1_p,uiosensor2_p,uiosensor3_p,uiosensor4_p);
    copy_tail_to_sonar_data();    
    send_all_package_to_upper_and_clear_sensor_num();           
}


/********************************************************************************
 * 名称：                    read_fpga_send_to_upper
 * 功能：                    FPGA--->ARM--->上位机主线程
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void read_fpga_send_to_upper(void)
{
    unsigned icount;
    int err;
    double timeuse;
    while (1)
    {
        //gettimeofday(&start_time,NULL);
        err = read(fd_uio6, &icount, 4); //进中断
        write(fd_uio6, &irq_on, sizeof(irq_on));//清中断
        if (err != 4) 
        {
            perror("uio read err\n");
        }
        else
        {
            flag++;
            //DBG("flag: %d\n", flag);
            if(flag % 2 == 0){
                make_all_send_to_upper_package(
                    ptr_fpga_frame_first_a,
                    data_head_mem_a.mem_ptr,
                    uio_iq_mem_a.mem_ptr,
                    original_mem.mem_ptr,
                    sensor_mem_0.mem_ptr,
                    uio_sensor_mem_1.mem_ptr,
                    uio_sensor_mem_2.mem_ptr,
                    uio_sensor_mem_3.mem_ptr,
                    uio_sensor_mem_4.mem_ptr
                );
            }else if(flag % 2 == 1){ 
                make_all_send_to_upper_package(
                    ptr_fpga_frame_first_b,
                    data_head_mem_b.mem_ptr,
                    uio_iq_mem_b.mem_ptr,
                    original_mem.mem_ptr,
                    sensor_mem_0.mem_ptr,
                    uio_sensor_mem_9.mem_ptr,
                    uio_sensor_mem_A.mem_ptr,
                    uio_sensor_mem_B.mem_ptr,
                    uio_sensor_mem_C.mem_ptr
                );
            }else{
                printf("flag error\n");
            }
        } 
    }
}

