/*****************************************************************************
 * * Copyright (C) 2022 Beijing StarTest High-Tech Co., Ltd.
 * * 
 * * All rights reserved by Star Test, Inc.
 * * 
 * * FilePath     : beam_down.c
 * * 
 * * Author       : WuCheng <wucheng@startest.net>
 * * 
 * * Date         : 2022-10-13 17:05:04
 * * 
 * * LastEditTime : 2022-11-17 11:06:09
 ******************************************************************************/
#include <stdlib.h>
#include <sys/time.h>
#include "main.h"
#include "tcptrans_link.h"
#include "uppertodry.h"
#include "drytoupper.h"
#include "upgrade_program.h"
#include "uppertodry.h"
#include "wettodry.h"
#include "fpga_init.h"
#include "tempsensor.h"
#include "armtodsp.h"
#include "armtofpga.h"
#include "beam_down.h"

struct timeval tv2 = {0};
struct timeval tv3 = {0};
int BeamDataSize = 0, BeamDataTotalSize = 0;
int All_Rapidio_SendToDSP_Size = 0, Rapidio1_Send_Size = 0, Rapidio2_Send_Size = 0;
float Overall_Angle = 0.0f, Beam_Angle = 0.0f;
char Ins_Mod_State = 0;
char Svp_Data[10] = {0};

/***********内置获取同步Roll的参数*********************/
INS_SENSOR_DATA_HEAD *ptr_InsSensor_Data_Para = NULL;
INS_SENSOR_DATA *ptr_Ins_Sensor_Data = NULL;
char Mems_Data[200000] = {0};//接收湿端MEMS数据，DDDD开头
unsigned int Recv_WetMemsNum = 0;//1ping 接收到湿端MEMS的个数
unsigned int Recv_WetMemsSumNum = 0;//接收到湿端MEMS的总个数，每一ping都会累加，最大不超过400个；
/********************************************/

/***********内置获取同步时刻的参数*********************/
unsigned int Mems_PPS_Num_Last = 0;
float Ins_Zda_Time = 0.0f;
char Ins_zda_time_hour[2] = {0}, Ins_zda_time_min[2] = {0},Ins_zda_time_sec[6] = {0};
char* pIns_zda_sec = Ins_zda_time_sec + 6;
char Ins_flag_date = 0;
/********************************************/

/***********外置获取同步Roll的参数*********************/
EXT_GPZDA_DATA *ptr_Ext_Gpzda_Data = NULL;
char Ext_Gpzda_Data[128000] = {0};//接收干端数据，DDDD开头
char Ext_Tss1_Data[128000] = {0};//接收干端数据，DDDD开头
char Ext_Handing_Data[128000] = {0};
unsigned int Recv_ExtSensorNum = {0};//1ping 接收到湿端MEMS的个数
unsigned int Recv_ExtSensor_SumNum = 0;//接收到湿端MEMS的总个数，每一ping都会累加，最大不超过400个；
/********************************************/

/***********外置获取同步时刻的参数*********************/
unsigned int ExtSensor_PPS_Num_Last = 0;
float Ext_Zda_Time = 0.0f;
char Ext_zda_time_hour[2] = {0}, Ext_zda_time_min[2] = {0}, Ext_zda_time_sec[6] = {0};
char* pExt_zda_sec = Ext_zda_time_sec + 6;
char Ext_flag_date = 0;
/********************************************/

float Sonar_Sync_Time = 0.0f;
float sonar_stime = 0.0f;
float Roll_Sequential_Value[400] = {0};//橫摇序列值
float Motion_Time_Sync[400] = {0};
float Roll_Time_Diff[400] = {0};//橫摇与声学数据时间差预留
float roll_value = 0.0f;


#define MAX_SIZE 10000

int memory[MAX_SIZE] = {0};
int top = -1;

void* my_malloc(int size) {
    if (top + size > MAX_SIZE) {
        return NULL;
    }
    void* ptr = memory + top;
    top += size;
    return ptr;
}

void my_free(void* ptr) {

}

//动态内存分配
void * tmalloc(int size_t)
{
    static int array[5120] = {0};/*空间不能大于65535*/
    int * head = 0; /* 表头，组成格式为0xcdxxxxac，其中xx代表已经分配了的空间长度 */
    int * nextHead = 0;
    int *ret = 0;
    int status = 0;
    int i = 0;
    int intSize;
    
    if(size_t > sizeof(array) || size_t < 1)
    {
        return ret;
    }
    
    head = array;   /*初始化表指针*/
    intSize =  size_t / 4;   /*计算需要多少int型空间*/
    if(size_t % 4 != 0)
    {
        intSize += 1;
    }
    
    do
    {
        if((head + intSize) >= array + (sizeof(array) / 4)) /*内存不足，分配失败*/
        {
            status = 1;
            break;
        }
        if((head[0] >> 24) == 0xcd) /*内存被使用*/
        {
            if((head[0] & 0xff) == 0xac)
            {
                 head +=  ((head[0] >> 8) & 0xffff) + 1;
            }   
        }
        else  /*内存没有被使用*/
        {
            nextHead = head + 1;
            for(i = 0;i < intSize;i++)
            {
                if((nextHead[0] >> 24) != 0xcd) 
                {
                    nextHead++;
                }
                else /*找到下一个表头*/
                {
                   head = nextHead;
                   break;
                }
            }
            if(i == intSize)//找到未被分配的空间
            {
                break;
            }
        }
    
    }while(1);
    
    if(status == 0) /*更新表头，返回地址*/
    {
        head[0] =(0xcd0000ac | (intSize << 8));
        ret = head + 1;
    }
    else
    {
        ret = 0;
    }
    
    return ret;
}

//内存释放
int tfree(void * ptemp)
{
    int *head = 0;
    int size = 0;
    int i = 0;
    
    head = (int *)ptemp;
    head -= 1;
    
    if((head[0] >> 24) != 0xcd)
    {
        return  0;
    }
    size = (head[0] >> 8) & 0xffff;
    
    for(i = 0;i <= size;i++)
    {
        head[i] = 0;
    } 
    return  1;    
}



/*****************************************************************************
 * * description : 切换管道时,清空同步的相关buf
 * * return       {*}
 * * Date        : 2022-11-02 13:58:31
 * * Other
 ******************************************************************************/
void Change_InsModeState_ClearBuf(void)
{
    Sonar_FristPing_flag = 0;
    Recv_WetMemsNum = 0;
    Recv_WetMemsSumNum = 0;
    Recv_ExtSensorNum = 0;
    Recv_ExtSensor_SumNum = 0;
    Mems_PPS_Num_Last = 0;
    ExtSensor_PPS_Num_Last = 0;
    Ins_Zda_Time = 0;
    Ext_Zda_Time = 0;
    Ext_flag_date = 0;
    Ins_flag_date = 0;
    Sonar_Sync_Time = 0;
    roll_value = 0;
    memset(Ext_zda_time_hour,0,sizeof(Ext_zda_time_hour));
    memset(Ext_zda_time_min,0,sizeof(Ext_zda_time_min));
    memset(Ext_zda_time_sec,0,sizeof(Ext_zda_time_sec));
    memset(Ins_zda_time_hour,0,sizeof(Ins_zda_time_hour));
    memset(Ins_zda_time_min,0,sizeof(Ins_zda_time_min));
    memset(Ins_zda_time_sec,0,sizeof(Ins_zda_time_sec));
    memset(Roll_Time_Diff,0,sizeof(Roll_Time_Diff));
    memset(Motion_Time_Sync,0,sizeof(Motion_Time_Sync));
    memset(Roll_Sequential_Value,0,sizeof(Roll_Sequential_Value));

    Sensor1_Num = 0;
    Sensor2_Num = 0;
    Sensor3_Num = 0;
    Sensor4_Num = 0;
}

  
 #if 0 
void Test_SYNC_Time_Roll_Func(void)
{
  
    fwrite(&(IQData_ParaHead_Last.PingCount), 4, 1, fb);
    printf("Fwrite_PingCnt=%d\n", IQData_ParaHead_Last.PingCount);
    fwrite(&Recv_WetMemsSumNum, 4, 1, fb);
    printf("Fwrite_Recv_WetMemsSumNum=%d\n", Recv_WetMemsSumNum);
    fwrite(&Motion_Time_Sync, 400, 4, fb);
    //printf("Motion_Time_Sync=%f\n", Motion_Time_Sync[0]);
    fwrite(&Sonar_Sync_Time, 4, 1, fb);
    printf("Fwrite_sonar_sync_time=%f\n", Sonar_Sync_Time);
    fwrite(&Roll_Sequential_Value, 400, 4, fb);
    //printf("wuroll=%f\n", Roll_Sequential_Value[0]);
    fwrite(&(ptr_fpga_register_data->fpga_registers_parameters.Mems_Num), 4, 1, fb);
    printf("Fwrite_Mems_Num=%d\n", ptr_fpga_register_data->fpga_registers_parameters.Mems_Num);
    fwrite(&Toa_Time_s, 512, 4, fb);
    //printf("Toa_Time_s=%f\n", Toa_Time_s[0]); 
}
#endif

/*****************************************************************************
 * * description : 1.设置FPGA波束形成时需要的最左侧波速开角即波速角的最小值
 *                 2.将扇形区域的角度均分为512份,每一份即为波速开角的递增系数
 *                 3.2^29 = 536870912 FPGA是29位小数
 *                 4.除以180的角度与弧度的转换，其中乘以pi在FPGA中实现
 * * return       {*}
 * * Date        : 2022-10-13 16:05:27
 * * Other       : [波束下放]
 ******************************************************************************/
static void Set_Beam_Angle_Value(void)
{
    pthread_mutex_lock(&mut2);
    Overall_Angle = (cmd_package.Finish_Angle - cmd_package.Start_Angle);//-65~65
    Beam_Angle = ((0.5 * Overall_Angle / 512) - (Overall_Angle / 2));//-64.873声纳图左边第一个波速角度
    ptr_fpga_register_data->fpga_registers_parameters.Beam_Angle = (int)(Beam_Angle / 180 * FPGA_2_29);//开角角度
    ptr_fpga_register_data->fpga_registers_parameters.Angle_k = (int)(Overall_Angle / 180 / 512 * FPGA_2_29);//512波束均分130度后的弧度值
    pthread_mutex_unlock(&mut2);
}

