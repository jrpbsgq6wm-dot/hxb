#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "serial.h"
#include "usart1.h"
#include "stmflash.h"
#include "tdc_gp22.h"
#include "timer.h"

uint8_t     s_serialRecvBuf[SERIAL_RX_BUFSIZE] = {0};         //串口发送接收缓冲区声明
uint8_t     s_serialSendBuf[SERIAL_RX_BUFSIZE] = {0};
uint8_t     s_Recvlength;  
unsigned char serial_read_over = 0;                     //串口接收标志位
uint8_t cal_ave_flag = 0;                               //标定模式平均打印

uint16_t CRC_16;                                        //CRC校验
char CRC_LOW;                                           //CRC低字节
char CRC_HIG;                                           //CRC高字节

/****************************************************
    设备运行参数设置，主要设置波特率，通讯方式，数据格式
*****************************************************/
void runParameterSet(void)
{
    uint32_t temp = 0;
    uint16_t tempBuf[2] = {0};
    
    STMFLASH_Read(DEVICE_BOUND_ADDR,(u16 *)tempBuf,2);//115200  0001 c200   
    temp = tempBuf[1];
    temp <<= 16;
    temp += tempBuf[0];
    if((temp == 2400) || (temp == 4800) || (temp == 9600)  || (temp == 19200)  || (temp == 38400) || (temp == 57600) || (temp == 115200))
    {
        ;
    }
    else
    {
        temp = 115200;
        tempBuf[0] = (uint16_t)temp;
        tempBuf[1] = (uint16_t)(temp>>16);
        STMFLASH_Write(DEVICE_BOUND_ADDR,(u16 *)tempBuf,2);
    }  
    
    STMFLASH_Read(DEVICE_TYPE_ADDR,(u16 *)tempBuf,1);
    temp = (tempBuf[0]  & 0x00ff);  
    if ((0 == temp) || (1 == temp))
    {
        ;
    }
    else
    {
        temp = device_serial_type;//rs232
        STMFLASH_Write(DEVICE_TYPE_ADDR,(u16 *)&temp,1);
    }
    
    STMFLASH_Read(DEVICE_FORMAT_ADDR,(u16 *)tempBuf,1);
    temp = (tempBuf[0] & 0x00ff);
    if ((0 == temp) || (1 == temp) || (2 == temp)|| (3 == temp)|| (4 == temp))
    {
         ;
    }
    else
    {
        temp = device_dataformat;//aml
        STMFLASH_Write(DEVICE_FORMAT_ADDR,(u16 *)&temp,1);
    }
}

//运行参数设置
void ParameterSet(void)
{
    runParameterSet();                  //设备运行参数设置 
    gp22_parameter_set();               //GP22参数设置 
}


//通讯参数出厂设置
void serialParameterReset(void)
{ 
    uint16_t tempBuf[2] = {0};
    
    device_bound = 115200;
    device_serial_type = 0x00;
    device_dataformat = 0x00;
    
    tempBuf[0] = (uint16_t)device_bound;
    tempBuf[1] = (uint16_t)(device_bound>>16);
       
    STMFLASH_Write(DEVICE_BOUND_ADDR,(u16 *)tempBuf,2);
    STMFLASH_Write(DEVICE_TYPE_ADDR,(u16 *)&device_serial_type,1);
    STMFLASH_Write(DEVICE_FORMAT_ADDR,(u16 *)&device_dataformat,1);
}

//gp1022 参数复位
void gp22ParameterReset(void)
{ 
    uint16_t tempBuf[2] = {0};
    uint32_t i = 0;
    uint32_t coe_addr = SOUND_VELOCITY_COE_A1_ADDR;
    uint32_t sco_addr = COE_USE_A1_B1_UL;
    
    Source_distance = 3.0f;
    memset(tempBuf, 0, 2); 
    tempBuf[0] = *((uint32_t*)&Source_distance);
    tempBuf[1] = *((uint32_t*)&Source_distance) >> 16;
    STMFLASH_Write(PROBE_DISTANCE_ADDR,(u16 *)tempBuf,2);
    
    /*阈值*/
    outliers_threshold = 10;
    memset(tempBuf, 0, 2);
    tempBuf[0] = *((uint16_t*)&outliers_threshold);
    tempBuf[1] = *((uint16_t*)&outliers_threshold) >> 16;
    STMFLASH_Write(OUTLIERS_THRESHOLD_ADDR,(u16 *)&tempBuf,2);
    
    /*系数*/
    for(i=0;i<SOUND_COE_SIZE;i++){
        memset(tempBuf, 0, 2); 
        tempBuf[0] = *((uint32_t*)&Sound_velocity_coe_def[i]);
        tempBuf[1] = *((uint32_t*)&Sound_velocity_coe_def[i]) >> 16;
        STMFLASH_Write(coe_addr,(u16 *)&tempBuf,2);
        coe_addr+=4;
    }
    
    /*应用系数范围*/
    for(i=0;i<SOUND_SCOPE_SIZE;i++){
        memset(tempBuf, 0, 2);       
        tempBuf[0] = *((uint32_t*)&Sound_coe_scope_def[i]);
        tempBuf[1] = *((uint32_t*)&Sound_coe_scope_def[i]) >> 16;
        STMFLASH_Write(sco_addr,(u16 *)&tempBuf,2);
        sco_addr+=4;
        
    }
    //printf("TDC_RESET OK \r\n");  
}
 
 
//出厂运行参数设置 - 恢复出厂设置
void ParameterReset(void)
{
    PauseTimer(TIM4);
    PauseTimer(TIM2);
    //擦除配置FLASH
    //Erase_Flash_Section(0x0800FC00);
    serialParameterReset();     //通讯相关参数出厂设置
    gp22ParameterReset();        //tdc相关参数出厂设置
    PauseTimer(TIM2);
    ResumeTimer(TIM4);
}

//串口初始化
void serialInit(void)
{
    uint32_t bound = 0;
    uint8_t type = 0;
    uint8_t formata = 0;
    uint16_t tempBuf[2] = {0};
    
    STMFLASH_Read(DEVICE_BOUND_ADDR,(u16 *)tempBuf,2);
    STMFLASH_Read(DEVICE_TYPE_ADDR,(u16 *)&type,1);
    STMFLASH_Read(DEVICE_FORMAT_ADDR,(u16 *)&formata,1);
    
    bound = tempBuf[1];
    bound <<= 16;
    bound += tempBuf[0];
    
    if ((2400 == bound) || (4800 == bound) || (9600 == bound) || (19200 == bound) 
        || (38400 == bound) || (57600 == bound) || (115200 == bound))
    {
        usart1_init(bound);
    }
    else
    {
        usart1_init(115200);
    }
    if ((0 == type) || (1 == type))
    {
         device_serial_type = type;
    }
    else
    {
        device_serial_type = 0;//rs232
    }
    if ((0 == formata) || (1 == formata) || (2 == formata)|| (3 == formata)|| (4 == formata))
    {
         device_dataformat = formata;

    }
    else
    {
        device_dataformat = 0;//aml
    }
}


