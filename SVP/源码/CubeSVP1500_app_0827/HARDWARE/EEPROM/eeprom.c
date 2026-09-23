#include "eeprom.h"

#include "usart.h"
#include "delay.h"
#include "string.h"




HAL_StatusTypeDef AT24C02_Write(uint16_t memAddress,uint8_t *pData,uint16_t size){
    if(memAddress + size > 256){
        return HAL_ERROR;
    }
    HAL_StatusTypeDef status;
    uint16_t byteWritten = 0;
    uint16_t currentAddress = memAddress;  
	while(byteWritten < size){
        uint16_t pageOffset = currentAddress % EEPROM_PAGE_BYTE_SIZE;
        uint16_t bytesRemainingInPage = EEPROM_PAGE_BYTE_SIZE - pageOffset;
        uint16_t bytesToWrite = (size - byteWritten) < bytesRemainingInPage ? (size-byteWritten) : bytesRemainingInPage;
        
        
        status = HAL_I2C_Mem_Write(&IIC_Config,EEPROM_WRITE_ADDR,currentAddress,I2C_MEMADD_SIZE_8BIT,&pData[byteWritten],bytesToWrite,HAL_MAX_DELAY);
        if(status != HAL_OK )
            return status;
        delay_ms(5);
        byteWritten += bytesToWrite;
        currentAddress += bytesToWrite;
    }
    
    return status;
}


HAL_StatusTypeDef AT24C02_Read(uint16_t memAddress,uint8_t *pData,uint16_t size){
    if(memAddress + size > 256){
        return HAL_ERROR;
    }
    return HAL_I2C_Mem_Read(&IIC_Config,EEPROM_WRITE_ADDR|0x01,memAddress,I2C_MEMADD_SIZE_8BIT,pData,size,HAL_MAX_DELAY);
}

void AT24C02_EraseALL(void){
    uint8_t blank[256] = {0};
    AT24C02_Write(0,blank,256);
    delay_ms(5);
}

void at24c02_reset(void){
    AT24C02_EraseALL(); 
    svp_cmd_t reset_set;
    reset_set.BOUND = 115200;
    reset_set.TYPE = 0;
    reset_set.DATE.year = 2025;
    reset_set.DATE.mon = 4;
    reset_set.DATE.date = 24;
    reset_set.PROBE_DISTANCE = 5.0f;
    reset_set.OUTLIERS_THRESHOLD = 50;
    reset_set.KLAMAN_OBSERVATIONS = 0;
    reset_set.FREQ_BOUND = 0;
    //声速默认系数
    reset_set.SOUND_VELOCITY_COE_A1 = 1.0f;
    reset_set.SOUND_VELOCITY_COE_B1 = 1.0f;
    reset_set.SOUND_VELOCITY_COE_A2 = 1.0f;
    reset_set.SOUND_VELOCITY_COE_B2 = 0.0f;
    reset_set.SOUND_VELOCITY_COE_A3 = 0.0f;
    reset_set.SOUND_VELOCITY_COE_B3 = 0.0f;
    reset_set.COE_2_SPOCE = 1300.00f;
    reset_set.COE_3_SPOCE = 1400.00f;
    reset_set.FRIST_WAVE_V = 15;
    reset_set.RTC_DATE_TIME.year = 25;
    reset_set.RTC_DATE_TIME.mon = 4;
    reset_set.RTC_DATE_TIME.date = 24;
    reset_set.RTC_DATE_TIME.day = 4;
    reset_set.RTC_DATE_TIME.hour = 13;
    reset_set.RTC_DATE_TIME.min = 30;
    reset_set.RTC_DATE_TIME.sec = 00;
    reset_set.RTC_DATE_TIME.ampm = 1;
    reset_set.RTC_FLAG = 0; //RTC开始初始化
    reset_set.FILE_STATUS = 0;
    reset_set.WORK_MODE_FLAG = 0;
    reset_set.WORK_SET_MODE_FLAG = 0;
    reset_set.WORK_VALUE = 100;
    //压力默认系数
    reset_set.PA_COE_A = 0.0;
    reset_set.PA_COE_E = 1.0;
    reset_set.PA_COE_I = 1.0;
    reset_set.PA_COE_M = 1.0;
    //温度默认系数
    reset_set.TEMP_COE_A = 1.0;
    reset_set.TEMP_COE_B = 0.0;
    reset_set.TEMP_COE_C = 0.0;
    reset_set.TEMP_COE_D = 0.0;
		//压力补偿系数
		reset_set.DEPTH_EC_X = 1.0;
		reset_set.DEPTH_EC_Y = 0.0;
    AT24C02_Write(0,(uint8_t*)&reset_set,sizeof(reset_set));
}

void SVP_CMD_INIT(svp_cmd_t *svp_cmd){
    AT24C02_Read(0,(uint8_t*)svp_cmd,sizeof(svp_cmd_t));
    /*
	printf("    SV:\r\n");
	printf("        svp_cmd->PROBE_DISTANCE %f\r\n",svp_cmd->PROBE_DISTANCE);
	printf("        svp_cmd->OUTLIERS_THRESHOLD %d\r\n",svp_cmd->OUTLIERS_THRESHOLD);
	printf("        %f~%f\r\n",svp_cmd->COE_2_SPOCE,svp_cmd->COE_3_SPOCE);
	printf("            svp_cmd->SOUND_VELOCITY_COE_A1 %f\r\n",svp_cmd->SOUND_VELOCITY_COE_A1);
	printf("            svp_cmd->SOUND_VELOCITY_COE_B1 %f\r\n",svp_cmd->SOUND_VELOCITY_COE_B1);
	printf("            svp_cmd->SOUND_VELOCITY_COE_A2 %f\r\n",svp_cmd->SOUND_VELOCITY_COE_A2);
	printf("            svp_cmd->SOUND_VELOCITY_COE_B2 %f\r\n",svp_cmd->SOUND_VELOCITY_COE_B2);
	printf("            svp_cmd->SOUND_VELOCITY_COE_A3 %f\r\n",svp_cmd->SOUND_VELOCITY_COE_A3);
	printf("            svp_cmd->SOUND_VELOCITY_COE_B3 %f\r\n",svp_cmd->SOUND_VELOCITY_COE_B3);
	printf("        SN:%s\r\n",svp_cmd->SV_SN_BUF);
	printf("    TEMP:\r\n");
	printf("        svp_cmd->TEMP_COE_A %f\r\n",svp_cmd->TEMP_COE_A);
	printf("        svp_cmd->TEMP_COE_B %f\r\n",svp_cmd->TEMP_COE_B);
	printf("        svp_cmd->TEMP_COE_A %f\r\n",svp_cmd->TEMP_COE_C);
	printf("        svp_cmd->TEMP_COE_B %f\r\n",svp_cmd->TEMP_COE_D);
	printf("        SN:%s\r\n",svp_cmd->TEMP_SN_BUF);
	printf("    PA :\r\n");
	printf("        SN:%s\r\n",svp_cmd->PRESSURE_SN_BUF);
    */
    uint8_t buf = AT24C02_Read(256,&buf,1);
    if(buf == 0xFF){
        at24c02_reset();
    }
    buf = 0x01;
    AT24C02_Write(256,&buf,1);
}