/*****************************************************************************
 * * description : 获取声速
 * * return       {*}
 * * Date        : 2022-10-13 14:08:30
 * * Other       : [波束下放]
 ******************************************************************************/
float svp_n = 0.0f;//20240109svp有时解析为0,加一个中间变量保存上次解析的svp
static void Set_SoundSpeed_Value(void)
{  
    int n = 0;
  //  pthread_mutex_lock(&mut2);
	if (cmd_package.Manual_SoundSpeed > 1) //手动声速,即显控下发的声速值
    {
        ptr_fpga_register_data->fpga_registers_parameters.Manual_SoundSpeed = cmd_package.Manual_SoundSpeed;
	/*	DBG("Recv upper soundspeed\n");
		DBG("Svp_Mode = %f\t Svp_Value = %f\n", cmd_package.Manual_SoundSpeed ,ptr_fpga_register_data->fpga_registers_parameters.Manual_SoundSpeed);
 */       
    }
    else if (cmd_package.Manual_SoundSpeed < 1)//表声上传的声速
	{
        if (Sensor4_Num > 0)
        {
	//	DBG("Sensor4_Num = %d\n",Sensor4_Num);
            for ( n = 0; n < 8; ++n)
            {
           //     Svp_Data[n] = *(unsigned char*)(IQData + IQDataSizeFromWetSend - 6 + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG * 4 + (Sensor1_Num + Sensor2_Num + Sensor3_Num) * SIZE_OF_ONE_PING_SENSOR + 16 + 2 + n * 2);
				Svp_Data[n] = *(unsigned char*)(IQData+SIZE_OF_WET_SONAR_FIRST+8+ptr_IQData_ParaHead->SonarDataSize+4+SIZE_OF_LONG*5+(Sensor1_Num+Sensor2_Num+Sensor3_Num)*SIZE_OF_ONE_PING_SENSOR+16+2+n*2);
			}
//			DBG("Svp_Data = %s\n",Svp_Data);
			if(strtof(Svp_Data,NULL)< 1250 || strtof(Svp_Data,NULL) > 1600 )
			{
				ptr_fpga_register_data->fpga_registers_parameters.Manual_SoundSpeed = svp_n;
			} 
			else
			{
				ptr_fpga_register_data->fpga_registers_parameters.Manual_SoundSpeed = strtof(Svp_Data,NULL);
				svp_n = strtof(Svp_Data,NULL);
			}
		}
		else 
		{
		//	ptr_fpga_register_data->fpga_registers_parameters.Manual_SoundSpeed = 1500;
			ptr_fpga_register_data->fpga_registers_parameters.Manual_SoundSpeed = svp_n;
		}
	}
    else
    {
        ;//空语句
    }
//	DBG("Svp_Mode = %f\t Svp_Value = %f\n", cmd_package.Manual_SoundSpeed ,ptr_fpga_register_data->fpga_registers_parameters.Manual_SoundSpeed);
    // pthread_mutex_unlock(&mut2);
}

/*****************************************************************************
 * * description : 设备横摇补偿系数
 * * return       {*}
 * * Date        : 2022-10-13 17:07:20
 * * Other       : [波束下放]
 ******************************************************************************/
static void Set_Coe_Value(void)
{
    long long PwmFreq = 400;
    pthread_mutex_lock(&mut2);
    ptr_fpga_register_data->fpga_registers_parameters.Roll_Coe = PwmFreq * (-3.5) / (ptr_fpga_register_data->fpga_registers_parameters.Manual_SoundSpeed) * FPGA_2_30;
    ptr_fpga_register_data->fpga_registers_parameters.Tao_Coe = 35 / (ptr_fpga_register_data->fpga_registers_parameters.Manual_SoundSpeed) * FPGA_2_30;
    pthread_mutex_unlock(&mut2);
}

/*****************************************************************************
 * * description : 设置波束形成时FPGA所需要的参数
 * * return       {*}
 * * Date        : 2022-10-13 17:28:21
 * * Other       : [波束下放]
 ******************************************************************************/
static void Set_BeamDown_Fpga_Para(void)
{
    Set_Beam_Angle_Value();
  //  Set_SoundSpeed_Value();
    Set_Coe_Value();
}

/*****************************************************************************
 * * description : 读取MEMS数据的参数头:DDDD开头
 * * param        {char} *pmems_data    要读出的MEMSdata包
 * * param        {int} offset          第几包的MEMSdata
 * * return       {*}                   正确返回0
 * * Date        : 2022-10-20 13:14:41
 * * Other
 ******************************************************************************/
static int Read_Mems_Data_Head(char *pmems_data,int offset)
{

 //   if (NULL != (ptr_InsSensor_Data_Para = (INS_SENSOR_DATA_HEAD*)malloc(INSSENSOR_DATA_PARA_LEN)))
    {
	//	DBG("malloc 3\n");
        memset(ptr_InsSensor_Data_Para, 0L, INSSENSOR_DATA_PARA_LEN);
        //memcpy(ptr_InsSensor_Data_Para, (pmems_data + offset * 1024),INSSENSOR_DATA_PARA_LEN);
        memcpy(ptr_InsSensor_Data_Para, (pmems_data + offset * 256),INSSENSOR_DATA_PARA_LEN);
       // ptr_InsSensor_Data_Para->PPS_ms = ptr_InsSensor_Data_Para->PPS_ms / 100000;
        return OK;
    }
 //   else  return FAIL;
}

/*****************************************************************************
 * * description : 读取一包MEMS data:$snc200开头
 * * param        {char} *pmems_data  要读取的MEMSdata
 * * param        {int} offset        要读取第几个MEMSdata
 * * return       {*}                 正确返回0
 * * Date        : 2022-10-20 13:16:06
 * * Other
 ******************************************************************************/
static int Read_Mems_OnePack_Data(char *pmems_data,int offset)
{
  //  if (NULL != (ptr_Ins_Sensor_Data = (INS_SENSOR_DATA*)malloc(INSSENSOR_DATA_LEN)))
    {
        memset(ptr_Ins_Sensor_Data, 0L, INSSENSOR_DATA_LEN);
        //memcpy(ptr_Ins_Sensor_Data, (pmems_data + (offset*1024) + INSSENSOR_DATA_PARA_LEN), INSSENSOR_DATA_LEN);
        memcpy(ptr_Ins_Sensor_Data, (pmems_data + (offset*256) + INSSENSOR_DATA_PARA_LEN), INSSENSOR_DATA_LEN);
        return OK;
    } 
  //  else  return FAIL;
}

/*****************************************************************************
 * * description : 得到MEMSdata里的那些参数
 * * param        {char} l_range    获得参数的范围
 * * param        {char} h_range    获得参数的范围
 * * return       {*}               正确返回0
 * * Date        : 2022-10-20 13:17:54
 * * Other
 ******************************************************************************/
