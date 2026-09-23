#ifndef __RTC__H__
#define __RTC__H__

#include "sys.h"
#include "iic.h"
#include "eeprom.h"


#define DS3231_ADDR     0XD0

#define DS3231_SEC      0X00//秒
#define DS3231_MIN      0X01//分
#define DS3231_HOUR     0X02//时
#define DS3231_DAY      0X03//星期
#define DS3231_DATE     0X04//日
#define DS3231_MONTH    0X05//月
#define DS3231_YEAR     0X06//年
//闹钟1
#define DS3231_AL1SEC   0X07//
#define DS3231_AL1MIN   0X08
#define DS3231_AL1HOUR  0X09
#define DS3231_AL1DAY   0X0A
//闹钟2
#define DS3231_AL2MIN   0X0B
#define DS3231_AL2HOUR  0X0B
#define DS3231_AL2DAY   0X0D
#define DS3231_CONTROL  0X0E    //控制寄存器
#define DS3231_STATUS   0X0F    //状态寄存器

#define BSY     2//BUSY
#define OSF     7//振荡器停止标志
#define DS3231_XTAL 0x10    //晶体老化标志位
#define DS3231_TEMP_H   0X11    //温度寄存器高8
#define DS3231_TEMP_L   0X12    //温度寄存器低字节 8位中的高2

typedef enum{
    SECONDS=0,
    MINUTES,
    HOURS,
    DAY,
    DATE,
    MONTH_CENTURY,
    YEAR,
    ALARM_1_SECONDS,
    ALARM_1_MINUTES,
    ALARM_1_HOURS,
    ALARM_1_DAY_DATE,
    ALARM_2_MINUTES,
    ALARM_2_HOURS,
    ALARM_2_DAY_DATE,
    CONTROL,
    CONTROL_STATUS,
    AGING_OFFSET,
    TEMP_MSB,
    TEMP_LSB
}DS3231_REG;

/*小时制*/
typedef enum{
    HOUR_FORM_24,
    HOUR_FORM_12
}DS3231_HOUR_FORM;

/*上午 下午*/
typedef enum{
    AM,
    PM
}AM_PM;

/*DS3231寄存器结构体*/
typedef struct{
    uint8_t Seconds;
    uint8_t Minutes;
    uint8_t Hours;
    uint8_t Day;
    uint8_t Date;
    uint8_t Month_Century;
    uint8_t Year;
    
    uint8_t Alarm_1_Seconds;
    uint8_t Alarm_1_Minutes;
    uint8_t Alarm_1_Hours;
    uint8_t Alarm_1_Day_Date;
    
    uint8_t Alarm_2_Minutes;
    uint8_t Alarm_2_Hours;
    uint8_t Alarm_2_Day_Date;
    
    uint8_t Control;
    uint8_t Control_Status;
    uint8_t Aging_offset;
    uint8_t Temp_MSB;
    uint8_t Temp_LSB;
}DS3231_RegisterType;

typedef struct{
    uint8_t hour;
    uint8_t AM_PM;
    DS3231_HOUR_FORM hour_form;
    uint8_t min;
    uint8_t sec;
    uint8_t year;
    uint8_t mon;
    uint8_t date;
    uint8_t day;
}DS3231_TimeType;

/*DS3231结构体缓存*/
extern volatile uint8_t DS3231_buffer[19];
/*DS3231全局变量*/
extern DS3231_RegisterType DS3231_Register;
/*时间全局变量*/
extern DS3231_TimeType DS3231_Time;


extern void DS3231_Read_All(void);
extern void DS3231_Read_Time(void);
extern float DS3231_Read_temp(void);
extern void DS3231_Init(uint8_t year,uint8_t mon,uint8_t date,uint8_t day,uint8_t hour,uint8_t min,uint8_t sec,uint8_t AM_PM);
extern void DS3231_Set_Time(DS3231_TimeType *time);
extern void DS3231_Update(void);
extern void rtc_date_time(void);
extern void rtc_read_time(void);
#endif