/*  lenght : all words after lenght
    crc16  : all words before CRC

RX: deciceID(0xAA)  cmd  lenght  crc_low   crc_high  0x55 
TX: deciceID(0xAA)  cmd  lenght   device_bound(4)   device_serial_type  device_dataformat  crc_low  crc_high  0x55

https://www.lddgo.net/encrypt/crc
    串口接受指令判断函数
*/
void serialProcess(void)
{
    if (serial_read_over)
    {
        if (s_serialRecvBuf[0] == m_deviceID)   //确认ID是正确
        {
            switch (s_serialRecvBuf[1])         //[1] 为命令字
            {
                case READ_PARAM:                //读串口参数 0x01
                {
                    read_param();
                    break;
                }
                case WRITE_PARAM_BOUND:         //串口参数设置 
                {
                    write_param_bound();
                    break;
                }
                case WRITE_PARAM_RS:            //串口通讯方式设置    
                {
                    write_param_rs();
                    break;
                }
                case WRITE_PARAM_AML:			//串口发送数据格式
                {
                    write_param_aml();
                    break;
                }
                case READ_TDC_PARAM: 			//读当前芯片参数
                {
                    read_tdc_param();
                    break;
                }
                case READ_SCOPE:
                {
                    read_Scope();
                    break;
                }
                case SET_SCOPE:
                {
                    set_Scope();
                    break;
                }
                case WRITE_TDC_DISTANCE: 		//设置距离
                {
                    write_tdc_distance();
                    break;
                } 
                case WRITE_TDC_COE: 			//设置系数
                {
                    write_tdc_coe();
                    break;
                }
                case WRITE_TDC_THRESHOLD: 		//设置阈值
                {
                    write_tdc_threshold();
                    break;
                }
                
                case SET_FRIST_WAVE_VOLTAGE:
                {
                    set_frist_wave_v();
                    break;
                }
                case UP_DATA_ONLINE:      		//在线升级
                {
                    program_upgrade();
                    break;
                }
                case DEFAULT_INIT:				//恢复出厂设置
                {
                    ParameterReset();
                    break;
                }                
                case OUT_ORIGINAL_DATA:			//输出原始数据
                {
                    set_debug_data_out(0x0003);
                    break;
                }                
                case OUT_DEBUG_DATA:			//输出调试数据
                {
                    set_debug_data_out(0x0004);
                    break;
                }
                case START_OUTPUT:				//开始输出
                {
                    TIM_ClearFlag(TIM2,TIM_FLAG_Update);
                    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //允许定时器2更新中断
                    //TimerFlag = 1;
                    ResumeTimer(TIM2);  
                    TIM_Cmd(TIM2,ENABLE); //失能定时器2
                    
                    TIM_ClearFlag(TIM4,TIM_FLAG_Update);
                    TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE); //允许定时器2更新中断
                    //TimerFlag = 1;
                    ResumeTimer(TIM4);  
                    TIM_Cmd(TIM4,ENABLE); //失能定时器2
                    break;
                }
                case STOP_OUTPUT:				//停止输出
                {
                    TIM_ClearFlag(TIM2,TIM_FLAG_Update);
                    TIM_ITConfig(TIM2,TIM_IT_Update,DISABLE); //不允许定时器2更新中断
                    //TimerFlag = 0;
                    PauseTimer(TIM2);
                    TIM_Cmd(TIM2,DISABLE); //失能定时器2
                    
                    TIM_ClearFlag(TIM4,TIM_FLAG_Update);
                    TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE); //允许定时器2更新中断
                    //TimerFlag = 1;
                    ResumeTimer(TIM4);  
                    TIM_Cmd(TIM4,DISABLE); //失能定时器2
                    break;
                }
                case WRITE_FACTORY_TIME:{		//设置设备出厂时间
                    set_factory_time();
                    break;
                }
                case OUT_FACTORY_TIME:{		    //输出设备出厂时间
                    output_factory_time();
                    break;
                }
                case WRITE_DEVICE_SN:{			//写设备SN号
                    set_device_sn();
                    break;
                }
                case OUT_DEVICE_SN:{            //输出设备SN号
                    output_device_sn();
                    break;
                }
                case SET_SOS_FREQ:{            //设置声速测量频率
                    set_sos_freq_func();
                    break; 
                }
                case OUTPUT_SOS_FREQ: {           //输出声速测量频率
                    read_sos_freq_func();
                    break;
                } 
                case 0x99:{
                    cal_ave_flag =1;
                    char tempbuff[32];
                    sprintf(tempbuff,"Cal Ave AGIN \r\n");
                    USART1_Send_Data(tempbuff,strlen(tempbuff));
                    break;
                }
                default:
                    break;
                }
        }
        s_Recvlength = 0;
        serial_read_over = 0;       //清接收完成标识
    }
}

/*  串口信息
 *   AA 01 03 10 71 55
 *   55 01 09 00 C2 01 00 00 00 E9 42 55
 */
void read_param(void)
{
    uint8_t DaLength = 0;
    uint8_t temp_bound[4] = {0};
    uint8_t type = 0;
    uint8_t format = 0;
    uint16_t tempBuf[2] = {0};
    
    CRC_16 = CRC16(s_serialRecvBuf, 3);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    
    if ((CRC_LOW == s_serialRecvBuf[3]) && (CRC_HIG == s_serialRecvBuf[4]))
    {
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        STMFLASH_Read(DEVICE_BOUND_ADDR,(u16 *)tempBuf,2);
        STMFLASH_Read(DEVICE_TYPE_ADDR,(u16 *)&type,1);
        STMFLASH_Read(DEVICE_FORMAT_ADDR,(u16 *)&format,1);
        DaLength = s_serialRecvBuf[2];
        
        memset(s_serialSendBuf,0,sizeof(s_serialSendBuf));
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength + 6;
        
        temp_bound[0] = tempBuf[0];
        tempBuf[0] >>= 8;
        temp_bound[1] = tempBuf[0];
        temp_bound[2] = tempBuf[1];
        tempBuf[1] >>= 8;
        temp_bound[3] = tempBuf[1];
        s_serialSendBuf[DaLength]   = temp_bound[0];
        s_serialSendBuf[DaLength+1] = temp_bound[1];
        s_serialSendBuf[DaLength+2] = temp_bound[2];
        s_serialSendBuf[DaLength+3] = temp_bound[3];
        
        s_serialSendBuf[DaLength+4] = type;
        s_serialSendBuf[DaLength+5] = format;
   
        CRC_16 = CRC16(s_serialSendBuf,DaLength + 6);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[DaLength+6] = CRC_LOW;
        s_serialSendBuf[DaLength+7] = CRC_HIG;
        s_serialSendBuf[DaLength+8] = SERIAL_SEND_MSG_TAIL;

        USART1_Send_Data(s_serialSendBuf,DaLength+8 +1);
            
       TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
    }
}

/*  写波特率
 *   AA 10 07 00 C2 01 00 06 36 55
 *   55 10 07 00 C2 01 00 09 39 AA 
 */