static int Get_Data_for_Mems(char l_range,char h_range)
{
    int rd_inscnt = 0;
    unsigned char* pRdIns = NULL;
    unsigned char* ptr_Read_Ins = NULL;
    int val_cnt = 0;
    int Ins_Len = 0;

    if (NULL == (ptr_Read_Ins = (unsigned char*)tmalloc(INSSENSOR_DATA_LEN)))
		return FAIL;
//	DBG("malloc 2\n");
    memset(ptr_Read_Ins, 0L, INSSENSOR_DATA_LEN);
    pRdIns = (unsigned char*)ptr_Ins_Sensor_Data;
    while (!(*(unsigned char*)(pRdIns - 2) == 0X2A))//*
    {
//	DBG("pRdIns = %#x\n",(*(unsigned char*)(pRdIns-2)));
        rd_inscnt = 0;
        while (0 != strncmp((char*)pRdIns, ",", sizeof(char)))
        {
            if ((*(unsigned char*)pRdIns == 0x2A))  break;
            strncpy((char*)(ptr_Read_Ins + rd_inscnt), (char*)pRdIns, sizeof(char));
            pRdIns = pRdIns + 2;
            rd_inscnt = rd_inscnt + 1;
        }
        strncpy((char*)(ptr_Read_Ins + rd_inscnt), "\0", sizeof(char));
        Ins_Len = rd_inscnt + 1;
	//	DBG("val_cnt = %d\n",val_cnt);
        if ((val_cnt > l_range) && (val_cnt < h_range))//只取ZDA
        {
            switch (val_cnt)
            {
                case 0: memcpy(ptr_Ins_Sensor_Data->Ins_Head, ptr_Read_Ins, Ins_Len); break;
                case 1: memcpy(ptr_Ins_Sensor_Data->Ins_GnggaHead, ptr_Read_Ins, Ins_Len); break;
                case 2: memcpy(ptr_Ins_Sensor_Data->Ins_UTCtime, ptr_Read_Ins, Ins_Len); break;
                case 3: memcpy(ptr_Ins_Sensor_Data->Ins_Lat, ptr_Read_Ins, Ins_Len); break;
                case 4: memcpy(ptr_Ins_Sensor_Data->Ins_NorS, ptr_Read_Ins, Ins_Len); break;
                case 5: memcpy(ptr_Ins_Sensor_Data->Ins_Long, ptr_Read_Ins, Ins_Len); break;
                case 6: memcpy(ptr_Ins_Sensor_Data->Ins_WorE, ptr_Read_Ins, Ins_Len); break;
                case 7: memcpy(ptr_Ins_Sensor_Data->Ins_Sat_State, ptr_Read_Ins, Ins_Len); break;
                case 8: memcpy(ptr_Ins_Sensor_Data->Ins_Sat_Num, ptr_Read_Ins, Ins_Len); break;
                case 9: memcpy(ptr_Ins_Sensor_Data->Ins_HDOP, ptr_Read_Ins, Ins_Len); break;
                case 10:memcpy(ptr_Ins_Sensor_Data->Ins_Alt, ptr_Read_Ins, Ins_Len); break;
                case 11:memcpy(ptr_Ins_Sensor_Data->Ins_Alt_Unit, ptr_Read_Ins, Ins_Len); break;
                case 12:memcpy(ptr_Ins_Sensor_Data->Ins_Undulation, ptr_Read_Ins, Ins_Len); break;
                case 13:memcpy(ptr_Ins_Sensor_Data->Ins_Ins_Undulation_Unit, ptr_Read_Ins, Ins_Len); break;
                case 14:memcpy(ptr_Ins_Sensor_Data->Ins_Age, ptr_Read_Ins, Ins_Len); break;
                case 15:memcpy(ptr_Ins_Sensor_Data->Ins_Age_ID, ptr_Read_Ins, Ins_Len); break;
                case 16:memcpy(ptr_Ins_Sensor_Data->Ins_GpzdaHead, ptr_Read_Ins, Ins_Len); break;
                case 17:memcpy(ptr_Ins_Sensor_Data->Ins_Zdatime, ptr_Read_Ins, Ins_Len); break;
                case 18:memcpy(ptr_Ins_Sensor_Data->Ins_Day, ptr_Read_Ins, Ins_Len); break;
                case 19:memcpy(ptr_Ins_Sensor_Data->Ins_Mounth, ptr_Read_Ins, Ins_Len); break;
                case 20:memcpy(ptr_Ins_Sensor_Data->Ins_Year, ptr_Read_Ins, Ins_Len); break;
                case 21:memcpy(ptr_Ins_Sensor_Data->Ins_Hour, ptr_Read_Ins, Ins_Len); break;
                case 22:memcpy(ptr_Ins_Sensor_Data->Ins_Min, ptr_Read_Ins, Ins_Len); break;
                case 23:memcpy(ptr_Ins_Sensor_Data->Ins_HehdtHead, ptr_Read_Ins, Ins_Len); break;
                case 24:memcpy(ptr_Ins_Sensor_Data->Ins_Heading, ptr_Read_Ins, Ins_Len); break;
                case 25:memcpy(ptr_Ins_Sensor_Data->Ins_Tss1Head, ptr_Read_Ins, Ins_Len); break;
                case 26:memcpy(ptr_Ins_Sensor_Data->Ins_Hor_Acc, ptr_Read_Ins, Ins_Len); break;
                case 27:memcpy(ptr_Ins_Sensor_Data->Ins_Ver_Acc, ptr_Read_Ins, Ins_Len); break;
                case 28:memcpy(ptr_Ins_Sensor_Data->Ins_Space, ptr_Read_Ins, Ins_Len); break;
                case 29:memcpy(ptr_Ins_Sensor_Data->Ins_Heave, ptr_Read_Ins, Ins_Len); break;
                case 30:memcpy(ptr_Ins_Sensor_Data->Ins_horH, ptr_Read_Ins, Ins_Len); break;
                case 31:memcpy(ptr_Ins_Sensor_Data->Ins_Roll, ptr_Read_Ins, Ins_Len); break;
                case 32:memcpy(ptr_Ins_Sensor_Data->Ins_Space1, ptr_Read_Ins, Ins_Len); break;
                case 33:memcpy(ptr_Ins_Sensor_Data->Ins_Pitch, ptr_Read_Ins, Ins_Len); break;
                case 34:memcpy(ptr_Ins_Sensor_Data->Ins_Ant1_Num, ptr_Read_Ins, Ins_Len); break;
                case 35:memcpy(ptr_Ins_Sensor_Data->Ins_Ant2_Num, ptr_Read_Ins, Ins_Len); break;
                case 36:memcpy(ptr_Ins_Sensor_Data->Ins_Imu_State, ptr_Read_Ins, Ins_Len); break;
                case 37:memcpy(ptr_Ins_Sensor_Data->Ins_Sys_State, ptr_Read_Ins, Ins_Len); break;
                case 38:memcpy(ptr_Ins_Sensor_Data->Ins_GpVtgHead, ptr_Read_Ins, Ins_Len); break;
                case 39:memcpy(ptr_Ins_Sensor_Data->Ins_Spd_Speed, ptr_Read_Ins, Ins_Len); break;
                case 40:memcpy(ptr_Ins_Sensor_Data->Ins_Spd_Unit, ptr_Read_Ins, Ins_Len); break;
                default: break;
            }
        }
        pRdIns = pRdIns + 2;
        ptr_Read_Ins = ptr_Read_Ins + rd_inscnt;
        val_cnt++;
	//	DBG("val_cnt = %d\n",val_cnt);
    }
    rd_inscnt = 0;
    val_cnt = 0;
    Ins_Len = 0;
    ptr_Read_Ins = NULL;
    pRdIns = NULL;
    my_free(ptr_Read_Ins);
    tfree(ptr_Read_Ins);
	top = -1;
    return OK;
}

/*****************************************************************************
 * * description :获得声纳同步时间
 * * param        {char} *ptr_mes     MEMSdata
 * * param        {unsigned int} cnt  1ping中共几包传感器数据
 * * return       {*}
 * * Date        : 2022-10-20 13:18:37
 * * Other       :通过上1ping的数据来计算当前ping的同步时间
 ******************************************************************************/
void Get_SonarSyncTime_Ins(char *ptr_mes,unsigned int cnt)
{
    unsigned int Mems_PPS_Num = 0;  
    
	if(NULL ==  (ptr_InsSensor_Data_Para = (INS_SENSOR_DATA_HEAD*)my_malloc(INSSENSOR_DATA_PARA_LEN)))
		return FAIL;
    if (NULL == (ptr_Ins_Sensor_Data = (INS_SENSOR_DATA*)my_malloc(INSSENSOR_DATA_LEN)))
    	return FAIL; 
    for (Recv_WetMemsNum = 0; Recv_WetMemsNum < cnt; ++Recv_WetMemsNum)
    {
	//	DBG("Mems_PPS_Num = %d\n",ptr_InsSensor_Data_Para->PPS_s);
        Read_Mems_Data_Head(ptr_mes,Recv_WetMemsNum);//ddddd
        Mems_PPS_Num = ptr_InsSensor_Data_Para->PPS_s;
        
        if (Mems_PPS_Num != Mems_PPS_Num_Last)//当PPS跳变时，取跳变时的ZDA时间；
        {
            DBG("****************Mems_PPS_Num Changed = [%d]********************\n",Mems_PPS_Num);
            Mems_PPS_Num_Last = Mems_PPS_Num; 
            Read_Mems_OnePack_Data(ptr_mes,Recv_WetMemsNum);//snc200
            Get_Data_for_Mems(16, 21);
            strncpy(Ins_zda_time_hour, (char*)ptr_Ins_Sensor_Data->Ins_Zdatime, 2);
            strncpy(Ins_zda_time_min, (char*)ptr_Ins_Sensor_Data->Ins_Zdatime + 2, 2);
            strncpy(Ins_zda_time_sec, (char*)ptr_Ins_Sensor_Data->Ins_Zdatime + 4, 6);
        }
        Ins_Zda_Time = atoi(Ins_zda_time_hour) * 60 * 60 + atoi(Ins_zda_time_min) * 60 + strtof(Ins_zda_time_sec, &pIns_zda_sec);
        Sonar_Sync_Time = Ins_Zda_Time + (float)IQData_ParaHead_Last.TimeStamp / 1000 + (float)IQData_ParaHead_Last.PPS_Number - (float)Mems_PPS_Num;//声学同步时间 ,单位是S，精度mS

        ptr_fpga_register_data->fpga_registers_parameters.Time_Sec = (Sonar_Sync_Time - (int)floor(Sonar_Sync_Time)) + (int)(floor(Sonar_Sync_Time)) % 60;
        ptr_fpga_register_data->fpga_registers_parameters.Time_Min = ((int)(floor(Sonar_Sync_Time)) % 3600 - ((int)(floor(Sonar_Sync_Time)) % 3600) % 60) / 60;
        ptr_fpga_register_data->fpga_registers_parameters.Time_Hours = ((int)(floor(Sonar_Sync_Time)) - (int)(floor(Sonar_Sync_Time)) % 3600) / 3600; 
        if (Ins_flag_date == 0)
        {
            Ins_flag_date = 1;
            ptr_fpga_register_data->fpga_registers_parameters.Time_Date = atoi((char*)ptr_Ins_Sensor_Data->Ins_Day);
            ptr_fpga_register_data->fpga_registers_parameters.Time_Date |= (atoi((char*)ptr_Ins_Sensor_Data->Ins_Mounth) << 8);
            ptr_fpga_register_data->fpga_registers_parameters.Time_Year = atoi((char*)ptr_Ins_Sensor_Data->Ins_Year);
        }  
	}
/*	Debug("PingCount = [%d] Mems_PPS_Num = [%d] Sonar_Sync_Time = [%f] \n",IQData_ParaHead_Last.PingCount,Mems_PPS_Num,Sonar_Sync_Time);
	Debug("Ins_Zda_Time = [%f]\n" ,Ins_Zda_Time);
	Debug("Sonar_Sync_Time = [%f]\n",Sonar_Sync_Time);
	Debug("sec = [%f]\n",ptr_fpga_register_data->fpga_registers_parameters.Time_Sec);
	Debug("min = [%d]\n", ptr_fpga_register_data->fpga_registers_parameters.Time_Min);
	Debug("hours = [%d]\n",ptr_fpga_register_data->fpga_registers_parameters.Time_Hours);
	Debug("data = [%d]\n",ptr_fpga_register_data->fpga_registers_parameters.Time_Date);
	Debug("year = [%d]\n",ptr_fpga_register_data->fpga_registers_parameters.Time_Year);*/
	ptr_InsSensor_Data_Para = NULL;
	ptr_Ins_Sensor_Data = NULL;
	my_free(ptr_InsSensor_Data_Para);
	my_free(ptr_Ins_Sensor_Data);
	top = -1;
}

