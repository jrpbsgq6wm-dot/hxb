#include "beam.h"

/********************************************************************************
 * 模块：上位机参数解析与FPGA配置转换
 * 说明：由原 beam.c 按功能拆分，函数体保持原有逻辑。
 ********************************************************************************/

/*****************************************************************
 * 名称：                    crc16
 * 功能：                    计算两个字节的CRC
 * 入口参数：            	 *data：数组的起始地址  len：数组的长度  crc：crc的初值
 * 出口参数：            	 两字节的crc
 *****************************************************************/
unsigned short crc16(void* data, unsigned int len, unsigned short crc)
{
    unsigned char* pBuff = (unsigned char*)data;
    unsigned int i;
    for(i = 0; i < len; i++)
    {
        crc = crc^(unsigned short)pBuff[i];
        crc = (crc>>8)^crc16_tab[crc & 0x0FF];
    }
    return crc;
}
/********************************************************************************
 * 名称：         parsing_instructions_200k
 * 功能：         接收到的上位机数据转化为FPGA寄存器值
 * 入口参数：      *ptr_recv_upper_package：接收到的上位机的结构体值 *ptr_fpga_config_para：转化为FPGA的值
 * 出口参数：      正确为0，失败是-1
 *********************************************************************************/
int parsing_instructions_200k(const RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package, FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para)
{
	//工作模式寄存器   Bit0，0：CW，1：LFM 
    //                Bit1，0：升频，1：降频 
    //                Bit2，0：原始数据，1：处理数据
    ptr_fpga_config_para->wsm_registers_value.wsm_mod = (ptr_recv_upper_package->WorkMode) | (ptr_recv_upper_package->LFMMode << 1) | (ptr_recv_upper_package->DataType << 2); 
    if (ptr_recv_upper_package->PingRate!=0)
    {
        ptr_fpga_config_para->wsm_registers_value.wsm_ct = (unsigned int)(FPGA_CLK_FREQUENCY / ptr_recv_upper_package->PingRate);
    }
    else
    {               //升频0101 //降频0111
        printf("revc upper PingRate is 0!\n");
        return -1;
    }
    ptr_fpga_config_para->adc_registers_value.adc_sct = (unsigned int)((FPGA_CLK_FREQUENCY / (ptr_recv_upper_package->SamplingRate*1000)) - 1);  
    ptr_fpga_config_para->adc_registers_value.adc_sn = (unsigned int)((unsigned int)(ptr_recv_upper_package->Range * 2 * ptr_recv_upper_package->SamplingRate*1000/ V_SOUND)/ SAMPLE_FACTOR_8 * SAMPLE_FACTOR_8-1);  
    //ptr_fpga_config_para->dac_registers_value.dac_sct = (unsigned int)(ptr_recv_upper_package->UpperConfigParameter.Range * 2 * FPGA_CLK_FREQUENCY / 1500 / ptr_recv_upper_package->TvgDataSize); 
    ptr_fpga_config_para->dac_registers_value.dac_sct = 99999; //1ms
    ptr_fpga_config_para->dac_registers_value.dac_sn  = (unsigned int)(ptr_recv_upper_package->Range * 2 * 1000 / V_SOUND);
    ptr_fpga_config_para->pwm_registers_value.pwm_pulse = (unsigned int)(ptr_recv_upper_package->PulseWidth * (FPGA_CLK_FREQUENCY / 1000000.0f));
    ptr_fpga_config_para->pwm_registers_value.TransGear = ptr_recv_upper_package->TransGear;	
    
	ptr_fpga_config_para->pwm_registers_value.svp_select = ptr_recv_upper_package->Svp_Select;
    //初始相位赋值 
    Debug("U_pwm_ip1[1]: %f\n", ptr_recv_upper_package->pwm_ip[1]);
    int i=0;float tmp[15];
    for (i=0;i<15;i++)
    {  
        tmp[i] = (float)FPGA_CLK_FREQUENCY / (float)360 * ptr_recv_upper_package->pwm_ip[i]/ (float)ptr_recv_upper_package->PWMFreq /1000.00f;
        ptr_fpga_config_para->pwm_registers_value.pwm_ip[i] = ((unsigned int)((int)(tmp[i]+0.5)>(int)tmp[i]?(int)tmp[i]+1:(int)tmp[i]))<<16;//|ptr_recv_upper_package->pwm_ip[i];
    }
    Debug("temp[1]: %f\n", tmp[1]);
    //功率系数赋值或者经过计算后赋值，公式待定
    ptr_fpga_config_para->pwm_registers_value.power_factor = ((unsigned int)(ptr_recv_upper_package->power_factor*10))<<16|(unsigned int)(ptr_recv_upper_package->power_factor *1.23/48/3.3*4096);

    //pwm 频率和采样率和量程，赋值，fpga回传使用
    ptr_fpga_config_para->pwm_registers_value.pwm_frequency = ptr_recv_upper_package->PWMFreq;
    ptr_fpga_config_para->pwm_registers_value.sample_rate = ptr_recv_upper_package->SamplingRate;
    ptr_fpga_config_para->pwm_registers_value.range = ptr_recv_upper_package->Range; 
    ptr_fpga_config_para->pwm_registers_value.bandwidth = ptr_recv_upper_package->PWMBandWidth; 
    //pwm开关
    ptr_fpga_config_para->pwm_registers_value.pwm_start = ptr_recv_upper_package->pwm_start;
    //抽样后的ad个数，留着fpga使用
    ptr_fpga_config_para->pwm_registers_value.AD_sn_after = (ptr_fpga_config_para->adc_registers_value.adc_sn + 1) / SAMPLE_FACTOR;
 //   ptr_fpga_config_para->pwm_registers_value.AD_sn_after = ((ptr_fpga_config_para->adc_registers_value.adc_sn + 1) / SAMPLE_FACTOR)/2;
    //    if ((ptr_recv_upper_package->PWMFreq == 200)||(ptr_recv_upper_package->PWMFreq == 400))
    //    {
    //        ptr_fpga_config_para->pwm_registers_value.AD_sn_after = (ptr_fpga_config_para->adc_registers_value.adc_sn + 1) / SAMPLE_FACTOR_8;
    //    }
    //    if ((ptr_recv_upper_package->PWMFreq > 200)&&(ptr_recv_upper_package->PWMFreq < 400)) 
    //    {
    //        ptr_fpga_config_para->pwm_registers_value.AD_sn_after = (ptr_fpga_config_para->adc_registers_value.adc_sn + 1) / SAMPLE_FACTOR;
    //    }
    //ad通道组数选择
    ptr_fpga_config_para->pwm_registers_value.adc_num_fpga = ptr_recv_upper_package->AD_NUM;
    //计算pitch使用的系数
    ptr_fpga_config_para->pwm_registers_value.phcoe =(int)((307200)*(ptr_recv_upper_package->PWMFreq));//-*256/24*2^8*100=-273066
    ptr_fpga_config_para->pwm_registers_value.final_coe = (int)(100000/ptr_recv_upper_package->PWMFreq);//3276800000
    //初始相位开关
    ptr_fpga_config_para->pwm_registers_value.pitch_oe = ptr_recv_upper_package->pitch_oe;

    if (ptr_recv_upper_package->WorkMode == 0) 
    {
        ptr_fpga_config_para->pwm_registers_value.pwm_bf = (unsigned int)((double)(ptr_recv_upper_package->PWMFreq*1000*N2_32) / FPGA_CLK_FREQUENCY); 
        ptr_fpga_config_para->pwm_registers_value.pwm_lfm=0;
    }
    if (ptr_recv_upper_package->WorkMode == 1) 
    {
        ptr_fpga_config_para->pwm_registers_value.pwm_lfm = (unsigned int)((PWM_LFM_FACTOR / ptr_recv_upper_package->PulseWidth)* ptr_recv_upper_package->PWMBandWidth);
        if (ptr_recv_upper_package->LFMMode == 0) 
        {
            ptr_fpga_config_para->pwm_registers_value.pwm_bf = (unsigned int)(N2_32 / (FPGA_CLK_FREQUENCY/1000) * (ptr_recv_upper_package->PWMFreq - ptr_recv_upper_package->PWMBandWidth / 2) - ptr_fpga_config_para->pwm_registers_value.pwm_lfm / 2/N2_23); 
        }
        else
        {
            ptr_fpga_config_para->pwm_registers_value.pwm_bf = (unsigned int)(N2_32 / (FPGA_CLK_FREQUENCY/1000) * (ptr_recv_upper_package->PWMFreq + ptr_recv_upper_package->PWMBandWidth /2)- ptr_fpga_config_para->pwm_registers_value.pwm_lfm / 2/ N2_23);
        }
    } 
	if(ptr_recv_upper_package->sensor_fh_AT == 'q' || ptr_recv_upper_package->sensor_fh_HEADING == 'q')
	{
		DBG("111111111111111111111111111111111\n");
		ptr_fpga_config_para->pwm_registers_value.sync_state = (16 | ptr_fpga_config_para->pwm_registers_value.sync_state);
	}
	else
	{
		DBG("2222222222222222222222222222222222\n");
		ptr_fpga_config_para->pwm_registers_value.sync_state = (~16 & ptr_fpga_config_para->pwm_registers_value.sync_state);
	}
    return 0;
}

