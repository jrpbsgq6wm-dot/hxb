#include "rtc.h"


volatile uint8_t DS3231_buffer[19];

DS3231_RegisterType DS3231_Register;
DS3231_TimeType DS3231_Time;

//BCD转十进制
uint8_t bcd2dec(uint8_t bcd){
    return ( (bcd) >> 4 ) * 10 + ( (bcd) & 0x0f );
}
//十进制转BCD
uint8_t dec2bcd(uint8_t dec){
    return (( (dec)/10 ) << 4 ) + ( (dec) % 10 );
}
void DS3231_Init(uint8_t year,uint8_t mon,uint8_t date,uint8_t day,uint8_t hour,uint8_t min,uint8_t sec,uint8_t AM_PM){
    //
    DS3231_TimeType temp_time;
    temp_time.year = year;
    temp_time.mon = mon;
    temp_time.date = date;
    temp_time.day = day;
    temp_time.hour = hour;
    temp_time.min = min;
    temp_time.sec = sec;
    temp_time.hour_form = HOUR_FORM_24;
    temp_time.AM_PM = AM_PM;
    
    DS3231_Set_Time(&temp_time);
    HAL_Delay(50);
    DS3231_Update();
    HAL_Delay(50);
    DS3231_Read_All();
    HAL_Delay(50);
    DS3231_Read_Time();
    HAL_Delay(50);
}
//一次性读取所有的寄存器
void DS3231_Read_All(void){
    uint8_t temp[1] = {0};
    /*设置寄存器指针位置*/
    HAL_I2C_Master_Transmit(&IIC_Config,0XD0,temp,1,0xffff);
    HAL_I2C_Master_Receive(&IIC_Config,0xD1,(uint8_t*)&DS3231_Register,sizeof(DS3231_Register),0xffff);
}

float DS3231_Read_Temp(void){
    uint8_t sign = (DS3231_Register.Temp_MSB >> 7);
    float ret;
    ret = (float)DS3231_Register.Temp_MSB + (float)(DS3231_Register.Temp_LSB >> 6)*0.25f;
    if(sign)
        ret = 0 - ret;
    else
        ret = 0 + ret;
    return ret;
}

void DS3231_Read_Time(){
    DS3231_Time.sec = bcd2dec(DS3231_Register.Seconds);
    DS3231_Time.min = bcd2dec(DS3231_Register.Minutes);
    if((DS3231_Register.Hours & 0X40) == 0X40){
        DS3231_Time.hour_form = HOUR_FORM_12;
        DS3231_Time.AM_PM = (DS3231_Register.Hours) & 0x20;
        DS3231_Time.hour = bcd2dec(DS3231_Register.Hours & 0x1F);
    }else{
        DS3231_Time.hour_form = HOUR_FORM_24;
        DS3231_Time.hour = bcd2dec(DS3231_Register.Hours);
    }
    DS3231_Time.year = bcd2dec(DS3231_Register.Year);
    DS3231_Time.mon = bcd2dec(DS3231_Register.Month_Century);
    DS3231_Time.date = bcd2dec(DS3231_Register.Date);
    DS3231_Time.day = DS3231_Register.Day;
}

void DS3231_Set_Time(DS3231_TimeType* time){
    DS3231_Register.Seconds = dec2bcd(time->sec);
    DS3231_Register.Minutes = dec2bcd(time->min);
    if(time->hour_form == HOUR_FORM_12)
        DS3231_Register.Hours = (0X40 | (time->AM_PM << 5)) | (dec2bcd(time->hour));
    else
        DS3231_Register.Hours = dec2bcd(time->hour);
    DS3231_Register.Year = dec2bcd(time->year);
    DS3231_Register.Month_Century = dec2bcd(time->mon);
    DS3231_Register.Date = dec2bcd(time->date);
    DS3231_Register.Day = time->day;
}

void DS3231_Update(){
    HAL_I2C_Mem_Write(&IIC_Config,0XD0,0X00,1,(uint8_t*)&DS3231_Register,sizeof(DS3231_Register),0XFFFF);
}

//串口指令设置RTC时间
void rtc_date_time(void){
    DS3231_Init(
                    svp_cmd.RTC_DATE_TIME.year,
                    svp_cmd.RTC_DATE_TIME.mon,
                    svp_cmd.RTC_DATE_TIME.date,
                    svp_cmd.RTC_DATE_TIME.day,   
                    svp_cmd.RTC_DATE_TIME.hour,
                    svp_cmd.RTC_DATE_TIME.min,    
                    svp_cmd.RTC_DATE_TIME.sec,
                    svp_cmd.RTC_DATE_TIME.ampm
            );
    uint8_t rtcflag = 0x01;
    AT24C02_Write(RTC_FLAG_ADDR,&rtcflag,1);
}