/*****************************************************************************
 * * description : 获得当前ping的传感器时间及传感器的横摇值
 * * param        {char*} ptr_mes       MEMSdata
 * * param        {unsigned int} cnt    1ping中共几包传感器数据
 * * return       {*}
 * * Date        : 2022-10-20 13:20:09
 * * Other
 ******************************************************************************/
void Get_MotionTime_Roll_Ins(char* ptr_mes,unsigned int cnt)
{
    float Motion_Time;
    char motion_time_hour[2], motion_time_min[2],motion_time_sec[6];
    char* pIns_motiom_sec = motion_time_sec + 6;
    int  i = 0;

    if (NULL == (ptr_Ins_Sensor_Data = (INS_SENSOR_DATA*)my_malloc(INSSENSOR_DATA_LEN)))
    	return FAIL; 
	for (Recv_WetMemsNum = 0; Recv_WetMemsNum < cnt; ++Recv_WetMemsNum)
    {
        Read_Mems_OnePack_Data(ptr_mes,Recv_WetMemsNum); 
        Get_Data_for_Mems(16, 32);

        strncpy(motion_time_hour, (char*)ptr_Ins_Sensor_Data->Ins_Zdatime, 2);
        strncpy(motion_time_min, (char*)ptr_Ins_Sensor_Data->Ins_Zdatime + 2, 2);
        strncpy(motion_time_sec, (char*)ptr_Ins_Sensor_Data->Ins_Zdatime + 4, 6);
        Motion_Time = atoi(motion_time_hour) * 60 * 60 + atoi(motion_time_min) * 60 + strtof(motion_time_sec, &pIns_motiom_sec);
        
        Motion_Time_Sync[Recv_WetMemsSumNum] = Motion_Time;
        Roll_Sequential_Value[Recv_WetMemsSumNum] = (float)(atoi((char*)ptr_Ins_Sensor_Data->Ins_Roll)) / 100;//* 3.14 / 180;角度值
        Recv_WetMemsSumNum = Recv_WetMemsSumNum + 1;
    }
    for (i = 0; i < Recv_WetMemsSumNum; i++)
    {
        Roll_Time_Diff[i] = Motion_Time_Sync[i] - Sonar_Sync_Time;
        //printf("Roll_Time_Diff[%d] = %f   Roll_Sequential_Value[%d] = %f\n", i, Roll_Time_Diff[i] * 1000, i, Roll_Sequential_Value[i] * 3.14 / 180);
    }
    ptr_fpga_register_data->fpga_registers_parameters.Recv_SensorSumNum = Recv_WetMemsSumNum;
	//printf("fpga_registers_parameters.Recv_SensorSumNum = %d\n", Recv_SensorSumNum);
	ptr_Ins_Sensor_Data = NULL;
	my_free(ptr_Ins_Sensor_Data);
//	free(ptr_Ins_Sensor_Data);
	top = -1;

}

/*****************************************************************************
 * * description : 从众多的横摇里面挑出来发射同步时刻的横摇,并将值写道Bram中.共512个值
 * * return       {*}
 * * Date        : 2022-10-20 13:21:37
 * * Other         
 ******************************************************************************/
static void Find_RollValueFromRollBuf(void)
{
    float info1 = 2, info2 = 2;
    int  info1_index = 0, info2_index = 0;
    int  a = 0, b = 0;
    float roll_time_syn[400];
  
    for (b = 0; b < 400; b++)
    {
        info1 = 2, info2 = 2;
        info1_index = 0; info2_index = 0;
        for (a = Recv_WetMemsSumNum - 1; a >= 0; a--)
        {
            roll_time_syn[a] = Roll_Time_Diff[a] - Toa_Time_s[b];
            if (roll_time_syn[a] >= 0)
            {
                info2 = Motion_Time_Sync[a];
                info2_index = a;
            }
            else {
                info1 = Motion_Time_Sync[a];
                info1_index = a;
            }
            if (info1 != 2 && info2 != 2) break;
        }
        if (info1 == 2 && info2 == 2)
        {
            Roll_Sequential_Value[b] = 0;
        }
        else if (info1 == 2)
        {
            roll_value = Roll_Sequential_Value[0];
            if (cmd_package.Roll_stability == 0)
                *((int*)(uio_beamangel_bram.mem_ptr) + b) = (int)((Beam_Angle + 0) / 180 * 536870912) + ptr_fpga_register_data->fpga_registers_parameters.Angle_k * b;
            else
                *((int*)(uio_beamangel_bram.mem_ptr) + b) = (int)((Beam_Angle  / 180 * 536870912) + ptr_fpga_register_data->fpga_registers_parameters.Angle_k * b + roll_value / 180 * 536870912);
            //fwrite(&roll_value, 4, 1, fb);
        }
        else if (info2 == 2)
        {
            roll_value = Roll_Sequential_Value[Recv_WetMemsSumNum - 1];

            if (cmd_package.Roll_stability == 0)
                *((int*)(uio_beamangel_bram.mem_ptr) + b) = (int)((Beam_Angle + 0) / 180 * 536870912) + ptr_fpga_register_data->fpga_registers_parameters.Angle_k * b;
            else
                *((int*)(uio_beamangel_bram.mem_ptr) + b) = (int)((Beam_Angle / 180 * 536870912) + ptr_fpga_register_data->fpga_registers_parameters.Angle_k * b + roll_value / 180 * 536870912);
            //fwrite(&roll_value, 4, 1, fb);
        }
        else
        {
            double da = b * 0.005 - Roll_Time_Diff[info1_index];
            double db = Roll_Time_Diff[info2_index] - b * 0.005;
            double dc = Roll_Time_Diff[info2_index] - Roll_Time_Diff[info1_index];

            if (dc < 1)
            {
                roll_value = Roll_Sequential_Value[info1_index];
                if (cmd_package.Roll_stability == 0)
                    *((int*)(uio_beamangel_bram.mem_ptr) + b) = (int)((Beam_Angle + 0) / 180 * 536870912) + ptr_fpga_register_data->fpga_registers_parameters.Angle_k * b;
                else
                    *((int*)(uio_beamangel_bram.mem_ptr) + b) = (int)((Beam_Angle / 180 * 536870912) + ptr_fpga_register_data->fpga_registers_parameters.Angle_k * b + roll_value / 180 * 536870912);
                //fwrite(&roll_value, 4, 1, fb);
            }
            else
            {
                roll_value = (float)((Roll_Sequential_Value[info1_index] * db + Roll_Sequential_Value[info2_index] * da) / dc);
                if (cmd_package.Roll_stability == 0)
                    *((int*)(uio_beamangel_bram.mem_ptr) + b) = (int)((Beam_Angle + 0) / 180 * 536870912) + ptr_fpga_register_data->fpga_registers_parameters.Angle_k * b;
                else
                    *((int*)(uio_beamangel_bram.mem_ptr) + b) = (int)((Beam_Angle / 180 * 536870912) + ptr_fpga_register_data->fpga_registers_parameters.Angle_k * b + roll_value / 180 * 536870912);
                //fwrite(&roll_value, 4, 1, fb);
            }
        }
    }
	/*
        DBG("roll_value=%f\n",roll_value);
		DBG("Roll_Sequential_Value[] = %f\n",Roll_Sequential_Value[b]);
		DBG("Roll_Sequential_Value[] = %f\n",Roll_Sequential_Value[0]);*/
}

/*****************************************************************************
 * * description :设置FPGA波束形成需要的参数,并计算发送给DSP的长度
 * * return       {*}
 * * Date        : 2022-10-20 13:27:48
 * * Other
 ******************************************************************************/
void Set_Fpga_To_Dsp_Para(void)
{
    //湿端FPGA抽样后为40K带宽，%2标识FPGA需要偶数个数的采样点，如是奇数则抛掉
    ptr_fpga_register_data->fpga_registers_parameters.Ad_Sn = ((ptr_IQData_ParaHead_Last->SonarDataSize / 384 / SAMPLE_FACTOR) - (ptr_IQData_ParaHead_Last->SonarDataSize / 384 / SAMPLE_FACTOR) % 2) * SAMPLE_FACTOR;
    //由于湿端的40K带宽干端波束形成做不过来，所有又一次抽样，抽成了20K
    ptr_fpga_register_data->fpga_registers_parameters.Ad_Sn_After = (ptr_IQData_ParaHead_Last->SonarDataSize / 384 / SAMPLE_FACTOR) - (ptr_IQData_ParaHead_Last->SonarDataSize / 384 / SAMPLE_FACTOR) % 2;
    //1ping中每200个采样点取一个Mems_Num
    ptr_fpga_register_data->fpga_registers_parameters.Mems_Num = (unsigned int)((ptr_fpga_register_data->fpga_registers_parameters.Ad_Sn_After + 199) / 200);
    ptr_fpga_register_data->fpga_registers_parameters.Data_Type = ptr_IQData_ParaHead_Last->DataType;

    //波束形成后的数据长度：1ping的采样点数*（全阵位宽+子阵位宽）/ 8 * 波束个数 = Ad_Sn_After * 12 * 512
    BeamDataSize = ptr_fpga_register_data->fpga_registers_parameters.Ad_Sn_After * 12;
  //  BeamDataSize = ptr_fpga_register_data->fpga_registers_parameters.Ad_Sn_After * 12 * 512;
    BeamDataTotalSize = BeamDataSize + SIZE_OF_RAPIDIO_FIRST;//从帧头开始算起（含帧头）-> 波束数据结束
    All_Rapidio_SendToDSP_Size = 4 + BeamDataTotalSize + 4 + 4 + 4 + IQDataTotalSize_Last + 2;//除去帧头及帧长度后面的所有字节的长度,以$$结尾
    //将总长分为一半,保证每2k长度发送
    Rapidio1_Send_Size = All_Rapidio_SendToDSP_Size / 2 - (All_Rapidio_SendToDSP_Size / 2) % 2048;
    Rapidio2_Send_Size = (All_Rapidio_SendToDSP_Size / 2 + (All_Rapidio_SendToDSP_Size / 2) % 2048)  + 2048 - (All_Rapidio_SendToDSP_Size / 2 + (All_Rapidio_SendToDSP_Size / 2) % 2048) % 2048;
}


