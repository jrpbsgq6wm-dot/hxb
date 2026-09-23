/***************************
stm32f103c8t6
ROM 64k
RAM 20k
***************************/
#include <string.h>
#include "led.h"
#include "delay.h"
#include "main.h"
#include "tdc_gp22.h"
#include "exti.h"
#include "usart1.h"
#include "serial.h"
#include "spi.h"
#include "timer.h"
#include "stmflash.h"
#include "stm32f10x.h" 
#include "wdg.h"
#include "serial.h"

/*程序版本号*/
const u8 Program_Version_Buffer[]={"SVS1500_V2.0"};      
#define Version_Size sizeof(Program_Version_Buffer)/2
/*设备出厂时间结构体*/
FactoryTime factorytime;                                
uint8_t m_deviceID = 0xAA;                                  //设备地址 AA
uint32_t device_bound = 115200;                             //串口波特率 
uint8_t  device_serial_type = 0;                            //通讯方式 rs232 = 0;rs485 = 1
volatile uint8_t  device_dataformat = 0;                    /*数据格式   AML:<SR>1478.32<SR><CR><LF>   
                                                                        Valeport:1478320<CR><LF>  
                                                                        NMEA:$PRSOS,1350.000*78<CR><LF>  */
CircularBuffer circular_buffer;                             //声明环形缓冲区
tdc_pse_t tdc_pse;
ave_t svave;

//中断优先级设置
void set_NVIC_Config(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    //USART
    USART1_NVICConfiguration(0, 1);
    TIM4_NVICConfiguration(1, 0);
    //kalman
    TDC_GP22_NVICConfiguration(0, 0);
    //TDC
    TIM2_NVICConfiguration(2, 0);
    //SERIAL&output
    TIM3_NVICConfiguration(2, 1);
}
 
//SN号获取初始化
void program_version_num(void)
{
    uint16_t datatemp[Version_Size] = {0};
    STMFLASH_Write(APP_EDITION_ADDR,(u16*)Program_Version_Buffer,Version_Size);
    STMFLASH_Read(APP_EDITION_ADDR,(u16*)datatemp,Version_Size);
}
 

//出厂时间获取初始化
void factory_time(void){
    uint16_t temp[2] = {0};
    STMFLASH_Read(DEVICE_FACTORY_TIME_ADDR,(u16*)temp,2);
    factorytime.year = temp[0];
    factorytime.month = temp[1] & 0xFF;
    factorytime.data = (temp[1]>>8) & 0xFF;
}

//频率获取初始化
void freq_init(void){
    uint16_t read_sos_freq = 0;
    
    STMFLASH_Read(SOS_FREQ_ADDR,(u16 *)&read_sos_freq,1);
    if ( (1999 == read_sos_freq) || (4999 == read_sos_freq) || (9999 == read_sos_freq) || (3332 == read_sos_freq)){
        interrupt_comp = read_sos_freq;
    }
    else{
        interrupt_comp = 9999;
    }
}

/*  修改更新标志位(0x0800FC40)  -   UPDATA
 *   flag： 0xAA - 等待更新
 *          0x55 - 更新完成
 *          0x00 - 正常模式
 */
#include "serial.h"
void iap_app_flag(void){
    //修改更新标志位
    uint8_t updata_buf[2] = {0};
    STMFLASH_Read(UPDATA_FLAG,(u16 *)&updata_buf,1); 
    if(updata_buf[0] == 0x55 && updata_buf[1] == 0x55){ //bootloader已经完成了程序更新，并修改了标志位
        //向显控端返回更新成功消息
        uint16_t len = 0x03;
        uint16_t CRC_16 = 0;
        uint8_t CRC_LOW = 0,CRC_HIG = 0;
        memset(s_serialSendBuf,0,sizeof(s_serialSendBuf));
        s_serialSendBuf[0] = SERIAL_SEND_MSG_HEAD;
        s_serialSendBuf[1] = UP_DATA_ONLINE;
        s_serialSendBuf[2] = 0x03;
        CRC_16 = CRC16(s_serialSendBuf,len);
        CRC_LOW = CRC_16 & 0x00FF;
        CRC_HIG = (CRC_16 >> 8);
        s_serialSendBuf[len] = CRC_LOW;
        s_serialSendBuf[len+1] = CRC_HIG;
        s_serialSendBuf[len+2] = SERIAL_SEND_MSG_TAIL;
        USART1_Send_Data(s_serialSendBuf,len+2+1); 
        memset(s_serialSendBuf,0,sizeof(s_serialSendBuf));
    }
    /*将标志位置为正常状态*/
    updata_buf[0] = 0x00;
    updata_buf[1] = 0x00;
    STMFLASH_Write(UPDATA_FLAG,(u16*)updata_buf,1);
}

void function_initial(void)
{
    SystemInit();
    delay_init(72); 
    set_NVIC_Config();    
    usart1_init(device_bound);
 	LED_Init(); 
    LED0 = !LED0;
    delay_ms(5000);
    
    TDC_GP22_Init();
    TDC_Init_Reg();
    EXTIX_Init();
    
    factory_time();                 //获取出厂时间
    freq_init();                    //频率获取
    ParameterSet();                 //运行参数设置
    serialInit();  
    iap_app_flag();                 /*系统正常启动，更新标志位  -   UPDATA*/
    initBuffer(&circular_buffer);   //初始化环形缓冲区
    tdc_pse_init(&tdc_pse);         //初始化伪值处理
    Tdc_Ave_init(&svave);
}


int main(void)
{  
#if 1      
    function_initial();
    printf("SVS1500 INIT\r\n");
    printf("Sound_velocity_coefficient_A1=%f ",svca.Sound_velocity_coefficient_A1);
    printf("Sound_velocity_coefficient_B1=%f ",svca.Sound_velocity_coefficient_B1);
    printf("- %f ~ %f \r\n",scs.Sound_coe_scope_A1_B1_LL,scs.Sound_coe_scope_A1_B1_HL);
    printf("Sound_velocity_coefficient_A2=%f ",svca.Sound_velocity_coefficient_A2);
    printf("Sound_velocity_coefficient_B2=%f ",svca.Sound_velocity_coefficient_B2);
    printf("- %f ~ %f \r\n",scs.Sound_coe_scope_A2_B2_LL,scs.Sound_coe_scope_A2_B2_HL);
    printf("Sound_velocity_coefficient_A3=%f ",svca.Sound_velocity_coefficient_A3);
    printf("Sound_velocity_coefficient_B3=%f ",svca.Sound_velocity_coefficient_B3);
    printf("- %f ~ %f \r\n",scs.Sound_coe_scope_A3_B3_LL,scs.Sound_coe_scope_A3_B3_HL);
    printf("Source_distance : %f\r\n",Source_distance);
    printf("Outliers_threshold :%f\r\n",outliers_threshold);
    printf("Frist wave voltage :%hd mv\r\n",first_v);
    printf("factory_time %04d.%02d.%2d\r\n",factorytime.year,factorytime.month,factorytime.data);
    //测量晶振校准
    calibrateResonator(); 
    printf("Start_Cal_Resonator=%f\r\n",bytes_Cal); 
    printf("Time cal %d\n",interrupt_comp);
    TIM2_Int_Init(4999,719); //50ms
    TIM3_Int_Init(4999,719); //50ms
    TIM4_Int_Init(interrupt_comp,719); //10ms
    Init_IWDG(IWDG_Prescaler_32,3750);         //初始化独立看门狗,超时时间约为3s
    delay_us(10);
    while(1)
    {  
        serialProcess();        //串口接收检测处理
    }
#endif
}
