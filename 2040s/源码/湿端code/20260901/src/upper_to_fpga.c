#include "beam.h"
#include "network.h"
#include "upper_to_fpga.h"
#include "fpga_init.h"
#include "fpga_to_upper.h"
#include "sensor.h"
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
    ptr_fpga_config_para->wsm_registers_value.wsm_mod = (ptr_recv_upper_package->WorkMode) | (ptr_recv_upper_package->LFMMode << 1) | (ptr_recv_upper_package->DataType << 2); 
    if (ptr_recv_upper_package->PingRate!=0)
    {
        ptr_fpga_config_para->wsm_registers_value.wsm_ct = (unsigned int)(FPGA_CLK_FREQUENCY / ptr_recv_upper_package->PingRate);
    }
    else
    {
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
	//PWM初始相位寄存器暂时无用
	ptr_fpga_config_para->pwm_registers_value.pwm_ip_5 = 0;
	memset(ptr_fpga_config_para->pwm_registers_value.pwm_ip_RSV,0,sizeof(ptr_fpga_config_para->pwm_registers_value.pwm_ip_RSV));
	printf("1");
	//串口请求及相关参数配置
	ptr_fpga_config_para->pwm_registers_value.UART_REQ |= 0;
	ptr_fpga_config_para->pwm_registers_value.UART_REQ |= ptr_recv_upper_package->UART_REQ;
	ptr_fpga_config_para->pwm_registers_value.UART_REQ |= (ptr_recv_upper_package->MCP4725_DAC >> 8) << 8;
	ptr_fpga_config_para->pwm_registers_value.UART_REQ |= (ptr_recv_upper_package->MCP4725_DAC & 0xFF) << 16;

    //功率系数赋值或者经过计算后赋值，公式待定
    ptr_fpga_config_para->pwm_registers_value.power_factor = ((unsigned int)(ptr_recv_upper_package->power_factor*10))<<16|(unsigned int)(ptr_recv_upper_package->power_factor *1.23/48/3.3*4096);
	printf("2");
    //pwm 频率和采样率和量程，赋值，fpga回传使用
    ptr_fpga_config_para->pwm_registers_value.pwm_frequency = ptr_recv_upper_package->PWMFreq;
    ptr_fpga_config_para->pwm_registers_value.sample_rate = ptr_recv_upper_package->SamplingRate;
    ptr_fpga_config_para->pwm_registers_value.range = ptr_recv_upper_package->Range; 
    ptr_fpga_config_para->pwm_registers_value.bandwidth = ptr_recv_upper_package->PWMBandWidth; 
    //pwm开关
    ptr_fpga_config_para->pwm_registers_value.pwm_start = ptr_recv_upper_package->pwm_start;
    //抽样后的ad个数，留着fpga使用
    ptr_fpga_config_para->pwm_registers_value.AD_sn_after = (ptr_fpga_config_para->adc_registers_value.adc_sn + 1) / SAMPLE_FACTOR;
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
    Debug("wsm_mod: %d\n", ptr_fpga_config_para->wsm_registers_value.wsm_mod); 
    Debug("wsm_ct: %d\n", ptr_fpga_config_para->wsm_registers_value.wsm_ct); 
    Debug("adc_sct: %d\n", ptr_fpga_config_para->adc_registers_value.adc_sct); 
    Debug("adc_sn: %d\n", ptr_fpga_config_para->adc_registers_value.adc_sn);
    Debug("dac_sct: %d\n", ptr_fpga_config_para->dac_registers_value.dac_sct); 
    Debug("dac_sn: %d\n", ptr_fpga_config_para->dac_registers_value.dac_sn);  
    Debug("pwm_bf: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_bf); 
    Debug("pwm_lfm: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_lfm); 
    Debug("pwm_pulse: %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_pulse);
    Debug("pwm_start %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_start); 
    Debug("sync_state %d\n", ptr_fpga_config_para->pwm_registers_value.sync_state); 
    Debug("sync_plustype %d\n", ptr_fpga_config_para->pwm_registers_value.sync_selec); 
    Debug("sync_delaytime %d\n", ptr_fpga_config_para->pwm_registers_value.sync_delaytime); 

    //Debug("power_factor %d\n", ptr_fpga_config_para->pwm_registers_value.power_factor);
    Debug("pwm_frequency %d\n", ptr_fpga_config_para->pwm_registers_value.pwm_frequency);
    //Debug("sample_rate %d\n", ptr_fpga_config_para->pwm_registers_value.sample_rate);
    //Debug("range %d\n", ptr_fpga_config_para->pwm_registers_value.range);
    //Debug("bandwidth %d\n", ptr_fpga_config_para->pwm_registers_value.bandwidth);
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
 * 名称：                    clear_sensor_data
 * 功能：                    清传感器数据（断开时需要清传感器缓存）
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void clear_sensor_data(void)
{
    int m;
    for (m=0;m<=50;m++)
    {
        *(uio_sensor_mem_0.mem_ptr+m*64)=0;
        *(uio_sensor_mem_0.mem_ptr+m*64+1)=0;
        *(uio_sensor_mem_1.mem_ptr+m*64)=0;
        *(uio_sensor_mem_1.mem_ptr+m*64+1)=0;
        *(uio_sensor_mem_2.mem_ptr+m*64)=0;
        *(uio_sensor_mem_2.mem_ptr+m*64+1)=0;
        *(uio_sensor_mem_3.mem_ptr+m*64)=0;
        *(uio_sensor_mem_3.mem_ptr+m*64+1)=0;
        *(uio_sensor_mem_4.mem_ptr+m*64)=0;
        *(uio_sensor_mem_4.mem_ptr+m*64+1)=0;
        *(uio_sensor_mem_8.mem_ptr+m*64)=0;
        *(uio_sensor_mem_8.mem_ptr+m*64+1)=0;
        *(uio_sensor_mem_9.mem_ptr+m*64)=0;
        *(uio_sensor_mem_9.mem_ptr+m*64+1)=0;
        *(uio_sensor_mem_A.mem_ptr+m*64)=0;
        *(uio_sensor_mem_A.mem_ptr+m*64+1)=0;
        *(uio_sensor_mem_B.mem_ptr+m*64)=0;
        *(uio_sensor_mem_B.mem_ptr+m*64+1)=0;
        *(uio_sensor_mem_C.mem_ptr+m*64)=0;
        *(uio_sensor_mem_C.mem_ptr+m*64+1)=0;
    }
}

/********************************************************************************
 * 名称：                    error_process
 * 功能：                    异常断网的处理机制（如上位机界面闪退或者异常关闭）
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void error_process(void)
{
    printf("Network interrupt!.\n");
    netStatus = 0;
    ptr_fpga_register_data->wsm_con=0;
    // Send_ss_status=0;
    Fpga_start_mod = 0;
    //Send_data_status=0;
    flag=0;
    close(Connect_fd);
    clear_sensor_data();
}

void config_sensor_baud_and_framehead(const RECV_UPPER_CONFIG_PARAMETERS *ptr_recv_upper_package)
{
	fpga_register_data.fpga_sensor.GGA_ZDA_BAUD = FPGA_CLK_FREQUENCY/ptr_recv_upper_package->sensor_boud_GGA_ZDA; //波特率38400
	fpga_register_data.fpga_sensor.HEADING_BAUD =  FPGA_CLK_FREQUENCY/ptr_recv_upper_package->sensor_boud_HEADING; 
	fpga_register_data.fpga_sensor.AT_BAUD= FPGA_CLK_FREQUENCY/ptr_recv_upper_package->sensor_boud_AT;
	fpga_register_data.fpga_sensor.SVT_BAUD= FPGA_CLK_FREQUENCY/ptr_recv_upper_package->sensor_boud_SVT;
	fpga_register_data.fpga_sensor.GGA_ZDA_FH =(unsigned int)ptr_recv_upper_package->sensor_fh_GGA_ZDA;//4724;//$G改成$ 
	fpga_register_data.fpga_sensor.HEADING_FH =(unsigned int) ptr_recv_upper_package->sensor_fh_HEADING; //0x4824;//$H改成$ 
	fpga_register_data.fpga_sensor.AT_FH=(unsigned int)ptr_recv_upper_package->sensor_fh_AT;//0x303A;//:0改成:
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

/*
	设置声纳参数
*/
typedef struct
{
	char dev_cmd[3];
	const char *dbg_cmd;
	const char *idle_msg;
	const char *print_prefix;
	int syncstatus_value;
	unsigned int fpga_sync_state;
	char upper_sync_state;
	int update_upper_sync_params;
} SYNC_CONFIG;

static int match_sync_command(const char dev_sync[3], const SYNC_CONFIG *config)
{
	return (dev_sync[0] == config->dev_cmd[0])
		&& (dev_sync[1] == config->dev_cmd[1])
		&& (dev_sync[2] == config->dev_cmd[2]);
}

static int send_sync_ack_if_idle(const char *idle_msg)
{
	if (ptr_fpga_register_data->fpga_sta.wstatus != 0)
	{
		return 0;
	}

	DBG("%s\n", idle_msg);
	if (send(Connect_fd, "<<YS", 4, 0) < 0)
	{
		error_process();
		return -1;
	}

	return 0;
}

static int recv_sync_bytes(void *buf, unsigned int len)
{
	unsigned int recv_len = 0;
	char *ptr = (char *)buf;

	while (recv_len < len)
	{
		int current_len = recv(Connect_fd, ptr + recv_len, len - recv_len, 0);
		if (current_len <= 0)
		{
			error_process();
			return -1;
		}
		recv_len += current_len;
	}

	return 0;
}

static int recv_sync_parameters(char *sync_type, float *sync_delay_time)
{
	if (recv_sync_bytes(sync_type, 1) < 0)
	{
		return -1;
	}

	if (recv_sync_bytes(sync_delay_time, 4) < 0)
	{
		return -1;
	}

	return 0;
}

static void commit_sync_config_to_fpga(void)
{
	ptr_fpga_register_data->set_pr = 1;
	usleep(1000);//1ms
	ptr_fpga_register_data->set_pr = 0;
}

static void apply_sync_config(const SYNC_CONFIG *config, char sync_type, float sync_delay_time)
{
	unsigned int sync_delay = (unsigned int)((int)(sync_delay_time * 100));

	syncstatus = config->syncstatus_value;
	ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state = config->fpga_sync_state;
	ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_selec = sync_type;
	ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_delaytime = sync_delay;

	ptr_send_upper_status_information->sync_state = config->upper_sync_state;
	if (config->update_upper_sync_params)
	{
		ptr_send_upper_status_information->sync_selec = sync_type;
		ptr_send_upper_status_information->sync_delaytime = sync_delay_time;
	}

	printf("%s SyncPlus_Type = %hhu\n", config->print_prefix, sync_type);
	printf("%s SyncPlus_Delaytime = %f\n", config->print_prefix, sync_delay_time);
	printf("%s sync_delay = %d\n", config->print_prefix, sync_delay);
	printf("%s sync_state = %d\n", config->print_prefix, ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state);

	commit_sync_config_to_fpga();
}

static int process_sync_command(const char dev_sync[3])
{
	static const SYNC_CONFIG sync_configs[] = {
		{{'_', 'I', 'N'}, "<<SY_IN", "=======同步输入=======", "syn_in",  1,  9, 1, 1},
		{{'_', 'O', 'T'}, "<<SY_OT", "=======同步输出=======", "syn_out", 1, 10, 2, 1},
		{{'_', 'N', 'O'}, "<<SY_NO", "=======非同步=======",   "syn_no",  0,  0, 0, 0}
	};
	char sync_type;
	float sync_delay_time;
	unsigned int i;

	for (i = 0; i < sizeof(sync_configs) / sizeof(sync_configs[0]); i++)
	{
		if (!match_sync_command(dev_sync, &sync_configs[i]))
		{
			continue;
		}
		DBG("%s\n", sync_configs[i].dbg_cmd);
		if (send_sync_ack_if_idle(sync_configs[i].idle_msg) < 0)
		{
			return -2;
		}

		if (recv_sync_parameters(&sync_type, &sync_delay_time) < 0)
		{
			return -1;
		}

		apply_sync_config(&sync_configs[i], sync_type, sync_delay_time);
		printf("======================================================\n");
		fpga_register_data.fpga_registers_200k = ptr_fpga_register_data->fpga_registers_200k;
		Debug_pritf_convert_fpga_parameter(&(fpga_register_data.fpga_registers_200k));//打印
		return 1;
	}
	DBG("unknown sync command: %c%c%c\n", dev_sync[0], dev_sync[1], dev_sync[2]);
	return 0;
}

int set_fpga_work_parameter(void){
	int len_recv;
	int len_recv_crc;
	unsigned short recv_crc_calculate, recv_crc;
	char DataTail[4];

	len_recv = recv_socket(Connect_fd, (char *)(ptr_recv_upper_package), SIZE_OF_CONFIG_PARA);
	if(len_recv < 0){
		error_process();
		return -1;
	}
	len_recv_crc=recv_socket(Connect_fd,(char *)&recv_crc,SIZE_OF_LONG);
	if (len_recv_crc < 0)
	{
		error_process();
		return -1;
	}
	recv_socket(Connect_fd, DataTail, SIZE_OF_LONG); 
	recv_crc_calculate= crc16((char *)(ptr_recv_upper_package),SIZE_OF_CONFIG_PARA,0xFFFF);
	if (recv_crc_calculate!=recv_crc)
	{
		/*
		printf("recv_crc_calculate:%x \n",recv_crc_calculate);
		printf("recv_crc:%x \n",recv_crc);
		printf("crc error! \n");
		*/
		return -1;
	}
	copy_recv_cmd(&cmd_package, ptr_recv_upper_package);//拷贝到本地
	Debug_pritf_receive_para(&cmd_package);//打印
	if(parsing_instructions_200k(&cmd_package, &(fpga_register_data.fpga_registers_200k)) == -1) //解析fpga寄存器
	{
		return -1;
	}
	Debug_pritf_convert_fpga_parameter(&(fpga_register_data.fpga_registers_200k));//打印
	ptr_fpga_register_data->fpga_registers_200k= fpga_register_data.fpga_registers_200k;//下发FPGA参数配置命令 
	config_sensor_baud_and_framehead(&cmd_package);
	Debug_pritf_sensor_baud_and_framehead(&(fpga_register_data.fpga_sensor)); //打印
	ptr_fpga_register_data->fpga_sensor = fpga_register_data.fpga_sensor; //下发传感器参数配置命令 
	//下发TVG数据
	memset(uio_tvg_register.mem_ptr, 0, SIZE_OF_TVG_MAX); //剩余tvg置0 
	memcpy((char *)uio_tvg_register.mem_ptr, (char *)&cmd_package.tvgGain, fpga_register_data.fpga_registers_200k.dac_registers_value.dac_sn*4);//下发tvg数据            
	//参数更新中断
	ptr_fpga_register_data->set_pr=1;
	usleep(1000);//1ms 
	ptr_fpga_register_data->set_pr=0;
	return 1;
}	


/********************************************************************************
* 名称：                    receive_process_sendto_fpga
* 功能：                    上位机--->ARM--->FPGA主线程
* 入口参数：            	 无
* 出口参数：            	 无
*********************************************************************************/
int syncstatus;
void receive_process_sendto_fpga(void)
{
	int len_recv_head1, len_recv_head2;
	char DataHead1[2];
	char DataHead2[2];
	char DEV_SYNC[3];

	while (1)
	{
		if (netStatus == 1)
		{
			DataHead1[0] = 0;
			DataHead1[1] = 0;
			while ((DataHead1[0] != '<') || (DataHead1[1] != '<'))
			{
				len_recv_head1 = recv(Connect_fd, DataHead1, 2, 0);
				if (len_recv_head1 <= 0)
				{
					error_process();
					break;
				}
			}

			if (netStatus == 0)
			{
				continue;
			}

			len_recv_head2 = recv(Connect_fd, DataHead2, 2, 0);
			if (len_recv_head2 < 0)
			{
				error_process();
				continue;
			}

			printf("DataHead2: %c %c\n", DataHead2[0], DataHead2[1]);
			if (netStatus == 0)
			{
				continue;
			}

			if ((DataHead2[0] == 'R') && (DataHead2[1] == 'S'))//3.request status，请求硬件工作状态信息指令
			{
				pthread_mutex_lock(&mut);
				send_status_to_upper();
				pthread_mutex_unlock(&mut);
			}

			if ((DataHead2[0] == 'U') && (DataHead2[1] == 'P'))//更新BOOT.BIN和image.ub
			{

			}

			if ((DataHead2[0] == 'B') && (DataHead2[1] == 'R'))//4.Begin Request Beam Data，开始请求数据
			{
				DBG("<<BR\n");
				ptr_fpga_register_data->wsm_con = 1;
				Fpga_start_mod = 1;
			}

			if ((DataHead2[0] == 'E') && (DataHead2[1] == 'R'))//5.End RequestBeam Data停止请求波形数据
			{
				DBG("<<ER\n");
				ptr_fpga_register_data->wsm_con = 0;
				Fpga_start_mod = 0;
				clear_sensor_data();
			}

			if ((DataHead2[0] == 'S') && (DataHead2[1] == 'P'))//6.set parameter，设置声呐的各项参数 
			{
				int ret;

				DBG("<<SP\n");
				pthread_mutex_lock(&mut);
				ret = set_fpga_work_parameter();
				pthread_mutex_unlock(&mut);

				if (ret < 0)
				{
					printf("set_fpga_work_parameter error!\n");
					continue;
				}
			}

			if ((DataHead2[0] == 'S') && (DataHead2[1] == 'Y'))//设备同步输入输出命令
			{
				int sync_ret;

				len_recv_head1 = recv(Connect_fd, DEV_SYNC, 3, 0);
				if (len_recv_head1 <= 0)
				{
					error_process();
					continue;
				}

				sync_ret = process_sync_command(DEV_SYNC);
				if (sync_ret == -2)
				{
					return;
				}
				if (sync_ret < 0)
				{
					continue;
				}
			}
		}
		else
		{
			if ((Connect_fd = accept(Socket_fd_server, (struct sockaddr *)NULL, NULL)) == -1)
			{
				printf("accept socket error: %s(errno: %d)", strerror(errno), errno);
				continue;
			}//阻塞
			netStatus = 1;
			DBG("======connect complete!======\n");
		}
	}
}