void write_param_bound(void)
{
    uint8_t DaLength = 0;
    uint16_t write_boundBuf[2] = {0};
    uint16_t read_boundBuf[2] = {0};
    uint32_t bound = 0;
    
    CRC_16 = CRC16(s_serialRecvBuf, 7);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    
    if ((CRC_LOW == s_serialRecvBuf[7]) && (CRC_HIG == s_serialRecvBuf[8]))
    {
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        DaLength = s_serialRecvBuf[2];
        
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength;
        
        
        write_boundBuf[0] = (s_serialRecvBuf[4]<< 8 & 0xff00) | (s_serialRecvBuf[3] & 0x00ff);//c2 00
        write_boundBuf[1] = (s_serialRecvBuf[6]<< 8 & 0xff00) | (s_serialRecvBuf[5] & 0x00ff);//00 01
        
        STMFLASH_Write(DEVICE_BOUND_ADDR,(u16 *)write_boundBuf,2);
        STMFLASH_Read(DEVICE_BOUND_ADDR,(u16 *)read_boundBuf,2);  
        
        s_serialSendBuf[3] = read_boundBuf[0] & 0x00ff;
        s_serialSendBuf[4] = read_boundBuf[0] >> 8 & 0x00ff; 
        s_serialSendBuf[5] = read_boundBuf[1] & 0xffff;
        s_serialSendBuf[6] = read_boundBuf[1] >> 8 & 0x00ff;
        
        CRC_16 = CRC16(s_serialSendBuf,DaLength);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[DaLength] = CRC_LOW;
        s_serialSendBuf[DaLength+1] = CRC_HIG;
        s_serialSendBuf[DaLength+2] = SERIAL_SEND_MSG_TAIL;
        
        USART1_Send_Data(s_serialSendBuf,DaLength + 2 + 1 ); 

        bound = read_boundBuf[1];
        bound <<= 16;
        bound += read_boundBuf[0];
        
        if ((2400 == bound) || (4800 == bound) || (9600 == bound) || (19200 == bound) 
            || (38400 == bound) || (57600 == bound) || (115200 == bound))
        {
            usart1_init(bound);
        }
        else
        {
            usart1_init(115200);
        } 
       TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
    }        
}

/*  写串口工作模式（232/485）
 *   AA 11 04 00 73 39 55
 *   55 11 04 00 43 2D AA 
 */ 
void write_param_rs(void)//Not currently in used
{
    uint8_t DaLength = 0;
    uint16_t write_rs = 0;
    uint16_t read_rs = 0;
    
    CRC_16 = CRC16(s_serialRecvBuf, 4);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    
    if ((CRC_LOW == s_serialRecvBuf[4]) && (CRC_HIG == s_serialRecvBuf[5]))
    {
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        DaLength = s_serialRecvBuf[2];
        
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength;
           
        write_rs = s_serialRecvBuf[3];
        
        STMFLASH_Write(DEVICE_TYPE_ADDR,(u16 *)&write_rs,1);
        STMFLASH_Read(DEVICE_TYPE_ADDR,(u16 *)&read_rs,1);  
        
        s_serialSendBuf[3] = read_rs & 0x00ff;

        
        CRC_16 = CRC16(s_serialSendBuf,DaLength);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[DaLength] = CRC_LOW;
        s_serialSendBuf[DaLength+1] = CRC_HIG;
        s_serialSendBuf[DaLength+2] = SERIAL_SEND_MSG_TAIL;
        USART1_Send_Data(s_serialSendBuf,DaLength + 2 +1);  
        
        if ((0 == read_rs) || (1 == read_rs))
        {
            device_serial_type = read_rs;
        }
        else
        {
            device_serial_type = 0;//rs232
        }
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
    }      
}

/*  写输出数据格式
 *   AA 12 04 00 83 39 55 //aml
 *   55 12 04 00 B3 2D AA 
 */ 
void write_param_aml(void)
{
    uint8_t DaLength = 0;
    uint16_t write_aml = 0;
    uint16_t read_aml = 0;
    
    CRC_16 = CRC16(s_serialRecvBuf, 4);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    
    if ((CRC_LOW == s_serialRecvBuf[4]) && (CRC_HIG == s_serialRecvBuf[5]))
    { 
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        DaLength = s_serialRecvBuf[2];
        
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength;
           
        write_aml = s_serialRecvBuf[3];
        
        STMFLASH_Write(DEVICE_FORMAT_ADDR,(u16 *)&write_aml,1);
        STMFLASH_Read(DEVICE_FORMAT_ADDR,(u16 *)&read_aml,1);  
        
        s_serialSendBuf[3] = read_aml & 0x00ff;
        CRC_16 = CRC16(s_serialSendBuf,DaLength);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[DaLength] = CRC_LOW;
        s_serialSendBuf[DaLength+1] = CRC_HIG;
        s_serialSendBuf[DaLength+2] = SERIAL_SEND_MSG_TAIL;
        
        USART1_Send_Data(s_serialSendBuf,DaLength + 2 + 1); 
        
        if ((0 == read_aml) || (1 == read_aml) || (2 == read_aml)|| (3 == read_aml)|| (4 == read_aml))
        {
             device_dataformat = read_aml;
        }
        else
        {
            device_dataformat = 0;//aml
        }
        STMFLASH_Read(DEVICE_FORMAT_ADDR,(u16 *)&read_aml,1); 
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
                
    }      
}

/*  读取tdc参数
 *   响应帧 24byte 
 *   AA 20 03 08 21 55
 *   55 20 23 
        04 00 80 40     //距离
        00 00 80 3F     //A1
        04 00 80 40     //B1
        04 00 80 40     //A2
        04 00 80 40     //B2
        04 00 80 40     //A3
        00 00 00 00     //B3
        82 00 00 00     //阈值
        0A 00   
    69 02
    AA
 */
void read_tdc_param(void)
{
    uint16_t tempBuf[2] = {0};  
    uint8_t temp_distance[4] = {0};
    uint8_t temp_svs_coe[4] = {0};
    uint8_t temp_threshold[4] = {0};
    uint32_t i = 0,index = 0;
    CRC_16 = CRC16(s_serialRecvBuf, 3);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    
    if ((CRC_LOW == s_serialRecvBuf[3]) && (CRC_HIG == s_serialRecvBuf[4]))//
    {   
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        TIM_ITConfig(TIM2,TIM_IT_Update,DISABLE);
        
        s_serialSendBuf[index++] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[index++] = s_serialRecvBuf[1];
        index++;
        //挡片距离
        STMFLASH_Read(PROBE_DISTANCE_ADDR,(u16 *)tempBuf,2);
        temp_distance[0] = tempBuf[0]&0x00ff;
        tempBuf[0] >>= 8;
        temp_distance[1] = tempBuf[0]&0x00ff;
        temp_distance[2] = tempBuf[1]&0x00ff;
        tempBuf[1] >>= 8;
        temp_distance[3] = tempBuf[1]&0x00ff;
        s_serialSendBuf[index++] = temp_distance[0];
        s_serialSendBuf[index++] = temp_distance[1];
        s_serialSendBuf[index++] = temp_distance[2];
        s_serialSendBuf[index++] = temp_distance[3];
        memset(tempBuf,0,sizeof(tempBuf));
        
        /*系数 A1 B1 A2 B2 A3 B3*/
        for(i=0;i<6;i++){
            memset(tempBuf,0,sizeof(tempBuf));
            memset(temp_svs_coe,0,sizeof(temp_svs_coe));
            STMFLASH_Read(SOUND_VELOCITY_COE_A1_ADDR + (i*4),(u16 *)tempBuf,2);//3f 80 00 00
            temp_svs_coe[0] = tempBuf[0]&0x00ff;
            tempBuf[0] >>= 8;
            temp_svs_coe[1] = tempBuf[0]&0x00ff;
            temp_svs_coe[2] = tempBuf[1]&0x00ff;
            tempBuf[1] >>= 8;
            temp_svs_coe[3] = tempBuf[1]&0x00ff;
            s_serialSendBuf[index++] = temp_svs_coe[0];  //00
            s_serialSendBuf[index++] = temp_svs_coe[1];  //00
            s_serialSendBuf[index++] = temp_svs_coe[2];  //80
            s_serialSendBuf[index++] = temp_svs_coe[3];  //3f
        }
        
        //阈值
        STMFLASH_Read(OUTLIERS_THRESHOLD_ADDR,(u16 *)tempBuf,2);
        temp_threshold[0] = tempBuf[0]&0x00ff;
        tempBuf[0] >>= 8;
        temp_threshold[1] = tempBuf[0]&0x00ff;
        temp_threshold[2] = tempBuf[1]&0x00ff;
        tempBuf[1] >>= 8;
        temp_threshold[3] = tempBuf[1]&0x00ff;
        s_serialSendBuf[index++]  = temp_threshold[0];
        s_serialSendBuf[index++]  = temp_threshold[1];
        s_serialSendBuf[index++] = temp_threshold[2];
        s_serialSendBuf[index++] = temp_threshold[3];
        memset(temp_svs_coe,0,sizeof(temp_svs_coe));
        
        //第一波电压
        STMFLASH_Read(FRIST_WAVE_VOLTAGE,(u16 *)tempBuf,1);
        s_serialSendBuf[index++] = tempBuf[0]&0x00ff;
        tempBuf[0] >>= 8;
        s_serialSendBuf[index++] = tempBuf[0]&0x00ff;
        
        
        CRC_16 = CRC16(s_serialSendBuf,37);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[index++] = CRC_LOW;
        s_serialSendBuf[index++] = CRC_HIG;
        s_serialSendBuf[index++] = SERIAL_SEND_MSG_TAIL;
        
        s_serialSendBuf[2] = 37;
        
        USART1_Send_Data(s_serialSendBuf,40 ); 
        
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
        TIM_ITConfig(TIM2,TIM_IT_Update,DISABLE);
    }        
}


