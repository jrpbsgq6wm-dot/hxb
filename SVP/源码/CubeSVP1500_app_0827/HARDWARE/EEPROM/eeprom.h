#ifndef __EEPROM__H__
#define __EEPROM__H__

#include "sys.h"
#include "iic.h"
#include "stdint.h"


//������ַ
#define EEPROM_WRITE_ADDR   0xA0       
#define EEPROM_READ_ADDR    0XA1
#define EEPROM_TIMEOUT      100

#define EEPROM_PAGE_NUM        256         //ҳ��С 
#define EEPROM_PAGE_BYTE_SIZE   8   //ÿҳ���ֽ���

//SYS                                AT24C02��ַ
//#define SVP_APP_EDITION_ADDR                      //app����汾��
//#define SVP_BOOT_EDITION_ADDR                     //boot����汾��
#define SVP_BOUND_ADDR                      0X00      //������
#define SVP_TYPE_ADDR                       0X04      //�ӿ�����            -����
#define SVP_FACTORY_DATA_ADDR               0X06      //��������
#define SVP_SN_ADDR                         0X0A      //SN
#define SVP_PROBE_DISTANCE_ADDR             0X1A      //����������߶�
#define SVP_OUTLIERS_THRESHOLD_ADDR         0X1E      //����ˮ�����ֵ
#define SVP_KLAMAN_OBSERVATIONS_ADDR        0X22      //kalman�˲�������С
#define SVP_FREQ_ADDR                       0X24      //���ٲ���Ƶ��        -����
#define SOUND_VELOCITY_COE_A1_ADDR          0X28      //ϵ��A1
#define SOUND_VELOCITY_COE_B1_ADDR          0X2C      //ϵ��B1
#define SOUND_VELOCITY_COE_A2_ADDR          0X30      //ϵ��A2
#define SOUND_VELOCITY_COE_B2_ADDR          0X34      //ϵ��B2
#define SOUND_VELOCITY_COE_A3_ADDR          0X38      //ϵ��A3
#define SOUND_VELOCITY_COE_B3_ADDR          0X3C      //ϵ��B3
#define COE_USE_A2_B2                       0X40      //Ӧ��A1B1���� A2B2���� 
#define COE_USE_A3_B3                       0X44      //Ӧ��A2B2���� A3B3���� 
#define FRIST_WAVE_VOLTAGE                  0X48      //TDC��һ����ֵ��ѹ����
#define WRITE_RTC_ADDR                      0X4C      //RTCʱ������
#define RTC_FLAG_ADDR                       0X54      //RTC_FLAG
#define FILE_STATUS_ADDR                    0X55      //�ļ�״̬��־λ
#define WORK_MODE_ADDR                      0X57      //����ģʽ�������׼ѡ�� 0 ѹ�� 1Ƶ��
#define PA_COE_ADDR                         0X5D      //ѹ��ϵ��                   
#define IAP_UPDATA_ADDR                     0X6D      //IAP
#define LTC2943_ACR_ADDR                    0x6E      //������mAh
#define TEMPERATURE_COE_ADDR                0x70      //�¶�ϵ�� 
#define SV_SN_ADDR                          0x80      //�������к�
#define TEMP_SN_ADDR                        0x88      //�¶����к�
#define PRESSURE_SN_ADDR                    0x90      //ѹ�����к�
#define BUTTERY_CAL_ADDR                    0x98      //��ص�ѹУ׼ 1��enable 
#define DEPTH_EC                            0x99      //ѹ�����Ȳ��� 2*4��float��=8 0xA3
//161
//162

/*�������� cmd*/
typedef struct {
    uint8_t date;
    uint8_t mon;
    uint16_t year;
}svp_Date_t;
/*RTC����ʱ�� cmd*/
typedef struct{
    uint8_t ampm;
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day;
    uint8_t date;
    uint8_t mon;
    uint8_t year;
}RTC_DT_T;
/*����ģʽ cmd*/
typedef struct{
    uint8_t mode_flag;
    uint8_t set_flag;
    uint32_t value;
}work_mode_t;

#pragma pack(push ,1)   //�޸Ķ��뷽ʽΪ1�ֽڶ���
typedef struct{
    uint32_t        BOUND;            			         
    uint16_t        TYPE;                      
    svp_Date_t      DATE;                      
    uint8_t         SN[16];                     
    float           PROBE_DISTANCE;            
    uint32_t        OUTLIERS_THRESHOLD;     
    uint16_t        KLAMAN_OBSERVATIONS;        
    uint32_t        FREQ_BOUND;                
    float           SOUND_VELOCITY_COE_A1;      
    float           SOUND_VELOCITY_COE_B1;      
    float           SOUND_VELOCITY_COE_A2;      
    float           SOUND_VELOCITY_COE_B2;
    float           SOUND_VELOCITY_COE_A3;
    float           SOUND_VELOCITY_COE_B3;
    float           COE_2_SPOCE;
    float           COE_3_SPOCE; 
    uint32_t        FRIST_WAVE_V;
    RTC_DT_T        RTC_DATE_TIME;
    uint8_t         RTC_FLAG;
    uint16_t        FILE_STATUS;
    uint8_t         WORK_MODE_FLAG;
    uint8_t         WORK_SET_MODE_FLAG;
    uint32_t        WORK_VALUE;
    float           PA_COE_A;    
    float           PA_COE_E;
    float           PA_COE_I;
    float           PA_COE_M;
    uint8_t         IAP_FLAG;
    uint16_t        FULL_BATTERY_MAH;
    float           TEMP_COE_A;
    float           TEMP_COE_B;
    float           TEMP_COE_C;
    float           TEMP_COE_D;
    char            SV_SN_BUF[8];
    char            TEMP_SN_BUF[8];
    char            PRESSURE_SN_BUF[8];
    uint8_t         BATTERY_CAL_FLAG;
    float           DEPTH_EC_X;
    float           DEPTH_EC_Y;
}svp_cmd_t;
extern svp_cmd_t svp_cmd;
#pragma pack(pop)



extern HAL_StatusTypeDef AT24C02_Write(uint16_t memAddress,uint8_t *pData,uint16_t size);
extern HAL_StatusTypeDef AT24C02_Read(uint16_t memAddress,uint8_t *pData,uint16_t size);
extern void AT24C02_EraseALL(void);
extern void SVP_CMD_INIT(svp_cmd_t *svp_cmd);
extern void at24c02_reset(void);
#endif


