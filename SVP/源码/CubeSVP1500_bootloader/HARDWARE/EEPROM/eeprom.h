#ifndef __EEPROM__H__
#define __EEPROM__H__

#include "sys.h"
#include "iic.h"
#include "stdint.h"



//器件地址
#define EEPROM_WRITE_ADDR   0xA0       
#define EEPROM_READ_ADDR    0XA1
#define EEPROM_TIMEOUT      100

#define EEPROM_PAGE_NUM        256         //页大小 
#define EEPROM_PAGE_BYTE_SIZE   8   //每页的字节数

//SYS                                     AT24C02地址
//#define SVP_APP_EDITION_ADDR                      //app程序版本号
//#define SVP_BOOT_EDITION_ADDR                     //boot程序版本号
#define SVP_BOUND_ADDR                      0X00      //波特率
#define SVP_TYPE_ADDR                       0X04      //接口类型            -无用
#define SVP_FACTORY_DATA_ADDR               0X06      //出厂日期
#define SVP_SN_ADDR                         0X0A      //SN
#define SVP_PROBE_DISTANCE_ADDR             0X1A      //换能器挡板高度
#define SVP_OUTLIERS_THRESHOLD_ADDR         0X1E      //出入水检测阈值
#define SVP_KLAMAN_OBSERVATIONS_ADDR        0X22      //kalman滤波样本大小
#define SVP_FREQ_ADDR                       0X24      //声速测量频率        -无用
#define SOUND_VELOCITY_COE_A1_ADDR          0X28      //系数A1
#define SOUND_VELOCITY_COE_B1_ADDR          0X2C      //系数B1
#define SOUND_VELOCITY_COE_A2_ADDR          0X30      //系数A2
#define SOUND_VELOCITY_COE_B2_ADDR          0X34      //系数B2
#define SOUND_VELOCITY_COE_A3_ADDR          0X38      //系数A3
#define SOUND_VELOCITY_COE_B3_ADDR          0X3C      //系数B3
#define COE_USE_A2_B2                       0X40      //应用A1B1上限 A2B2下限 
#define COE_USE_A3_B3                       0X44      //应用A2B2上限 A3B3下限 
#define FRIST_WAVE_VOLTAGE                  0X48      //TDC第一波阈值电压配置
#define WRITE_RTC_ADDR                      0X4C      //RTC时间设置
#define RTC_FLAG_ADDR                       0X54      //RTC_FLAG
#define FILE_STATUS_ADDR                    0X55      //文件状态标志位
#define WORK_MODE_ADDR                      0X57      //自容模式的输出基准选择 0 压力 1频率
#define PA_COE_ADDR                         0X5D      //压力系数       
#define IAP_UPDATA_ADDR                     0X6D        //IAP

typedef struct {
    uint8_t date;
    uint8_t mon;
    uint16_t year;
}svp_Date_t;

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

typedef struct{
    uint8_t mode_flag;
    uint8_t set_flag;
    uint32_t value;
}work_mode_t;

#pragma pack(push ,1)   //修改对齐方式为1字节对齐
typedef struct{
    uint32_t        BOUND;                      //0
    uint16_t        TYPE;                       //4
    svp_Date_t      DATE;                       //6
    uint8_t         SN[16];                     //10
    float           PROBE_DISTANCE;             //26
    uint32_t        OUTLIERS_THRESHOLD;      //30
    uint16_t        KLAMAN_OBSERVATIONS;        //34
    uint32_t        FREQ_BOUND;                 //36
    float           SOUND_VELOCITY_COE_A1;      //40
    float           SOUND_VELOCITY_COE_B1;      //44
    float           SOUND_VELOCITY_COE_A2;      //
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
}svp_cmd_t;
extern svp_cmd_t svp_cmd;
#pragma pack(pop)



extern HAL_StatusTypeDef AT24C02_Write(uint16_t memAddress,uint8_t *pData,uint16_t size);
extern HAL_StatusTypeDef AT24C02_Read(uint16_t memAddress,uint8_t *pData,uint16_t size);
extern void AT24C02_EraseALL(void);
extern void SVP_CMD_INIT(svp_cmd_t *svp_cmd);
#endif