/*
    设置系数应用范围
    0 ~ 1300      00 00 00 00  ~ 00 80 A2 44
    1300 ~ 1350   00 80 A2 44  ~ 00 C0 A8 44
    1350 ~ 1600   00 C0 A8 44  ~ 00 00 C8 44
    指令帧
    AA 22 1B 00 80 A2 44 00 00 00 00 00 C0 A8 44 00 80 A2 44 00 00 C8 44 00 C0 A8 44 DC FF 55
    响应帧数
    55 22 1B 00 80 A2 44 00 00 00 00 00 C0 A8 44 00 80 A2 44 00 00 C8 44 00 C0 A8 44 95 A7 AA
*/
void set_Scope(void){
    uint16_t write_scopeBuf[2] = {0};
    uint16_t read_scopeBuf[2] = {0};
    uint32_t i,index = 0,scope_index = 3;
    
    CRC_16 = CRC16(s_serialRecvBuf, 27);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    if ((CRC_LOW == s_serialRecvBuf[27]) && (CRC_HIG == s_serialRecvBuf[28]))
    {
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        TIM_ITConfig(TIM2,TIM_IT_Update,DISABLE);
        
        s_serialSendBuf[index++] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[index++] = s_serialRecvBuf[1];
        index++;
        /*写FLASH A1 A2 B1 B2 C1 C2*/
        for(i=0;i<6;i++){
            memset(write_scopeBuf,0,sizeof(write_scopeBuf));
            write_scopeBuf[0] = s_serialRecvBuf[scope_index+1]<< 8 | s_serialRecvBuf[scope_index];
            write_scopeBuf[1] = s_serialRecvBuf[scope_index+3]<< 8 | s_serialRecvBuf[scope_index+2];
            scope_index+=4;
            STMFLASH_Write(COE_USE_A1_B1_UL + (i*4),(u16 *)write_scopeBuf,2);
        }
        /*读FLASH A1 A2 B1 B2 C1 C2*/
        for(i=0;i<6;i++){
            STMFLASH_Read(COE_USE_A1_B1_UL + (i*4),(u16 *)read_scopeBuf,2);  
            s_serialSendBuf[index++] = read_scopeBuf[0]  & 0x00ff;
            s_serialSendBuf[index++] = read_scopeBuf[0] >> 8 & 0xffff; 
            s_serialSendBuf[index++] = read_scopeBuf[1]  & 0x00ff;
            s_serialSendBuf[index++] = read_scopeBuf[1] >> 8 & 0xffff;
            memset(read_scopeBuf,0,sizeof(read_scopeBuf));
        }
        
        gp22_parameter_set();     //GP22参数设置 
        
        CRC_16 = CRC16(s_serialSendBuf,index+1);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[index++] = CRC_LOW;
        s_serialSendBuf[index++] = CRC_HIG;
        s_serialSendBuf[index++] = SERIAL_SEND_MSG_TAIL;

        s_serialSendBuf[2] = index-3;  
        
        USART1_Send_Data(s_serialSendBuf,30); 
        TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);        
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
    }
}


/*
    读取系数应用范围
    指令帧：
            AA 21 03 09 B1 55
    响应帧：- 55 21 1B 00 80 A2 44 14 FE A7 44 14 FE A7 44 29 E4 AB 44 29 E4 AB 44 00 00 C8 44 40 17 AA
*/
void read_Scope(void){
    uint16_t read_scopeBuf[2] = {0};
    uint32_t i={0},index = 0;
    
    CRC_16 = CRC16(s_serialRecvBuf, 3);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    if ((CRC_LOW == s_serialRecvBuf[3]) && (CRC_HIG == s_serialRecvBuf[4]))
    {

        s_serialSendBuf[index++] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[index++] = s_serialRecvBuf[1];
        index++;
        /*读FLASH A1 A2 B1 B2 C1 C2*/
        for(i=0;i<6;i++){
            STMFLASH_Read(COE_USE_A1_B1_UL + (i*4),(u16 *)read_scopeBuf,2);  
            s_serialSendBuf[index++] = read_scopeBuf[0]  & 0x00ff;
            s_serialSendBuf[index++] = read_scopeBuf[0] >> 8 & 0xffff; 
            s_serialSendBuf[index++] = read_scopeBuf[1]  & 0x00ff;
            s_serialSendBuf[index++] = read_scopeBuf[1] >> 8 & 0xffff;
            memset(read_scopeBuf,0,sizeof(read_scopeBuf));
        }
        CRC_16 = CRC16(s_serialSendBuf,27);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[index++] = CRC_LOW;
        s_serialSendBuf[index++] = CRC_HIG;
        s_serialSendBuf[index++] = SERIAL_SEND_MSG_TAIL;
        s_serialSendBuf[2] = index-3;  
        
        USART1_Send_Data(s_serialSendBuf,30); 

    }
}

//设置距离
void write_tdc_distance(void)
{
    uint8_t DaLength = {0};
    uint16_t write_distanceBuf[2] = {0};
    uint16_t read_distanceBuf[2] = {0};

    CRC_16 = CRC16(s_serialRecvBuf, 7);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    if ((CRC_LOW == s_serialRecvBuf[7]) && (CRC_HIG == s_serialRecvBuf[8]))
    {
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        DaLength = s_serialRecvBuf[2];
        
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength;    
        
        write_distanceBuf[0] = s_serialRecvBuf[4]<< 8 | s_serialRecvBuf[3];//00 04
        write_distanceBuf[1] = s_serialRecvBuf[6]<< 8 | s_serialRecvBuf[5];//40 80
        
        STMFLASH_Write(PROBE_DISTANCE_ADDR,(u16 *)write_distanceBuf,2);
        
        STMFLASH_Read(PROBE_DISTANCE_ADDR,(u16 *)read_distanceBuf,2);  
        
        s_serialSendBuf[3] = read_distanceBuf[0] & 0x00ff; //
        s_serialSendBuf[4] = read_distanceBuf[0] >> 8 & 0xffff; 
        s_serialSendBuf[5] = read_distanceBuf[1] & 0x00ff;
        s_serialSendBuf[6] = read_distanceBuf[1] >> 8 & 0xffff;

        gp22_parameter_set();     //GP22参数设置 
        
        
        CRC_16 = CRC16(s_serialSendBuf,DaLength);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[DaLength] = CRC_LOW;
        s_serialSendBuf[DaLength+1] = CRC_HIG;
        s_serialSendBuf[DaLength+2] = SERIAL_SEND_MSG_TAIL;
        
        USART1_Send_Data(s_serialSendBuf,DaLength + 2 +1); 
       TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
    }        
}