/*****************************************************************************
 * * description : 将波束下放的整包数据,从ARM端拷贝值FPGA侧,准备发送值DSP
 * * return       {*}
 * * Date        : 2022-10-20 10:14:44
 * * Other
 ******************************************************************************/
void Copy_AllDataToFpga(UIO_CONFIG_PARAMETER struct_uio_ddr)
{
    int total_len_iqallsize = 0;
    char rapidio_head[4] = { '@','@','@','@' };
    char rapidio_tail[2] = { '$','$' };
    char SendUpperHead[4] = { '<','<','S','T' };
    int pingnum = 0;
  
    memcpy((unsigned char*)(struct_uio_ddr.mem_ptr), rapidio_head, 4);//@@@@
	memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 4), &All_Rapidio_SendToDSP_Size, 4);//帧长
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 8), &BeamDataTotalSize, 4);//波束数据总长度（含参数头）
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA ),ptr_recv_upper_package->CH_error,240);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240), Roll_Time_Diff, 1600);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600), Roll_Sequential_Value, 1600);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600), &BeamDataSize, 4);
//	DBG("BeamDataSize = %d\n",BeamDataSize);
    total_len_iqallsize = IQDataTotalSize_Last + 8;//包含了<<ST + 长度 + IQData + ED>>
//	DBG("total_len_iqallsize = %d\n",total_len_iqallsize);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize), &total_len_iqallsize, 4);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize + 4), SendUpperHead, 4);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize + 4 + 4), &IQDataTotalSize_Last, 4);
    my_copy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize + 4 + 4 + 4), IQData_Last, IQDataTotalSize_Last);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize + 4 + 4 + 4 + IQDataTotalSize_Last), rapidio_tail, 2); 
//	memcpy(&pingnum,(unsigned char*)((unsigned char*)IQData_Last+ 14),  4);

	/*  
    memcpy((unsigned char*)(struct_uio_ddr.mem_ptr), rapidio_head, 4);//@@@@
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 4), &All_Rapidio_SendToDSP_Size, 4);//帧长
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 8), &BeamDataTotalSize, 4);//波束数据总长度（含参数头）
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240), Roll_Time_Diff, 1600);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600), Roll_Sequential_Value, 1600);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600), &BeamDataSize, 4);
    total_len_iqallsize = IQDataTotalSize_Last + 8;//包含了<<ST + 长度 + IQData + ED>>
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize), &total_len_iqallsize, 4);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize + 4), SendUpperHead, 4);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize + 4 + 4), &IQDataTotalSize_Last, 4);
    my_copy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize + 4 + 4 + 4), IQData_Last, IQDataTotalSize_Last);
    memcpy((unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr) + 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize + 4 + 4 + 4 + IQDataTotalSize_Last), rapidio_tail, 2); 
    memcpy(&pingnum,(unsigned char*)((unsigned char*)(struct_uio_ddr.mem_ptr)+ 12 + SIZE_OF_RAPIDIO_FIRST_FPGA + 240 + 1600 + 1600 + 4 + BeamDataSize + 4 + 4 + 4 + 11),  4);
   */
//	DBG("Send to Dsp PingNum is %d\n",pingnum);
  //  ptr_fpga_register_addr->Svp_Buffer_Addr = pingnum;//波束数据参数
}

/*****************************************************************************
 * * description : 配置读写rapidio地址,目前是3个buff轮发,每个buf最大204M大小
 * * return       {*}
 * * Date        : 2022-10-20 11:11:20
 * * Other        目前硬件湿2X SRIO,其中LAN1作为写数据总线,LAN0作为读数据总线
 ******************************************************************************/
void Set_Dsp_TransBuf(void)
{
    if (Bd_Date_TransBuf_Flag == 1)
    {
        //配置波束地址
        ptr_fpga_register_addr->BeamPackageHead_Addr = 0x19000000 + 12;//帧头+帧长度+波束数据长度
        ptr_fpga_register_addr->Beam_Addr1 = 0x19000000 + 12 + SIZE_OF_RAPIDIO_FIRST;//波束数据参数
        ptr_fpga_register_addr->Beam_Addr2 = 0x19000000 + 12 + SIZE_OF_RAPIDIO_FIRST + BeamDataSize / 3;
        ptr_fpga_register_addr->Beam_Addr3 = 0x19000000 + 12 + SIZE_OF_RAPIDIO_FIRST + BeamDataSize / 3 * 2;
        ptr_fpga_register_addr->IQ_Base_Addr = 0x19000000 + 12 + SIZE_OF_RAPIDIO_FIRST + BeamDataSize + 12 + SIZE_OF_WET_SONAR_FIRST;
        //配置rapidio 0 目前没有使用
        ptr_fpga_register_addr->Send_Byte_Cnt_0 = Rapidio1_Send_Size;//需要发送的数据长度
        ptr_fpga_register_addr->send_Base_Addr_0 = 0x19000000;
        ptr_fpga_register_addr->Send_Target_Addr_0 = 0x80000000;
        //配置rapidio 1
        ptr_fpga_register_addr->Send_Byte_Cnt_1 = Rapidio1_Send_Size + Rapidio2_Send_Size;
        ptr_fpga_register_addr->send_Base_Addr_1 = 0x19000000;
      //  ptr_fpga_register_addr->Send_Target_Addr_1 = 0x80000000;//dsp_srio高速总线写缓冲地址

        ptr_fpga_register_addr->Read_TargetAddr_0_Buffer1 = 0x0c000000;//dsp_srio高速总线读缓冲地址
        ptr_fpga_register_addr->Read_TargetAddr_0_Buffer2 = 0x0c000000;
        ptr_fpga_register_addr->Read_Byte_Cnt_0 = 0x40000;//3M数据
       // ptr_fpga_register_addr->Read_Byte_Cnt_0 = 0x100000;//3M数据
       //ptr_fpga_register_addr->Read_Byte_Cnt_0 = 0x300000;//3M数据

        //ptr_fpga_register_addr->Read_TargetAddr_0_Buffer1 = 0x0c020000;
        //ptr_fpga_register_addr->Read_TargetAddr_0_Buffer2 = 0x0c020000;
       

        //拷贝数据到ddr
        //printf("*******1*************\n");
        Copy_AllDataToFpga(uio_ddr_mem_IQ_1);            
        usleep(1000);
        Bd_Date_TransBuf_Flag = Bd_Date_TransBuf_Flag + 1;
        ptr_fpga_register_addr->Addr_irq = 0;
        usleep(1000);
        ptr_fpga_register_addr->Addr_irq = 1;
    }
    else if (Bd_Date_TransBuf_Flag == 2)
    {
        //配置波束地址
        ptr_fpga_register_addr->BeamPackageHead_Addr = 0x25C00000 + 12;
        ptr_fpga_register_addr->Beam_Addr1 = 0x25C00000 + 12 + SIZE_OF_RAPIDIO_FIRST;
        ptr_fpga_register_addr->Beam_Addr2 = 0x25C00000 + 12 + SIZE_OF_RAPIDIO_FIRST + BeamDataSize / 3;
        ptr_fpga_register_addr->Beam_Addr3 = 0x25C00000 + 12 + SIZE_OF_RAPIDIO_FIRST + BeamDataSize / 3 * 2;
        ptr_fpga_register_addr->IQ_Base_Addr = 0x25C00000 + 12 + SIZE_OF_RAPIDIO_FIRST + BeamDataSize + 12 + SIZE_OF_WET_SONAR_FIRST;
        //配置rapidio 0
        ptr_fpga_register_addr->Send_Byte_Cnt_0 = Rapidio1_Send_Size;
        ptr_fpga_register_addr->send_Base_Addr_0 = 0x25C00000;  
        ptr_fpga_register_addr->Send_Target_Addr_0 = 0x80000000;
        //配置rapidio 1
        ptr_fpga_register_addr->Send_Byte_Cnt_1 = Rapidio1_Send_Size + Rapidio2_Send_Size;
        ptr_fpga_register_addr->send_Base_Addr_1 = 0x25C00000;
     //   ptr_fpga_register_addr->Send_Target_Addr_1 = 0x80000000;

        //printf("*******2*************\n");
        Copy_AllDataToFpga(uio_ddr_mem_IQ_2);
        usleep(1000);
        Bd_Date_TransBuf_Flag = Bd_Date_TransBuf_Flag + 1;
        ptr_fpga_register_addr->Addr_irq = 0;
        usleep(1000);
        ptr_fpga_register_addr->Addr_irq = 1;
    }
    else if (Bd_Date_TransBuf_Flag == 3)
    {
        //配置波束地址
        ptr_fpga_register_addr->BeamPackageHead_Addr = 0x32800000 + 12;
        ptr_fpga_register_addr->Beam_Addr1 = 0x32800000 + 12 + SIZE_OF_RAPIDIO_FIRST;
        ptr_fpga_register_addr->Beam_Addr2 = 0x32800000 + 12 + SIZE_OF_RAPIDIO_FIRST + BeamDataSize / 3;
        ptr_fpga_register_addr->Beam_Addr3 = 0x32800000 + 12 + SIZE_OF_RAPIDIO_FIRST + BeamDataSize / 3 * 2;
        ptr_fpga_register_addr->IQ_Base_Addr = 0x32800000 + 12 + SIZE_OF_RAPIDIO_FIRST + BeamDataSize + 12 + SIZE_OF_WET_SONAR_FIRST;
        //配置rapidio 0
        ptr_fpga_register_addr->Send_Byte_Cnt_0 = Rapidio1_Send_Size;
        ptr_fpga_register_addr->send_Base_Addr_0 = 0x32800000;
        ptr_fpga_register_addr->Send_Target_Addr_0 = 0x80000000;
        //配置rapidio 1
        ptr_fpga_register_addr->Send_Byte_Cnt_1 = Rapidio1_Send_Size + Rapidio2_Send_Size;
        ptr_fpga_register_addr->send_Base_Addr_1 = 0x32800000;
     //   ptr_fpga_register_addr->Send_Target_Addr_1 = 0x80000000;

        //printf("*******3*************\n");                            
        Copy_AllDataToFpga(uio_ddr_mem_IQ_3);
        usleep(1000);
        Bd_Date_TransBuf_Flag = 1;
        ptr_fpga_register_addr->Addr_irq = 0;
        usleep(1000);
        ptr_fpga_register_addr->Addr_irq = 1;
    }
    else
    {
        ;//空语句
    }
}