/********************************************************************************
 * 名称：                    Debug_pritf_convert_fpga_parameter
 * 功能：                    打印FPGA的寄存器的内容
 * 入口参数：            	 *ptr_fpga_config_para ARM下发给FPGA的值
 * 出口参数：            	 无
 *********************************************************************************/
void Debug_pritf_convert_fpga_parameter(FPGA_CONFIG_PARAMETERS *ptr_fpga_config_para )
{
	printf("***************************************************\n");
    Debug("wsm_mod: %d\n", ptr_fpga_config_para->wsm_registers_value.wsm_mod); 
    Debug("wsm_ct: %d\n", ptr_fpga_config_para->wsm_registers_value.wsm_ct); 
    Debug("adc_sct: %d\n", ptr_fpga_config_para->adc_registers_value.adc_sct); 
    Debug("adc_sn: %d\n", ptr_fpga_config_para->adc_registers_value.adc_sn);
    Debug("dac_sct: %d\n", ptr_fpga_config_para->dac_registers_value.dac_sct); 
    Debug("dac_sn: %d\n", ptr_fpga_config_para->dac_registers_value.dac_sn);  
    Debug("pwm_bf: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_bf); 
    Debug("pwm_lfm: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_lfm); 
    //Debug("pwm_ip[1]: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_ip[1]); 
    Debug("pwm_pulse: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_pulse);
    Debug("pwm_start %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_start); 
    Debug("sync_state %d\n", ptr_fpga_config_para->pwm_registers_value.sync_state); 
    Debug("sync_plustype %d\n", ptr_fpga_config_para->pwm_registers_value.sync_selec); 
    Debug("sync_delaytime %d\n", ptr_fpga_config_para->pwm_registers_value.sync_delaytime); 

    //Debug("power_factor %d\n", ptr_fpga_config_para->pwm_registers_value.power_factor);
    Debug("pwm_frequency %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_frequency);
    //Debug("sample_rate %d\n", ptr_fpga_config_para->pwm_registers_value.sample_rate);
    //Debug("range %d\n", ptr_fpga_config_para->pwm_registers_value.range);
    Debug("bandwidth %d\n", ptr_fpga_config_para->pwm_registers_value.bandwidth);
    //Debug("AD_sn_after %d\n", ptr_fpga_config_para->pwm_registers_value.AD_sn_after);
    Debug("pitch_oe %d\n", ptr_fpga_config_para->pwm_registers_value.pitch_oe);
    Debug("phcoe %d\n", ptr_fpga_config_para->pwm_registers_value.phcoe);
    Debug("final_coe %d\n", ptr_fpga_config_para->pwm_registers_value.final_coe);
}

/********************************************************************************
 * 名称：                    Debug_pritf_receive_para
 * 功能：                    打印上位机下发给ARM的内容
 * 入口参数：            	 *receive_upper_package ARM接收上位机的值
 * 出口参数：            	 无
 *********************************************************************************/
void Debug_pritf_receive_para(RECV_UPPER_CONFIG_PARAMETERS *receive_upper_package)
{
    printf("upper print\n");
    Debug("DataType: %d\n", receive_upper_package->DataType); 
    Debug("WorkMode: %d\n", receive_upper_package->WorkMode); 
    Debug("LFMMode: %d\n", receive_upper_package->LFMMode); 
    //printf("upper print\n");
    Debug("PWMFreq: %d\n", receive_upper_package->PWMFreq); 
    Debug("PWMBandWidth: %d\n", receive_upper_package->PWMBandWidth); 
    Debug("SamplingRate: %d\n", receive_upper_package->SamplingRate); 
    Debug("Range: %d\n", receive_upper_package->Range); 
    Debug("ManualGain: %d\n", receive_upper_package->ManualGain); 
    Debug("AbsorbGainCoef: %d\n", receive_upper_package->AbsorbGainCoef); 
    Debug("SpreadGainCoef: %d\n", receive_upper_package->SpreadGainCoef); 
    Debug("TransGear: %d\n", receive_upper_package->TransGear); 
    Debug("PulseWidth: %f\n", receive_upper_package->PulseWidth); 
    Debug("PingRate: %f\n", receive_upper_package->PingRate); 
    Debug("U_power_factor: %f\n", receive_upper_package->power_factor); 
    Debug("U_pwm_start: %d\n", receive_upper_package->pwm_start);
    Debug("U_pwm_ip[1]: %f\n", receive_upper_package->pwm_ip[1]); 
    // Debug("fsv: %d\n", receive_upper_package->fsv);
    Debug("sensor_fh_GGA_ZDA: %c\n", receive_upper_package->sensor_fh_GGA_ZDA); 
    Debug("sensor_fh_HEADING: %c\n", receive_upper_package->sensor_fh_HEADING); 
    Debug("sensor_fh_AT: %c\n", receive_upper_package->sensor_fh_AT);
    Debug("sensor_fh_SVT: %c\n", receive_upper_package->sensor_fh_SVT);
    Debug("sensor_ft_GGA_ZDA[0]: %d\n", receive_upper_package->sensor_ft_GGA_ZDA[0]);
    Debug("sensor_ft_GGA_ZDA[1]: %d\n", receive_upper_package->sensor_ft_GGA_ZDA[1]);
    Debug("sensor_ft_HEADING[0]: %d\n", receive_upper_package->sensor_ft_HEADING[0]); 
    Debug("sensor_ft_HEADING[1]: %d\n", receive_upper_package->sensor_ft_HEADING[1]); 
    Debug("sensor_ft_AT[0]: %x\n", receive_upper_package->sensor_ft_AT[0]);
    Debug("sensor_ft_AT[1]: %x\n", receive_upper_package->sensor_ft_AT[1]);
    Debug("sensor_ft_SVT[0]: %x\n", receive_upper_package->sensor_ft_SVT[0]);
    Debug("sensor_ft_SVT[1]: %x\n", receive_upper_package->sensor_ft_SVT[1]);
    Debug("sensor_boud_GGA_ZDA: %d\n", receive_upper_package->sensor_boud_GGA_ZDA);
    Debug("sensor_boud_HEADING: %d\n", receive_upper_package->sensor_boud_HEADING); 
    Debug("sensor_boud_AT: %d\n", receive_upper_package->sensor_boud_AT);
    Debug("sensor_boud_SVT: %d\n", receive_upper_package->sensor_boud_SVT);

}
/********************************************************************************
 * 名称：                    copy_recv_cmd
 * 功能：                    将接收的上位机下发的数据保存本地
 * 入口参数：            	 *ptr_cmd_des ---保存到ARM本地的参数 *ptr_cmd_source----上位机下发的参数
 * 出口参数：            	 无
 *********************************************************************************/
void copy_recv_cmd(RECV_UPPER_CONFIG_PARAMETERS *ptr_cmd_des, const RECV_UPPER_CONFIG_PARAMETERS  *ptr_cmd_source)
{
    memcpy((char *)(&ptr_cmd_des->DataType), (char *)(&(ptr_cmd_source->DataType)), SIZE_OF_CONFIG_PARA); 
}

/********************************************************************************
* 名称：                    config_sensor_baud_and_framehead
* 功能：                    ARM向FPGA的传感器相关寄存器写值
* 入口参数：            	 ptr_recv_upper_package 接收到上位机的参数配置值
* 出口参数：            	 无
*********************************************************************************/
void config_sensor_baud_and_framehead(const RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package)
{
	fpga_register_data.fpga_sensor.GGA_ZDA_BAUD = FPGA_CLK_FREQUENCY/ptr_recv_upper_package->sensor_boud_GGA_ZDA; //波特率38400
	fpga_register_data.fpga_sensor.HEADING_BAUD =  FPGA_CLK_FREQUENCY/ptr_recv_upper_package->sensor_boud_HEADING; 
	fpga_register_data.fpga_sensor.AT_BAUD= FPGA_CLK_FREQUENCY/ptr_recv_upper_package->sensor_boud_AT;
	fpga_register_data.fpga_sensor.SVT_BAUD= FPGA_CLK_FREQUENCY/ptr_recv_upper_package->sensor_boud_SVT;
	fpga_register_data.fpga_sensor.GGA_ZDA_FH =(unsigned int)ptr_recv_upper_package->sensor_fh_GGA_ZDA;//4724;//$G改成$ 
	if(ptr_recv_upper_package->sensor_fh_AT == 'q'| ptr_recv_upper_package->sensor_fh_HEADING == 'q')
	{
		ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state = (16 | ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state);
		fpga_register_data.fpga_sensor.HEADING_FH =(unsigned int) ptr_recv_upper_package->sensor_fh_HEADING; //0x4824;//$H改成$ 
		fpga_register_data.fpga_sensor.AT_FH=(unsigned int)ptr_recv_upper_package->sensor_fh_AT;//0x303A;//:0改成:
	}
	else
	{
		ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state = (~16 & ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state);
		fpga_register_data.fpga_sensor.HEADING_FH =(unsigned int) ptr_recv_upper_package->sensor_fh_HEADING; //0x4824;//$H改成$ 
		fpga_register_data.fpga_sensor.AT_FH=(unsigned int)ptr_recv_upper_package->sensor_fh_AT;//0x303A;//:0改成:
	}
	fpga_register_data.fpga_sensor.SVT_FH=(unsigned int)ptr_recv_upper_package->sensor_fh_SVT;//0x3120; //空格1 改成空格
	fpga_register_data.fpga_sensor.GGA_ZDA_FT =(unsigned int)(ptr_recv_upper_package->sensor_ft_GGA_ZDA[1]*0x100+ptr_recv_upper_package->sensor_ft_GGA_ZDA[0]);//4724;//$G改成$ 
	fpga_register_data.fpga_sensor.HEADING_FT = (unsigned int)(ptr_recv_upper_package->sensor_ft_HEADING[1]*0x100+ptr_recv_upper_package->sensor_ft_HEADING[0]); //0x4824;//$H改成$ 
	fpga_register_data.fpga_sensor.AT_FT=(unsigned int)(ptr_recv_upper_package->sensor_ft_AT[1]*0x100+ptr_recv_upper_package->sensor_ft_AT[0]);//0x303A;//:0改成:
	fpga_register_data.fpga_sensor.SVT_FT=(unsigned int)(ptr_recv_upper_package->sensor_ft_SVT[1]*0x100+ptr_recv_upper_package->sensor_ft_SVT[0]);//0x3120; //空格1 改成空格
}

void Debug_pritf_sensor_baud_and_framehead(FPGA_CONFIG_SENSER_PARAMETERS *fpga_sensor_printf)
{
	Debug("GGA_ZDA_BAUD: %d\n", fpga_sensor_printf->GGA_ZDA_BAUD); 
	Debug("HEADING_BAUD: %d\n", fpga_sensor_printf->HEADING_BAUD); 
	Debug("GGA_ZDA_BAUD: %d\n", fpga_sensor_printf->AT_BAUD); 
	Debug("GGA_ZDA_BAUD: %d\n", fpga_sensor_printf->SVT_BAUD); 

	Debug("GGA_ZDA_FH: %c\n", fpga_sensor_printf->GGA_ZDA_FH); 
	Debug("HEADING_FH: %c\n", fpga_sensor_printf->HEADING_FH); 
	Debug("AT_FH     : %c\n", fpga_sensor_printf->AT_FH);
	Debug("SVT_FH    : %c\n", fpga_sensor_printf->SVT_FH);

	Debug("GGA_ZDA_FT: %x\n", fpga_sensor_printf->GGA_ZDA_FT); 
	Debug("HEADING_FT: %x\n", fpga_sensor_printf->HEADING_FT); 
	Debug("AT_FT     : %x\n", fpga_sensor_printf->AT_FT);
	Debug("SVT_FT    : %x\n", fpga_sensor_printf->SVT_FT);
}