/*
    设置声速系数A1 A2 B1 B2 C1 C2
        AA 2B 1B 
            00 00 80 3F     //A1  1.0
            00 00 80 3F     //B1 
            00 00 80 3F     //A2
            00 00 80 3F     //B2
            00 00 80 3F     //A3
            00 00 80 3F     //B3
        83 01 
        55
AA 2B 1B 00 00 80 3F 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 80 3F 00 00 00 00 12 91 55
    返回设置成功
        55 2B 1B
            00 00 80 3F     //A1
            00 00 80 3F     //B1 
            00 00 80 3F     //A2
            00 00 80 3F     //B2
            00 00 80 3F     //A3
            00 00 80 3F     //B3
        CRCL CRCH
        AA 
55 2B 1B 00 00 80 3F 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 80 3F 00 00 00 00 EF CC AA       
*/

void write_tdc_coe(void)
{
    uint16_t write_coeBuf[2] = {0};
    uint16_t read_coeBuf[2] = {0};
    uint32_t i = 0,index = 0,coe_index = 3;
    
    CRC_16 = CRC16(s_serialRecvBuf, 27);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    if ((CRC_LOW == s_serialRecvBuf[27]) && (CRC_HIG == s_serialRecvBuf[28]))
    {
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        
        s_serialSendBuf[index++] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[index++] = s_serialRecvBuf[1];
        index++;
        /*写FLASH A1 A2 B1 B2 C1 C2*/
        for(i=0;i<6;i++){
            memset(write_coeBuf,0,sizeof(write_coeBuf));
            write_coeBuf[0] = s_serialRecvBuf[coe_index+1]<< 8 | s_serialRecvBuf[coe_index];
            write_coeBuf[1] = s_serialRecvBuf[coe_index+3]<< 8 | s_serialRecvBuf[coe_index+2];
            coe_index+=4;
            STMFLASH_Write(SOUND_VELOCITY_COE_A1_ADDR + (i*4),(u16 *)write_coeBuf,2);
        }
        /*读FLASH A1 A2 B1 B2 C1 C2*/
        for(i=0;i<6;i++){
            STMFLASH_Read(SOUND_VELOCITY_COE_A1_ADDR + (i*4),(u16 *)read_coeBuf,2);  
            s_serialSendBuf[index++] = read_coeBuf[0]  & 0x00ff;
            s_serialSendBuf[index++] = read_coeBuf[0] >> 8 & 0xffff; 
            s_serialSendBuf[index++] = read_coeBuf[1]  & 0x00ff;
            s_serialSendBuf[index++] = read_coeBuf[1] >> 8 & 0xffff;
            memset(read_coeBuf,0,sizeof(read_coeBuf));
        }
        
        gp22_parameter_set();     //GP22参数设置 
        
        CRC_16 = CRC16(s_serialSendBuf,index+1);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[index++] = CRC_LOW;
        s_serialSendBuf[index++] = CRC_HIG;
        s_serialSendBuf[index++] = SERIAL_SEND_MSG_TAIL;

        s_serialSendBuf[2] = index-3;  
        
        USART1_Send_Data(s_serialSendBuf,30);  
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
    }
}

//读取TCD阈值
void write_tdc_threshold(void)
{
    uint8_t DaLength = 0;
    uint16_t write_thresholdBuf[2] = {0};
    uint16_t read_thresholdBuf[2] = {0};
    
    CRC_16 = CRC16(s_serialRecvBuf, 7);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    
   
    
    if ((CRC_LOW == s_serialRecvBuf[7]) && (CRC_HIG == s_serialRecvBuf[8]))
    {
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        DaLength = s_serialRecvBuf[2];
        
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength;    
        
        write_thresholdBuf[0] = s_serialRecvBuf[4]<< 8 | s_serialRecvBuf[3];//00 82
        write_thresholdBuf[1] = s_serialRecvBuf[6]<< 8 | s_serialRecvBuf[5];//00 00
        
        STMFLASH_Write(OUTLIERS_THRESHOLD_ADDR,(u16 *)write_thresholdBuf,2);
        
        STMFLASH_Read(OUTLIERS_THRESHOLD_ADDR,(u16 *)read_thresholdBuf,2);  
        
        s_serialSendBuf[3] = read_thresholdBuf[0]  & 0x00ff;
        s_serialSendBuf[4] = read_thresholdBuf[0] >> 8 & 0xffff; 
        s_serialSendBuf[5] = read_thresholdBuf[1] & 0x00ff;
        s_serialSendBuf[6] = read_thresholdBuf[1]  >> 8 & 0xffff;
        
        uint16_t tempBuf[2];
        uint32_t temp_value;
        //读取flash中的默认阈值
        STMFLASH_Read(OUTLIERS_THRESHOLD_ADDR,(u16 *)tempBuf,2);
        temp_value = tempBuf[1];
        temp_value <<= 16;
        temp_value += tempBuf[0];
        outliers_threshold = *(uint32_t*)&temp_value;
        
         
        
        CRC_16 = CRC16(s_serialSendBuf,DaLength);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[DaLength] = CRC_LOW;
        s_serialSendBuf[DaLength+1] = CRC_HIG;
        s_serialSendBuf[DaLength+2] = SERIAL_SEND_MSG_TAIL;

        USART1_Send_Data(s_serialSendBuf,DaLength + 2+1); 
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
    }        
}


void set_debug_data_out(u16 value)
{
    uint8_t DaLength = 0;
    uint16_t write_aml = 0;
    uint16_t read_aml = 0;
    
    CRC_16 = CRC16(s_serialRecvBuf, 3);
    CRC_LOW = CRC_16&0x00ff;
    CRC_HIG = (CRC_16>>8);
    
    if ((CRC_LOW == s_serialRecvBuf[3]) && (CRC_HIG == s_serialRecvBuf[4]))
    { 
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        
        DaLength = s_serialRecvBuf[2];
        
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength;
           
        write_aml = value;
        device_dataformat = (u8 )value;
        
        STMFLASH_Write(DEVICE_FORMAT_ADDR,(u16 *)&write_aml,1);
        STMFLASH_Read(DEVICE_FORMAT_ADDR,(u16 *)&read_aml,1);  
        
        s_serialSendBuf[3] = read_aml & 0x00ff;

        CRC_16 = CRC16(s_serialSendBuf,DaLength);
        CRC_LOW = CRC_16&0X00FF;
        CRC_HIG = (CRC_16>>8);
        
        s_serialSendBuf[DaLength] = CRC_LOW;
        s_serialSendBuf[DaLength+1] = CRC_HIG;
        s_serialSendBuf[DaLength+2] = SERIAL_SEND_MSG_TAIL;

        USART1_Send_Data(s_serialSendBuf,DaLength + 2+1);  
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
    }   
}