/*****************************************************************************
 * * description : 波束下放内置传感器处理程序
 * * return       {*}
 * * Date        : 2022-10-20 13:10:35
 * * Other       : 当前IQdata中存放的是上1ping的MEMSdata.所以计算上1ping同步时间时,需要用当前ping的数据
 ******************************************************************************/
static void Beam_Down_Ins_Mode(void)
{

    if ((Recv_WetMemsSumNum + Recv_Wet_MemsSensorHead.GGA_ZDA_NUM) <= 400)
    {
		Get_SonarSyncTime_Ins(Mems_Data,Recv_Wet_MemsSensorHead.GGA_ZDA_NUM);
        Get_MotionTime_Roll_Ins(Mems_Data,Recv_Wet_MemsSensorHead.GGA_ZDA_NUM);
    }
    else if ((Recv_WetMemsSumNum + Recv_Wet_MemsSensorHead.GGA_ZDA_NUM) > 400)
    {
        float Temp_Save[400];
        if (Recv_Wet_MemsSensorHead.GGA_ZDA_NUM < 400)
        {
            memcpy(Temp_Save, ((unsigned int *)(Roll_Time_Diff )+ Recv_WetMemsSumNum + Recv_Wet_MemsSensorHead.GGA_ZDA_NUM - 400), (400 - Recv_Wet_MemsSensorHead.GGA_ZDA_NUM)*4); 
            memcpy(Roll_Time_Diff, Temp_Save, (400 - Recv_Wet_MemsSensorHead.GGA_ZDA_NUM)*4);

            memcpy(Temp_Save, ((unsigned int*)(Motion_Time_Sync)+Recv_WetMemsSumNum + Recv_Wet_MemsSensorHead.GGA_ZDA_NUM - 400), (400 - Recv_Wet_MemsSensorHead.GGA_ZDA_NUM) * 4);  
            memcpy(Motion_Time_Sync, Temp_Save, (400 - Recv_Wet_MemsSensorHead.GGA_ZDA_NUM) * 4);  

            memcpy(Temp_Save,((unsigned int*) (Roll_Sequential_Value) + Recv_WetMemsSumNum + Recv_Wet_MemsSensorHead.GGA_ZDA_NUM - 400), (400 - Recv_Wet_MemsSensorHead.GGA_ZDA_NUM) * 4);
            memcpy(Roll_Sequential_Value, Temp_Save, (400 - Recv_Wet_MemsSensorHead.GGA_ZDA_NUM)*4);

            Recv_WetMemsSumNum = 400 - Recv_Wet_MemsSensorHead.GGA_ZDA_NUM;//目前已有Roll值的个数
        }
        else
        {
            Recv_WetMemsSumNum = 0;
            Recv_WetMemsNum = Recv_Wet_MemsSensorHead.GGA_ZDA_NUM - 400;
        }
        Get_SonarSyncTime_Ins(Mems_Data,Recv_Wet_MemsSensorHead.GGA_ZDA_NUM);
        Get_MotionTime_Roll_Ins(Mems_Data,Recv_Wet_MemsSensorHead.GGA_ZDA_NUM);
    } 
    else
    {
        ;//空语句
    }
}


/*****************************************************************************
 * * description : 读取一包外置传感器GNGGA数据
 * * param        {char} *ptr_ext
 * * param        {int} offset
 * * return       {*}
 * * Date        : 2022-11-02 14:00:04
 * * Other
 ******************************************************************************/
static int Read_Ext_Gngga_OnePack_Data(char *ptr_ext, int offset)
{
 //   if (NULL != (ptr_Ext_Gpzda_Data = (EXT_GPZDA_DATA*)malloc(SIZE_OF_ONE_PING_SENSOR)))
    {
        memset(ptr_Ext_Gpzda_Data, 0L, SIZE_OF_ONE_PING_SENSOR);
		memcpy(ptr_Ext_Gpzda_Data, (char*)(ptr_ext + (offset * 256) + EXTSENSOR_DATA_PARA_LEN), SIZE_OF_ONE_PING_SENSOR);
        return OK;
    }
 //   else  return FAIL;
}


/*****************************************************************************
 * * description : 得到外置传感器ZPZDA的值
 * * return       {*}
 * * Date        : 2022-11-02 14:01:58
 * * Other
 ******************************************************************************/
char Get_Zpzda_Data_for_Ext_Sensor(void)
{
    int rd_extcnt = 0;
    unsigned char* pRdExt = NULL;
    unsigned char* ptr_Read_Ext = NULL;
    int val_cnt = 0;
    int Ins_Len = 0;
    char deal_head[20] = { 0 };
    int head_flag = 0;

    if (NULL == (ptr_Read_Ext = (unsigned char*)tmalloc(SIZE_OF_ONE_PING_SENSOR))){
		DBG("malloc zda ptr failed\r\n");
        return FAIL;
    }
    memset(ptr_Read_Ext, 0L, SIZE_OF_ONE_PING_SENSOR);
    pRdExt = (unsigned char*)ptr_Ext_Gpzda_Data;
    if(pRdExt == NULL){
        DBG("pRdExt is NULL\r\n");
        tfree(ptr_Read_Ext);
        return FAIL;
    }

/*   
    20251211:
        解决提取ZDA数据
*/
    //提取协议头，仅处理zda数据
    int head_idx = 0;
    unsigned char* p_head = pRdExt;
    unsigned char* pRdExt_end = pRdExt + SIZE_OF_ONE_PING_SENSOR;   //传感器数据256字节边界
    
    //读取前40字节（覆盖20有效字符）
    while(head_idx < 19 && p_head < pRdExt + 40){
        if( *p_head == '$' ){
            deal_head[head_idx++] = *p_head;
            p_head += 2;
            //继续读取直到 ， * ，提取完整协议头
            while(head_idx < 19 && *p_head != ',' && *p_head != 0x2A){
                deal_head[head_idx++] = *p_head;
                p_head += 2;
            }
            break;
        }
        p_head += 2;
    }
    deal_head[head_idx] = '\0';
    // printf("识别到ZDA头[%s],开始下一步解算\r\n",deal_head);
    if(strstr(deal_head,"ZDA") == NULL){
        // DBG("非 ZDA协议头[%s] 跳过解算\r\n",deal_head);
        tfree(ptr_Read_Ext);
        return FAIL;
    }

    pRdExt = p_head;
    

    while(pRdExt < pRdExt_end && val_cnt < 7){
        
        rd_extcnt = 0;
        //读取到 ， * 为止
        while(pRdExt < pRdExt_end && *pRdExt != ',' && *pRdExt != '*'){
            //防止溢出保护
            if(rd_extcnt >= (SIZE_OF_ONE_PING_SENSOR-1)){
                // DBG("ZDA 字段长度溢出\r\n");
                tfree(ptr_Read_Ext);
                return FAIL;
            }

            //字符复制
            *(char*)(ptr_Read_Ext + rd_extcnt) = *pRdExt;
            pRdExt += 2;
            rd_extcnt++;            
        }

        //字段字符串加 '\0'
        *(char*)(ptr_Read_Ext + rd_extcnt) = '\0';
        Ins_Len = rd_extcnt + 1;

        //按照ZDA协议填充字段
        switch (val_cnt)
        {
            case 0: 
                memcpy(ptr_Ext_Gpzda_Data->Ext_GpzdaHead, deal_head, head_idx+1); 
                // printf("填充Ext_GpzdaHead：%s\r\n",ptr_Ext_Gpzda_Data->Ext_GpzdaHead);
                break;
            case 1: 
                memcpy(ptr_Ext_Gpzda_Data->Ext_Zdatime, ptr_Read_Ext, Ins_Len); 
                // printf("填充Ext_Zdatime：%s\r\n",ptr_Ext_Gpzda_Data->Ext_Zdatime);
                break;
            case 2: 
                memcpy(ptr_Ext_Gpzda_Data->Ext_Day, ptr_Read_Ext, Ins_Len); 
                // printf("填充Ext_Day：%s\r\n",ptr_Ext_Gpzda_Data->Ext_Day);
                break;
            case 3: 
                memcpy(ptr_Ext_Gpzda_Data->Ext_Mounth, ptr_Read_Ext, Ins_Len); 
                // printf("填充Ext_Mounth：%s\r\n",ptr_Ext_Gpzda_Data->Ext_Mounth);
                break;
            case 4: 
                memcpy(ptr_Ext_Gpzda_Data->Ext_Year, ptr_Read_Ext, Ins_Len); 
                // printf("填充Ext_Year：%s\r\n",ptr_Ext_Gpzda_Data->Ext_Year);
                break;
            case 5: 
                memcpy(ptr_Ext_Gpzda_Data->Ext_Hour, ptr_Read_Ext, Ins_Len); 
                // printf("填充Ext_Hour：%s\r\n",ptr_Ext_Gpzda_Data->Ext_Hour);
                break;
            case 6: 
                memcpy(ptr_Ext_Gpzda_Data->Ext_Min, ptr_Read_Ext, Ins_Len); 
                // printf("填充Ext_Min：%s\r\n",ptr_Ext_Gpzda_Data->Ext_Min);
                break;
            default: 
                break;
        }

        //指针偏移 跳过当前逗号
        if(*pRdExt == ',') {
            pRdExt += 2;
        } 
        
        val_cnt++;

        //找到终止符 * 退出循环
        if(*pRdExt == 0x2A){
            // printf("找到ZDA终止符 结算结束\r\n");
            break;
        }

    }

    rd_extcnt = 0;
    val_cnt = 0;
    Ins_Len = 0;
    ptr_Read_Ext = NULL;
    pRdExt = NULL;
    tfree(ptr_Read_Ext);
	top = -1;
    return OK;
}


