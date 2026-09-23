#include "power_app.h"


const float resistor = 0.01f;    //参考电阻 0.01Ω
uint8_t ack = 0;
static uint8_t mAh_or_Coulombs = 0;
static uint8_t celcius_or_kelvin = 0;
static uint16_t prescalar_mode = LTC2943_PRESCALAR_M_1024;
static uint16_t prescalar_value = 1024;
static uint16_t alcc_mode = LTC2943_ALRET_MODE;
float charge = 0,current = 0,voltage = 0,temperature = 0,SOC = 0.0f;;



void ltc2943_init(void){
    uint8_t LTC2943_mode,ack=0;
       
    
    LTC2943_mode = LTC2943_AUTOMATIC_MODE | prescalar_mode | alcc_mode; //暂存控制寄存器
    ack |= LTC2943_Write(LTC2943_I2C_ADDRESS,LTC2943_CONTROL_REG,LTC2943_mode); //1111 0100 
	
}

//满电电池校准
void LTC2943_writeFullChargeI2C(uint16_t fullacr){
    uint8_t data[2] = {(fullacr>>8&0xff),fullacr&0xff};
    uint8_t ctrl_reg;
    //读取当前控制寄存器值
    HAL_I2C_Mem_Read(&IIC_Config,LTC2943_I2C_ADDRESS,0x01,I2C_MEMADD_SIZE_8BIT,&ctrl_reg,1,100);
    //关闭模拟部分
    ctrl_reg |= 0x01;
    HAL_I2C_Mem_Write(&IIC_Config,LTC2943_I2C_ADDRESS,0x01,I2C_MEMADD_SIZE_8BIT,&ctrl_reg,1,100);
    //写入累计电荷寄存器
    HAL_I2C_Mem_Write(&IIC_Config,LTC2943_I2C_ADDRESS,0x02,I2C_MEMADD_SIZE_8BIT,data,2,100);
    //重新开启模拟部分
    ctrl_reg &= ~0x01;
    HAL_I2C_Mem_Write(&IIC_Config,LTC2943_I2C_ADDRESS,0x01,I2C_MEMADD_SIZE_8BIT,&ctrl_reg,1,100);
}

//读取电池状态 ... ...
HAL_StatusTypeDef read_power_data(void){
    if(menu_1_automatic_mode(mAh_or_Coulombs,celcius_or_kelvin,prescalar_mode,prescalar_value,alcc_mode) == HAL_OK)
        return HAL_OK;
    return HAL_ERROR_LTC2943;
}

/** 
 * 分段函数计算电池电量百分比
 * @param voltage当前电压
 * @return 电量百分比
 */
static float battery_percentage_segmented(float voltage) {
    // 锂电池放电曲线分段
    // 高压平台区: 8.4V-7.8V (100%-80%)
    // 线性放电区: 7.8V-7.0V (80%-20%)
    // 低压平台区: 7.0V-6.4V (20%-0%)
    
    if (voltage >= 8.4f) return 100.0;
    if (voltage <= 6.4f) return 0.0;
    
    float percentage;
    
    if (voltage >= 7.8f) {
        // 电量缓慢下降
        percentage = 80.0f + (voltage - 7.8f) / (8.4f - 7.8f) * 20.0f;
    } 
    else if (voltage >= 7.0f) {
        // 线性放电
        percentage = 20.0f + (voltage - 7.0f) / (7.8f - 7.0f) * 60.0f;
    } 
    else {
        // 低压平台区
        percentage = (voltage - 6.4f) / (7.0f - 6.4f) * 20.0f;
    }
    
    /*
     * 低于 6.6 V 时禁止继续记录并关闭当前文件；恢复到 6.7 V 以上才重新允许
     * TDC 的稳定入水逻辑发起下一轮记录，0.1 V 回差避免临界电压反复开关文件。
     */
	if(voltage < 6.6f){
        pri_set_record_enable(0);
    }else if(voltage >= 6.7f){
        pri_set_record_enable(1);
	}
	
    // 0-100
    if (percentage > 100.0f) percentage = 100.0f;
    if (percentage < 0.0f) percentage = 0.0f;
    
    return (float)(percentage);
}

uint8_t menu_1_automatic_mode(uint8_t mAh_or_Coulombs,uint8_t celcius_or_kelvin,uint16_t prescalar_mode,
                uint16_t prescalar_value,uint16_t alcc_mode)
{
    uint8_t ack=0;
    uint8_t status_code = 0;
    //uint16_t fullacr = 63764;
    uint16_t charge_code = 0,current_code = 0,voltage_code = 0,temperature_code = 0;
    uint16_t charge_l2b = 0,current_l2b = 0,voltage_l2b = 0,temperature_l2b = 0;
    
    ack |= LTC2943_Read_16_Bits(LTC2943_I2C_ADDRESS|I2C_READ_BIT,LTC2943_ACCUM_CHARGE_MSB_REG,&charge_code);
    ack |= LTC2943_Read_16_Bits(LTC2943_I2C_ADDRESS|I2C_READ_BIT,LTC2943_VOLTAGE_MSB_REG,&voltage_code);
    ack |= LTC2943_Read_16_Bits(LTC2943_I2C_ADDRESS|I2C_READ_BIT,LTC2943_CURRENT_MSB_REG,&current_code);
    ack |= LTC2943_Read(LTC2943_I2C_ADDRESS|I2C_READ_BIT,LTC2943_STATUS_REG,&status_code);
    ack |= LTC2943_Read_16_Bits(LTC2943_I2C_ADDRESS|I2C_READ_BIT,LTC2943_TEMPERATURE_MSB_REG,&temperature_code);
    
    charge_l2b = ((charge_code<< 8) | (charge_code>>8));
    current_l2b = ((current_code<< 8) | (current_code>>8));
    voltage_l2b = ((voltage_code<< 8) | (voltage_code>>8));
    temperature_l2b = ((temperature_code<< 8) | (temperature_code>>8));
    
    charge = LTC2943_code_to_mAh(charge_l2b,resistor,prescalar_value);
    current = LTC2943_code_to_current(current_l2b,resistor);
    voltage = LTC2943_code_to_voltage(voltage_l2b);
    temperature = LTC2943_code_to_celcius_temperature(temperature_l2b);
	
	SOC = battery_percentage_segmented(voltage);
	
	
	
#if 0
     //判断当前电流和电压是否完成了充电，如果完成了则满电赋值
    if(current < 0.1f && voltage > 8.8f){ //每3s写入一次 - 充满
        cdflag++;
        if(cdflag == 6 && (status_full_battry ==0)){
            LTC2943_writeFullChargeI2C(fullacr);
            cdflag = 0;
            status_full_battry = 1;
        }
    }else if(current < 0.0f && voltage < 6.0f ){
        lowflag++;
        //电池电量低，文件标志置位 - 让没有保存的文件保存
        if(lowflag == 6 && status_low_battry == 0){ // 电量低
            pri_file.start_flag = 0;
            lowflag = 0;
			status_low_battry = 1;
        }
    }else{      /* 正在充电或者未充电 */
        lowflag = 0;
        cdflag = 0;
    }
    //确保还没充满显示100%的情况
	//当前mah大于1600 并且电压大于3V
    if(charge>=1600 && current>3.0f){
        SOC = 99.9f;
    }else{
        //计算当前电量
        SOC = (charge-(350)) /(((Full_Battry_V)*0.425)-(350))* 100;
    }
    //限制电量
    if(SOC >= 100)
        SOC = 100.0f;
    else if(SOC <= 0)
        SOC = 0.0f;
#endif
	
    return ack;
}