/*
	设置设备出厂时间 2024.7.17\
    AA 36 07 E8 07 07 11 E7 01 55
    55 36 07 E8 07 07 11 E8 0E AA
*/
#if 1
void set_factory_time(void){
	uint8_t DaLength = 0;
    uint16_t write_factory_time_Buf[2] = {0};
    uint16_t read_factory_time_Buf[2] = {0};
	//CRC 
	CRC_16 = CRC16(s_serialRecvBuf, 7);
    CRC_LOW = CRC_16 & 0x00ff;
    CRC_HIG = (CRC_16 >> 8);
	//判断CRC校验	
	if ((CRC_LOW == s_serialRecvBuf[7]) && (CRC_HIG == s_serialRecvBuf[8])){
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        DaLength = s_serialRecvBuf[2];
        memset(s_serialSendBuf,0,sizeof(s_serialSendBuf));
        //填充返回帧缓冲区
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD; //55
        s_serialSendBuf[1] = s_serialRecvBuf[1];   //37
        s_serialSendBuf[2] = (uint8_t)DaLength;    //07 
        //取出串口接收缓冲区的出厂时间 写入到flash中
		write_factory_time_Buf[0] = s_serialRecvBuf[4]<< 8 | s_serialRecvBuf[3];//E8 07 -> 07 E8
        write_factory_time_Buf[1] = s_serialRecvBuf[6]<< 8 | s_serialRecvBuf[5];//07 11 -> 11 07
        STMFLASH_Write(DEVICE_FACTORY_TIME_ADDR,(u16 *)write_factory_time_Buf,2);
        //读取flash中写入的出厂时间，放入准备好的缓冲区 然后通过串口1发送回去
        STMFLASH_Read(DEVICE_FACTORY_TIME_ADDR,(u16 *)read_factory_time_Buf,2);                                                                                                                                                                                                                                                                                                                                                                                                                            
        s_serialSendBuf[3] = read_factory_time_Buf[0]  & 0x00ff;
        s_serialSendBuf[4] = read_factory_time_Buf[0] >> 8 & 0xffff;
        s_serialSendBuf[5] = read_factory_time_Buf[1] & 0x00ff;
        s_serialSendBuf[6] = read_factory_time_Buf[1]  >> 8 & 0xffff;
#if 0
printf("write_device_sn_Buf = 0x%04x\n",write_factory_time_Buf[0]);
printf("write_device_sn_Buf = 0x%04x\n",write_factory_time_Buf[1]);        
printf("read_device_sn_Buf = 0x%04x\n",read_factory_time_Buf[0]);
printf("read_device_sn_Buf = 0x%04x\n",read_factory_time_Buf[1]);
#endif
        CRC_16 = CRC16(s_serialSendBuf,DaLength);
        CRC_LOW = CRC_16 & 0x00FF;
        CRC_HIG = (CRC_16 >> 8);
        
        s_serialSendBuf[DaLength] = CRC_LOW;
        s_serialSendBuf[DaLength+1] = CRC_HIG;
        s_serialSendBuf[DaLength+2] = SERIAL_SEND_MSG_TAIL;
        
        USART1_Send_Data(s_serialSendBuf,DaLength + 2 + 1); 
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
	}
}
#endif

/*
	输出设备出厂时间 2024.7.17
    AA 37 03 07 D1 55
    55 37 07 E8 07 07 11 E9 DF AA
*/
void output_factory_time(void){
    uint8_t DaLength = 0;
    uint16_t read_factory_time_Buf[2] = {0};
	//CRC 
	CRC_16 = CRC16(s_serialRecvBuf, 3);
    CRC_LOW = CRC_16 & 0x00ff;
    CRC_HIG = (CRC_16 >> 8);
    
	if ((CRC_LOW == s_serialRecvBuf[3]) && (CRC_HIG == s_serialRecvBuf[4])){
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        DaLength = s_serialRecvBuf[2];
        //填充返回帧缓冲区
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength + 4;   
       
        //读取flash中写入的出厂时间，放入准备好的缓冲区 然后通过串口1发送回去
        STMFLASH_Read(DEVICE_FACTORY_TIME_ADDR,(u16 *)read_factory_time_Buf,2);                                                                                                                                                                                                                                                                                                                                                                                                                            
        s_serialSendBuf[3] = read_factory_time_Buf[0]  & 0x00ff;
        s_serialSendBuf[4] = read_factory_time_Buf[0] >> 8 & 0xffff;
        s_serialSendBuf[5] = read_factory_time_Buf[1] & 0x00ff;
        s_serialSendBuf[6] = read_factory_time_Buf[1]  >> 8 & 0xffff;
        
        CRC_16 = CRC16(s_serialSendBuf,DaLength + 4);
        CRC_LOW = CRC_16 & 0x00FF;
        CRC_HIG = (CRC_16 >> 8);

        s_serialSendBuf[DaLength+4] = CRC_LOW;
        s_serialSendBuf[DaLength+5] = CRC_HIG;
        s_serialSendBuf[DaLength+6] = SERIAL_SEND_MSG_TAIL;

        USART1_Send_Data(s_serialSendBuf,DaLength + 6 + 1); 
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
	}
}


/*
    设置设备SN号 - 20240703001
         (ASCII): 32 30 32 34 30 37 30 33 30 30 31    
    AA 38 0E   32 30 32 34 30 37 30 33 30 30 31   A3 DA 55
    55 38 0E   32 30 32 34 30 37 30 33 30 30 31   5C 25 55

    输出设备SN号 - 20240703001
         (ASCII): 32 30 32 34 30 37 30 33 30 30 31  
    AA 39 03 03 b1 55 
    55 39 0E 32 30 32 34 30 37 30 33 30 30 31 1B 12 AA 
*/
void set_device_sn(void){
    uint8_t DaLength = 0;
    uint16_t write_device_sn_Buf[6] = {0};
    uint16_t read_device_sn_Buf[6] = {0};
	//CRC 
	CRC_16 = CRC16(s_serialRecvBuf, 14);
    CRC_LOW = CRC_16 & 0x00ff;
    CRC_HIG = (CRC_16 >> 8);
    //判断CRC校验	
	if ((CRC_LOW == s_serialRecvBuf[14]) && (CRC_HIG == s_serialRecvBuf[15])){
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        DaLength = s_serialRecvBuf[2];
        //填充返回帧缓冲区
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength;     
        
		write_device_sn_Buf[0] = s_serialRecvBuf[4]<< 8 | s_serialRecvBuf[3];   // 30 32 
        write_device_sn_Buf[1] = s_serialRecvBuf[6]<< 8 | s_serialRecvBuf[5];   // 34 32
        write_device_sn_Buf[2] = s_serialRecvBuf[8]<< 8 | s_serialRecvBuf[7];   // 37 30 
        write_device_sn_Buf[3] = s_serialRecvBuf[10]<< 8 | s_serialRecvBuf[9];  // 33 30 
        write_device_sn_Buf[4] = s_serialRecvBuf[12]<< 8 | s_serialRecvBuf[11]; // 30 30
        write_device_sn_Buf[5] = 0x00 << 8 | s_serialRecvBuf[13];               // 00 31
        STMFLASH_Write(DEVICE_SN_ADDR,(u16 *)write_device_sn_Buf,6);
        
        STMFLASH_Read(DEVICE_SN_ADDR,(u16 *)read_device_sn_Buf,6);                                                                                                                                                                                                                                                                                                                                                                                                                            
        s_serialSendBuf[3] = read_device_sn_Buf[0]  & 0x00ff;                   // 32 30
        s_serialSendBuf[4] = read_device_sn_Buf[0] >> 8 & 0xffff;               // 32 34
        s_serialSendBuf[5] = read_device_sn_Buf[1] & 0x00ff;                    // 30 37
        s_serialSendBuf[6] = read_device_sn_Buf[1]  >> 8 & 0xffff;              // 30 33 
        s_serialSendBuf[7] = read_device_sn_Buf[2]  & 0x00ff;                   // 30 30
        s_serialSendBuf[8] = read_device_sn_Buf[2] >> 8 & 0xffff;               // 31 00
        s_serialSendBuf[9] = read_device_sn_Buf[3] & 0x00ff;
        s_serialSendBuf[10] = read_device_sn_Buf[3]  >> 8 & 0xffff;
        s_serialSendBuf[11] = read_device_sn_Buf[4]  & 0x00ff;
        s_serialSendBuf[12] = read_device_sn_Buf[4] >> 8 & 0xffff;
        s_serialSendBuf[13] = read_device_sn_Buf[5] & 0x00ff;
        
        CRC_16 = CRC16(s_serialSendBuf,DaLength);
        CRC_LOW = CRC_16 & 0x00FF;
        CRC_HIG = (CRC_16 >> 8);
        s_serialSendBuf[DaLength] = CRC_LOW;
        s_serialSendBuf[DaLength+1] = CRC_HIG;
        s_serialSendBuf[DaLength+2] = SERIAL_SEND_MSG_TAIL;
        
        USART1_Send_Data(s_serialSendBuf,DaLength + 2 +1 ); 
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
	}
}