/*****************************************************************************
 * * description : 获取外置传感器的同步时间
 * * param        {char*} ptr_ext
 * * param        {int} cnt
 * * return       {*}
 * * Date        : 2022-11-02 14:02:30
 * * Other
 ******************************************************************************/
static void Get_SonarSyncTime_Ext(char* ptr_ext,int cnt)
{
    static float  Last_Zda_Time = 0;
    unsigned int ExtSensor_PPS_Num = 0;

    if (NULL == (ptr_Ext_Gpzda_Data = (EXT_GPZDA_DATA*)my_malloc(SIZE_OF_ONE_PING_SENSOR)))
    	return FAIL;
    // printf("===============================================================================================================\r\n");
    // printf("ZDA数据总条数：(IQdate->帧计数-1 = SensorDate->帧计数)%d \r\n",cnt);
    // printf("IQdate->帧计数 = [%d]，帧计数-1=[%d]\n",IQData_ParaHead_Last.PingCount,IQData_ParaHead_Last.PingCount-1);
    // printf("---------------------------------------------------------------------------------------------------------------------1\r\n");
    for (Recv_ExtSensorNum = 0; Recv_ExtSensorNum < cnt; ++Recv_ExtSensorNum)
    {
        //传感器PPS秒计数
        ExtSensor_PPS_Num =   *(int*)((char*)ptr_ext + 8 + Recv_ExtSensorNum * 256);
        // printf("上一条ZDA传感器数据->PPS秒计数 ExtSensor_PPS_Num_Last = %d\r\n",ExtSensor_PPS_Num_Last);
        // printf("当前ZDA传感器数据->PPS秒计数 ExtSensor_PPS_Num = %d\r\n",ExtSensor_PPS_Num);
        
        if (ExtSensor_PPS_Num != ExtSensor_PPS_Num_Last)//当PPS跳变时，取跳变时的ZDA时间；50hz
        {
            // printf("PPS跳变 与上一次PPS值不同 重新获取ZDA时间信息\r\n");
            Read_Ext_Gngga_OnePack_Data(ptr_ext, Recv_ExtSensorNum);
            Get_Zpzda_Data_for_Ext_Sensor();
            if (strncmp((char*)ptr_Ext_Gpzda_Data->Ext_GpzdaHead, "$GPZDA", 7) == 0 | strncmp((char*)ptr_Ext_Gpzda_Data->Ext_GpzdaHead, "$GNZDA", 7) == 0 | strncmp((char*)ptr_Ext_Gpzda_Data->Ext_GpzdaHead, "$BDZDA", 7) == 0)
            {                
                strncpy(Ext_zda_time_hour, (char*)ptr_Ext_Gpzda_Data->Ext_Zdatime, 2);
                strncpy(Ext_zda_time_min, (char*)ptr_Ext_Gpzda_Data->Ext_Zdatime + 2, 2);
                strncpy(Ext_zda_time_sec, (char*)ptr_Ext_Gpzda_Data->Ext_Zdatime + 4, 6);
                Ext_Zda_Time = (float)(atoi(Ext_zda_time_hour) * 60 * 60 + atoi(Ext_zda_time_min) * 60) + strtof(Ext_zda_time_sec, &pExt_zda_sec);  
                
                ptr_fpga_register_data->fpga_registers_parameters.Time_Date = atoi((char*)ptr_Ext_Gpzda_Data->Ext_Day);
                ptr_fpga_register_data->fpga_registers_parameters.Time_Date |= (atoi((char*)ptr_Ext_Gpzda_Data->Ext_Mounth) << 8);
                ptr_fpga_register_data->fpga_registers_parameters.Time_Year = atoi((char*)ptr_Ext_Gpzda_Data->Ext_Year);
                // printf("重新计算zda时间Ext_Zda_Time%f=%d*60*60+%d*60+%f\r\n",Ext_Zda_Time,atoi(Ext_zda_time_hour),atoi(Ext_zda_time_min),strtof(Ext_zda_time_sec, &pExt_zda_sec));
                
                if (ExtSensor_PPS_Num_Last != 0)
                {
                    if (Ext_Zda_Time - Last_Zda_Time > 0.9 && Ext_Zda_Time - Last_Zda_Time < 1.1)
                    {
                        PPS_status = 1;
                    }
                    else
                    {
                        PPS_status = 0;
                    }
                }
                Last_Zda_Time = Ext_Zda_Time;
                ExtSensor_PPS_Num_Last = ExtSensor_PPS_Num;
            }else {
                // printf("没有找到正确的ZDA数据头:%s 使用上一次的ZDA时间:%f\r\n",(char*)ptr_Ext_Gpzda_Data->Ext_GpzdaHead,Ext_Zda_Time);
                
                Ext_Zda_Time = Ext_Zda_Time;
            }


        }
        //计算传感器同步时间
        Sonar_Sync_Time = Ext_Zda_Time + (float)IQData_ParaHead_Last.TimeStamp / 1000.0 + (float)IQData_ParaHead_Last.PPS_Number - (float)ExtSensor_PPS_Num;//声学同步时间 ,单位是S，精度mS
        // printf("声学同步时间：Sonar_Sync_Time:[%f] = Ext_Zda_Time:[%f] + (float)IQData_ParaHead_Last.TimeStamp / 1000.0:[%f] + (float)IQData_ParaHead_Last.PPS_Number:%f - (float)ExtSensor_PPS_Num:%f\r\n",Sonar_Sync_Time,Ext_Zda_Time,(float)IQData_ParaHead_Last.TimeStamp / 1000.0,(float)IQData_ParaHead_Last.PPS_Number,(float)ExtSensor_PPS_Num )
        sonar_stime = (float)IQData_ParaHead_Last.TimeStamp / 1000.0 + (float)IQData_ParaHead_Last.PPS_Number;
        // printf("sonar_stime:[%f] = (float)IQData_ParaHead_Last.TimeStamp / 1000.0:[%f] + (float)IQData_ParaHead_Last.PPS_Number:[%f]\r\n",sonar_stime,(float)IQData_ParaHead_Last.TimeStamp / 1000.0,(float)IQData_ParaHead_Last.PPS_Number);
        
        ptr_fpga_register_data->fpga_registers_parameters.Time_Sec = (Sonar_Sync_Time - (int)floor(Sonar_Sync_Time)) + (int)(floor(Sonar_Sync_Time)) % 60;
        ptr_fpga_register_data->fpga_registers_parameters.Time_Min = ((int)(floor(Sonar_Sync_Time)) % 3600 - ((int)(floor(Sonar_Sync_Time)) % 3600) % 60) / 60;
        ptr_fpga_register_data->fpga_registers_parameters.Time_Hours = ((int)(floor(Sonar_Sync_Time)) - (int)(floor(Sonar_Sync_Time)) % 3600) / 3600; 
       
        
    }

	ptr_Ext_Gpzda_Data = NULL;
	my_free(ptr_Ext_Gpzda_Data);
	top = -1;
}

int roll_flag = 0;
/*****************************************************************************
 * * description : 获取外部MOTION TIME 与 ROll值
 * * param        {char*} ptr_mes
 * * param        {unsigned int} cnt
 * * return       {*}
 * * Date        : 2022-11-02 14:02:50
 * * Other
 ******************************************************************************/
void Get_MotionTime_Roll_Ext(char* ptr_mes,unsigned int cnt)
{
    float Motion_Time;
    char ext_roll[6];
    int roll_pps_cnt = 0;
    int roll_pps_ms_cnt = 0;
    static last_pps_cnt = 0;
    int i;
    unsigned int j;
	float roll;
	unsigned short utemp;
	short stemp;
	memset(ext_roll,0,sizeof(ext_roll));

#if 0
/*
    打印完整HAEDINIG or 姿态
*/
	unsigned char c;
    printf("\n==============TSS1 256byte:============================\r\n");
    unsigned char* date = (unsigned char*)Ext_Tss1_Data;
    for(i=0;i<SIZE_OF_ONE_PING_SENSOR;i++){
        c = date[i];
        printf("%02x ",c);
    }
    printf("\n==========================================\n\n");
#endif
#if 0
    printf("\n==============HEADING 256byte=========================\r\n");
    date = (unsigned char*)Ext_Handing_Data;
    for(i=0;i<SIZE_OF_ONE_PING_SENSOR;i++){
        c = date[i];
        printf("%02x ",c);
    }
    printf("\n==========================================\n\n");
#endif

    for (Recv_ExtSensorNum = 0; Recv_ExtSensorNum < cnt; ++Recv_ExtSensorNum)
    {
        if (*(ptr_mes + 16 + Recv_ExtSensorNum * 256) == 'q')
        {
            memset(ext_roll,0,sizeof(ext_roll));
            ext_roll[0] = *(ptr_mes + 16 + Recv_ExtSensorNum * 256 + 26*2);
            ext_roll[1] = *(ptr_mes + 16 + Recv_ExtSensorNum * 256 + 27*2);
            memcpy(&utemp, ext_roll, sizeof(unsigned short));
            stemp = (short)(MYSWAP16(utemp));
            roll = (float)stemp*180.0*3.051758e-05;//1/2^15
            roll_pps_cnt = *((int*)(ptr_mes + Recv_ExtSensorNum * 256) + 2);
            roll_pps_ms_cnt = *((int*)(ptr_mes + Recv_ExtSensorNum * 256) + 3) / 100000;
            last_pps_cnt = roll_pps_cnt;
            Motion_Time = (float)roll_pps_ms_cnt/1000.0 + (float)roll_pps_cnt;
            Motion_Time_Sync[Recv_ExtSensor_SumNum] = Motion_Time;
            Roll_Sequential_Value[Recv_ExtSensor_SumNum] = roll;
            Recv_ExtSensor_SumNum = Recv_ExtSensor_SumNum + 1;
        }
        else if(*(ptr_mes + 16 + Recv_ExtSensorNum * 256) == ':')
        {
            memset(ext_roll,0,sizeof(ext_roll));
            for (i = 0; i < 5; ++i)
            {
                ext_roll[i] = *(ptr_mes + 16 + Recv_ExtSensorNum * 256 + 2 * 14 + i * 2 );
            }
            if (ext_roll[0] == 32)
            {
                ext_roll[0] = 43;//如果找到的是空格就给个“+”
            }
            roll = atof(ext_roll);
            roll_pps_cnt = *((int*)(ptr_mes + Recv_ExtSensorNum * 256) + 2);
            roll_pps_ms_cnt = *((int*)(ptr_mes + Recv_ExtSensorNum * 256) + 3) / 100000;
            last_pps_cnt = roll_pps_cnt;
            Motion_Time = (float)roll_pps_ms_cnt/1000.0 + (float)roll_pps_cnt;
            Motion_Time_Sync[Recv_ExtSensor_SumNum] = Motion_Time;
            Roll_Sequential_Value[Recv_ExtSensor_SumNum] = roll / 100.0;//* 3.14 / 180;
            Recv_ExtSensor_SumNum = Recv_ExtSensor_SumNum + 1;
        }
        else
            continue;
    }
    for (j = 1; j < Recv_ExtSensor_SumNum; j++)
    {
        Roll_Time_Diff[j] = Motion_Time_Sync[j] - sonar_stime;
		if(Roll_Time_Diff[j] > 0)
		{
			ptr_fpga_register_data->fpga_registers_parameters.Roll_Value = ((Roll_Sequential_Value[j-1]+Roll_Sequential_Value[j])/2)*3.14/180;  
		} 
    }
    ptr_fpga_register_data->fpga_registers_parameters.Recv_SensorSumNum = Recv_ExtSensor_SumNum;
}