/*
输出设备sn号
AA 39 03 03 b1 55 
55 38 0E 32 30 32 34 30 37 30 33 30 30 31 5C 25 55
*/
void output_device_sn(void){
    uint8_t DaLength = 0;
    uint16_t read_device_sn_Buf[7] = {0};
	//CRC 
	CRC_16 = CRC16(s_serialRecvBuf, 3);
    CRC_LOW = CRC_16 & 0x00ff;
    CRC_HIG = (CRC_16 >> 8);

	if ((CRC_LOW == s_serialRecvBuf[3]) && (CRC_HIG == s_serialRecvBuf[4])){
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        DaLength = s_serialRecvBuf[2];
        //填充返回帧缓冲区
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength+11;     
        //读取flash中写入的出厂时间，放入准备好的缓冲区 然后通过串口1发送回去
        STMFLASH_Read(DEVICE_SN_ADDR,(u16 *)read_device_sn_Buf,6);                                                                                                                                                                                                                                                                                                                                                                                                                            
        s_serialSendBuf[3] = read_device_sn_Buf[0]  & 0x00ff;
        s_serialSendBuf[4] = read_device_sn_Buf[0] >> 8 & 0xffff;
        s_serialSendBuf[5] = read_device_sn_Buf[1] & 0x00ff;
        s_serialSendBuf[6] = read_device_sn_Buf[1]  >> 8 & 0xffff;
        s_serialSendBuf[7] = read_device_sn_Buf[2]  & 0x00ff;
        s_serialSendBuf[8] = read_device_sn_Buf[2] >> 8 & 0xffff;
        s_serialSendBuf[9] = read_device_sn_Buf[3] & 0x00ff;
        s_serialSendBuf[10] = read_device_sn_Buf[3]  >> 8 & 0xffff;
        s_serialSendBuf[11] = read_device_sn_Buf[4]  & 0x00ff;
        s_serialSendBuf[12] = read_device_sn_Buf[4] >> 8 & 0xffff;
        s_serialSendBuf[13] = read_device_sn_Buf[5] & 0x00ff;
        
        CRC_16 = CRC16(s_serialSendBuf,DaLength + 14);
        CRC_LOW = CRC_16 & 0x00FF;
        CRC_HIG = (CRC_16 >> 8);
        
        s_serialSendBuf[DaLength+11] = CRC_LOW;
        s_serialSendBuf[DaLength+12] = CRC_HIG;
        s_serialSendBuf[DaLength+13] = SERIAL_SEND_MSG_TAIL;

        USART1_Send_Data(s_serialSendBuf,DaLength + 13 + 1);  //3+3+11+2=19
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
	}
}

/*
	设置声速测量频率  20Hz(50ms) 50Hz(20ms) 100Hz(100ms)
    arr                  1999        4999        9999
    帧头 AA
    功能码 2E
    长度 05
    内容 //999 - E7 03
        1999 - CF 07
        4999 - 87 13
        9999 - 0F 27
    CRC //2B 30
        34 F3
        02 FC
        65 2B
    帧尾 55
    //指令帧 AA 2E 05 E7 03 2E C0 55  
    //响应帧 55 2E 05 E7 03 3A D4 0D
*/
void set_sos_freq_func(void){
    uint8_t DaLength = 0;
    uint16_t write_sos_freq = 0;
    uint16_t read_sos_freq = 0;
    uint16_t freq = 0;
	//CRC 
	CRC_16 = CRC16(s_serialRecvBuf, 5);
    CRC_LOW = CRC_16 & 0x00ff;
    CRC_HIG = (CRC_16 >> 8);
	//判断CRC校验	
	if ((CRC_LOW == s_serialRecvBuf[5]) && (CRC_HIG == s_serialRecvBuf[6])){
        //PauseTimer(TIM2);
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        DaLength = s_serialRecvBuf[2];

        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength;    
        //将自动重装载值存入flash中
        write_sos_freq = s_serialRecvBuf[4]<< 8 | s_serialRecvBuf[3];
        STMFLASH_Write(SOS_FREQ_ADDR,(u16 *)&write_sos_freq,1);
        //在flash中获取自动重装载值放入串口发送缓冲区中
        STMFLASH_Read(SOS_FREQ_ADDR,(u16 *)&read_sos_freq,1);                                                                                                                                                                                                                                                                                                                                                                                                                            
        //修改定时器2的自动重装载值，启动定时器后生效
        freq = read_sos_freq;
        switch(freq)
		{
			case 1999: // 50Hz ? (PSC+1)*(ARR+1)=1440000 ? PSC=99, ARR=14399 - 20ms
				TIM_PrescalerConfig(TIM4, 99, TIM_PSCReloadMode_Immediate);
				TIM_SetAutoreload(TIM4, 14399);
				break;
			case 4999: // 20Hz ? (PSC+1)*(ARR+1)=3600000 ? PSC=499, ARR=7199 - 50ms
				TIM_PrescalerConfig(TIM4, 499, TIM_PSCReloadMode_Immediate);
				TIM_SetAutoreload(TIM4, 7199);
				break;
			case 999:  // 10Hz ? (PSC+1)*(ARR+1)=7200000 ? PSC=999, ARR=7199 - 100ms
				TIM_PrescalerConfig(TIM4, 999, TIM_PSCReloadMode_Immediate);
				TIM_SetAutoreload(TIM4, 7199);
				break;
			case 3332: // 30Hz ? (PSC+1)*(ARR+1)=2400000 ? PSC=199, ARR=11999 - 30ms
				TIM_PrescalerConfig(TIM4, 199, TIM_PSCReloadMode_Immediate);
				TIM_SetAutoreload(TIM4, 11999);
				break;
			case 1:    // 1Hz  ? (PSC+1)*(ARR+1)=72000000 ? PSC=1099, ARR=65453 - 1s
				TIM_PrescalerConfig(TIM4, 1099, TIM_PSCReloadMode_Immediate);
				TIM_SetAutoreload(TIM4, 65453);
				break;
			case 2:    // 2Hz  ? (PSC+1)*(ARR+1)=36000000 ? PSC=999, ARR=35999 - 500ms
				TIM_PrescalerConfig(TIM4, 999, TIM_PSCReloadMode_Immediate);
				TIM_SetAutoreload(TIM4, 35999);
				break;
			case 5:    // 5Hz  ? (PSC+1)*(ARR+1)=14400000 ? PSC=999, ARR=14399 - 200ms
				TIM_PrescalerConfig(TIM4, 999, TIM_PSCReloadMode_Immediate);
				TIM_SetAutoreload(TIM4, 14399);
				break;
			default: // 100ms
				TIM_PrescalerConfig(TIM4, 999, TIM_PSCReloadMode_Immediate);
				TIM_SetAutoreload(TIM4, 7199);
				break;
		}
        
        CRC_16 = CRC16(s_serialSendBuf,DaLength);
        CRC_LOW = CRC_16 & 0x00FF;
        CRC_HIG = (CRC_16 >> 8);
        
        s_serialSendBuf[DaLength] = CRC_LOW;
        s_serialSendBuf[DaLength+1] = CRC_HIG;
        s_serialSendBuf[DaLength+2] = SERIAL_SEND_MSG_TAIL;

        USART1_Send_Data(s_serialSendBuf,DaLength + 2 +1 ); 
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
        //ResumeTimer(TIM2);
	}
}


/*
    读取声速测量频率
    AA 2D 03 0C B1 55
    55 2D 05 0F 27 67 67 AA (9999)
*/
void read_sos_freq_func(void){
    uint8_t DaLength = 0;
    uint16_t read_sos_freq_Buf = 0;
	//CRC 
	CRC_16 = CRC16(s_serialRecvBuf, 3);
    CRC_LOW = CRC_16 & 0x00ff;
    CRC_HIG = (CRC_16 >> 8);
	//判断CRC校验	
    //printf("0x%02X  0x%02X\n",CRC_LOW,CRC_HIG);
    //printf("0x%02X  0x%02X",s_serialRecvBuf[3],s_serialRecvBuf[4]);
	if ((CRC_LOW == s_serialRecvBuf[3]) && (CRC_HIG == s_serialRecvBuf[4])){
        DaLength = s_serialRecvBuf[2];
        //填充返回帧缓冲区
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = s_serialRecvBuf[1];
        s_serialSendBuf[2] = (uint8_t)DaLength + 2;      
      
        STMFLASH_Read(SOS_FREQ_ADDR,(u16 *)&read_sos_freq_Buf,1);                                                                                                                                                                                                                                                                                                                                                                                                                            
        s_serialSendBuf[3] = read_sos_freq_Buf & 0x00ff;
        s_serialSendBuf[4] = read_sos_freq_Buf >> 8 & 0xffff;
        
        CRC_16 = CRC16(s_serialSendBuf,DaLength + 4);
        CRC_LOW = CRC_16 & 0x00FF;
        CRC_HIG = (CRC_16 >> 8);

        s_serialSendBuf[DaLength+2] = CRC_LOW;
        s_serialSendBuf[DaLength+3] = CRC_HIG;
        s_serialSendBuf[DaLength+4] = SERIAL_SEND_MSG_TAIL;

        USART1_Send_Data(s_serialSendBuf,DaLength + 4 + 1); 
	}
}

/*  程序升级
 *   AA 30 03 05 E1 55
 *   55 30 03 35 D1 AA
 */
void program_upgrade(void){
    uint8_t buf[2] = {0};     //定义更新标志位指令缓冲区
    
	CRC_16 = CRC16(s_serialRecvBuf, 3);
    CRC_LOW = CRC_16 & 0x00ff;
    CRC_HIG = (CRC_16 >> 8);
    if ((CRC_LOW == s_serialRecvBuf[3]) && (CRC_HIG == s_serialRecvBuf[4])){     /*判断crc校验*/
        TIM_ClearFlag(TIM2,TIM_FLAG_Update);
        TIM_ITConfig(TIM2,TIM_IT_Update,DISABLE); //不允许定时器2更新中断
        //TimerFlag = 0;
        PauseTimer(TIM2);
        TIM_Cmd(TIM2,DISABLE); //失能定时器2
        
        TIM_ClearFlag(TIM4,TIM_FLAG_Update);
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE); //不允许定时器2更新中断
        //TimerFlag = 0;
        PauseTimer(TIM4);
        TIM_Cmd(TIM4,DISABLE); //失能定时器2
        
        //设置标志位为更新状态
        buf[1] = 0xAA;
        buf[0] = 0xAA;
        STMFLASH_Write(UPDATA_FLAG,(u16 *)&buf,1);
        uint16_t valie = 1;
        STMFLASH_Write(UPDATA_BOOT_FLAG,(u16 *)&valie,1);
        
        __disable_irq();    //屏蔽系统中断
        
        NVIC_SystemReset();     //重启系统 -> 进入boot程序
        //ve_file_data(COMMTYPE_RS485);
    }
}



/*设置芯片第一波电压*/
void set_frist_wave_v(void){
    uint16_t frist_v = 0;
    
	CRC_16 = CRC16(s_serialRecvBuf, 5);
    CRC_LOW = CRC_16 & 0x00ff;
    CRC_HIG = (CRC_16 >> 8);
    if ((CRC_LOW == s_serialRecvBuf[5]) && (CRC_HIG == s_serialRecvBuf[6])){     /*判断crc校验*/
        TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
        //设置标志位为更新状态
        frist_v = s_serialRecvBuf[4]<< 8 | s_serialRecvBuf[3];
        STMFLASH_Write(FRIST_WAVE_VOLTAGE,(u16 *)&frist_v,1);
        
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;  //aa
        s_serialSendBuf[1] = s_serialRecvBuf[1];    //2f
        s_serialSendBuf[2] = 0x05;                   //05
        s_serialSendBuf[3] = s_serialRecvBuf[3];    //0A
        s_serialSendBuf[4] = s_serialRecvBuf[4];    //00
        
        CRC_16 = CRC16(s_serialSendBuf,5);
        CRC_LOW = CRC_16 & 0x00FF;
        CRC_HIG = (CRC_16 >> 8);
        
        s_serialSendBuf[5] = CRC_LOW;               //cl
        s_serialSendBuf[6] = CRC_HIG;               //ch
        s_serialSendBuf[7] = 0xAA;  //aa
        
        USART1_Send_Data(s_serialSendBuf,8); 
        TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
        delay_ms(1000);
    __disable_irq();    //屏蔽系统中断
    
    NVIC_SystemReset();     //重启系统 -> 进入boot程序
        //ve_file_data(COMMTYPE_RS485);
    }
}

/*------------------------------------------------------------------
** 函数原型: int CRC16 (const char *nData, int wLength)
** 功能描述: CRC16校验
** 输入参数: const char *nData, int wLength
** 调用模块: None
** 返 回 值: wCRCWord
** 说    明：
-------------------------------------------------------------------*/
static const int wCRCTable[] =
{
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

int CRC16(const void *_nData, uint16_t wLength)
{
    const char *nData;
    unsigned char nTemp;
    unsigned char CRCHI = 0XFF;
    unsigned char CRCLO = 0XFF;
    nData = (const char *)_nData;
    while (wLength--) 
    {
        nTemp = CRCHI ^ (*(nData++)); 	
        CRCHI = CRCLO ^ wCRCTable[nTemp]; 		
        CRCLO = wCRCTable[nTemp]>>8; 
    }
    return ((unsigned short)CRCLO << 8 | CRCHI);
} // End: CRC16