/*****************************************************************************
 * * description : 波束下放外置传感器处理程序
 * * return       {*}
 * * Date        : 2022-10-31 11:27:45
 * * Other
 ******************************************************************************/
static void Beam_Down_Ext_Mode(void)
{
    Get_SonarSyncTime_Ext(Ext_Gpzda_Data,Sensor1_Num);
    if ((Recv_ExtSensor_SumNum + Sensor3_Num) < 400)
    {
        Get_MotionTime_Roll_Ext(Ext_Tss1_Data,Sensor3_Num); 
    }
	if ((Recv_ExtSensor_SumNum + Sensor3_Num) >= 400)
    {
        float Temp_Save[400];
        if (Sensor3_Num < 400)
        {
            memcpy(Temp_Save, ((unsigned int *)(Roll_Time_Diff )+ Recv_ExtSensor_SumNum + Sensor3_Num - 400), (400 - Sensor3_Num)*4); 
			memcpy(Roll_Time_Diff, Temp_Save, (400 - Sensor3_Num)*4);

            memcpy(Temp_Save, ((unsigned int*)(Motion_Time_Sync)+Recv_ExtSensor_SumNum + Sensor3_Num - 400), (400 - Sensor3_Num) * 4);  
            memcpy(Motion_Time_Sync, Temp_Save, (400 - Sensor3_Num) * 4);  

            memcpy(Temp_Save,((unsigned int*) (Roll_Sequential_Value) + Recv_ExtSensor_SumNum + Sensor3_Num - 400), (400 - Sensor3_Num) * 4);
            memcpy(Roll_Sequential_Value, Temp_Save, (400 - Sensor3_Num)*4);

            Recv_ExtSensor_SumNum = 400 - Sensor3_Num;//目前已有Roll值的个数
        }
        else
        {
            Recv_ExtSensor_SumNum = 0;
            Recv_ExtSensor_SumNum = Sensor3_Num - 400;
        }
        Get_MotionTime_Roll_Ext(Ext_Tss1_Data,Sensor3_Num);  

    } 
}

void Copy_IQ_Extsensordata(void)
{
	memcpy(&Sensor1_Num,(char *)IQData+SIZE_OF_WET_SONAR_FIRST+8+ptr_IQData_ParaHead->SonarDataSize+4,4);
    if (Sensor1_Num > 100 || Sensor1_Num < 0)
        Sensor1_Num = 0;
	memcpy(&Sensor2_Num,(char *)IQData+SIZE_OF_WET_SONAR_FIRST+8+ptr_IQData_ParaHead->SonarDataSize+4+SIZE_OF_LONG*2+Sensor1_Num*SIZE_OF_ONE_PING_SENSOR,4);//2400 4字节时间位置长度原来在一起，后来分开，所以有4字节空 该sensor2为航向
    if (Sensor2_Num > 200 || Sensor2_Num < 0)
        Sensor2_Num = 0;
    memcpy(&Sensor3_Num,(char *)IQData+SIZE_OF_WET_SONAR_FIRST+8+ptr_IQData_ParaHead->SonarDataSize+4+SIZE_OF_LONG*3+(Sensor1_Num+Sensor2_Num)*SIZE_OF_ONE_PING_SENSOR,4);
    if (Sensor3_Num > 200 || Sensor3_Num < 0)
        Sensor3_Num = 0;
    memcpy(&Sensor4_Num,(char *)IQData+SIZE_OF_WET_SONAR_FIRST+8+ptr_IQData_ParaHead->SonarDataSize+4+SIZE_OF_LONG*4+(Sensor1_Num+Sensor2_Num+Sensor3_Num)*SIZE_OF_ONE_PING_SENSOR,4);
    if (Sensor4_Num > 200 || Sensor4_Num < 0)
        Sensor4_Num = 0;
    {
        my_copy(Ext_Gpzda_Data, IQData + SIZE_OF_WET_SONAR_FIRST+8 +ptr_IQData_ParaHead->SonarDataSize+4+4, Sensor1_Num * SIZE_OF_ONE_PING_SENSOR);   
        usleep(10);
        my_copy(Ext_Handing_Data, IQData + SIZE_OF_WET_SONAR_FIRST+8 +ptr_IQData_ParaHead->SonarDataSize+4+4+(Sensor1_Num * SIZE_OF_ONE_PING_SENSOR)+4+4, Sensor2_Num * SIZE_OF_ONE_PING_SENSOR);   
        usleep(10);
        my_copy(Ext_Tss1_Data, IQData + SIZE_OF_WET_SONAR_FIRST+8 + ptr_IQData_ParaHead->SonarDataSize+4+4+4+((Sensor1_Num + Sensor2_Num) * SIZE_OF_ONE_PING_SENSOR) + 4+4,Sensor3_Num * SIZE_OF_ONE_PING_SENSOR);    
    }
} 

/*****************************************************************************
 * * description :波束下放数据处理函数
 * * return       {*}
 * * Date        : 2022-10-20 13:09:32
 * * Other
 ******************************************************************************/
void Beam_Down_Data_Manage(void)
{
	char SendUpperTail[6] = { '0','0','E','D','>','>' };

	gettimeofday (&tv2, NULL);
	if (Sonar_FristPing_flag == 0) //第一ping`
	{
		Sonar_FristPing_flag = 1;
		Set_BeamDown_Fpga_Para();
		Set_SoundSpeed_Value();
		//该长度不包括<<ST及本身大小
		// IQDataTotalSize = IQDataSizeFromWetSend + 4 + ptr_send_to_upper_sensor->ExtSensorTotalSize + 4 + SIZE_OF_ONE_PING_SENSOR * Sensor4_Num;
		IQDataTotalSize = IQDataSizeFromWetSend;
		IQDataTotalSize_Last = IQDataTotalSize;
		memcpy(IQData + IQDataTotalSize - 6, SendUpperTail, 6);
		my_copy(IQData_Last, IQData, IQDataTotalSize_Last);//上1ping,此时已经压了2ping,湿端程序压了1ping
		Sensor1_Num = 0;
		Sensor2_Num = 0;
		Sensor3_Num = 0;
		Sensor4_Num = 0;
	}
	else
	{
        memcpy(&IQData_ParaHead_Last, IQData_Last, SIZE_OF_WET_SONAR_FIRST);//获取上1ping的PPS_CNT及PPS_NS_CNT
        memcpy(&IQData_ParaHead, IQData, SIZE_OF_WET_SONAR_FIRST);//获取当前ping的Ins_Mod
		if(ptr_recv_upper_package->INS_mod != Ins_Mod_State)//切惯导后清空同步buf
		{
			Ins_Mod_State = ptr_recv_upper_package->INS_mod;
			Change_InsModeState_ClearBuf();
		}
		Set_BeamDown_Fpga_Para();//coe svp angel
		memset(Ext_Gpzda_Data,0L,sizeof(Ext_Gpzda_Data));  
		memset(Ext_Tss1_Data,0L,sizeof(Ext_Tss1_Data));
		Copy_IQ_Extsensordata(); 
        if (Sensor1_Num > 0)
            GGAZDA_status =1;
        if (Sensor2_Num > 0)
            Heading_status = 1;
        if (Sensor3_Num > 0)
            TSS1_status = 1;
        if (Sensor4_Num > 0)
            SV_status = 1;                    
		Set_SoundSpeed_Value();
		Beam_Down_Ext_Mode();
        IQDataTotalSize = IQDataSizeFromWetSend;
        memcpy(IQData + IQDataTotalSize - 6, SendUpperTail, 6);//当前ping
		Set_Fpga_To_Dsp_Para();
        //Test_SYNC_Time_Roll_Func();//测试函数测试时候把fb打开
      //  Find_RollValueFromRollBuf();
        Set_Dsp_TransBuf();
        ptr_fpga_register_data->IQDataUpdata = 0;
        usleep(1000);
        ptr_fpga_register_data->IQDataUpdata = 1;
	//	sem_post(&sem_UPPER);
		Sensor1_Num = 0;
        Sensor2_Num = 0;
        Sensor3_Num = 0;
        Sensor4_Num = 0;
       // Get_Sonar_Sync_Time_Flag = 0;
        IQDataTotalSize_Last = IQDataTotalSize;
        my_copy(IQData_Last, IQData, IQDataTotalSize_Last);
    }
}

