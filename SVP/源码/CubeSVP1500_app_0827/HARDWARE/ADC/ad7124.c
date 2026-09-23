#include "ad7124.h"
#include "delay.h"

uint8_t AD_ID1_REG;		//复位值为0x12或0x14
volatile uint8_t DATA_STATUS;	//获取的通道号
uint8_t AD_Gain=16;
uint8_t HEX_Gain=4;

/*
 * 旧版硬件的压力测量使用 AD7124 三个通道轮询：
 * - 通道 0：PT100 温度；
 * - 通道 2：压力传感器参考电压；
 * - 通道 1：压力传感器输出电压。
 *
 * 新版硬件只调用 AD7124_DATA() 获取 PT100 温度，压力由 IIC 传感器负责。
 * 因此旧版专用状态全部集中在本文件，不能被新版 IIC 压力路径使用。
 */
#define AD7124_LEGACY_PRESSURE_GAIN       16.0f
#define AD7124_LEGACY_REFERENCE_GAIN       2.0f
#define AD7124_LEGACY_ZERO_SAMPLE_COUNT   15U

float pa_v = 0.0f;
volatile float frist_pav = 0.0f;
static float ad7124_legacy_first_depth = 0.0f;
static float ad7124_legacy_sum_depth = 0.0f;
static uint8_t ad7124_legacy_zero_count = 0U;

/*旧版本声剖压力采集极性配置*/
//压力采集通道模式配置宏
#define CHANNEL				    1	//1多通道 0单通道
////单双极性配置宏
//#define PA_V_POLARITY			1	//1单极性 0双极性
//#define REF_V_POLARITY			1	//1单极性 0双极性
  
    
#define PA_V_POLARITY			0	//1单极性 0双极性
#define REF_V_POLARITY			1	//1单极性 0双极性

void AD7124_PT100_Init(void)
{   
    //复位
    AD7124_Reset();  
	
     /*IO_Control_1 */ 
    AD7124_Write_Reg(AD7124_IO_CTRL1_REG ,3,
        AD7124_IO_CTRL1_REG_PDSW    |       //电桥关断开关控制位
        AD7124_IO_CTRL1_REG_IOUT1(4)   |    //IOUT1激励电流的值  
        AD7124_IO_CTRL1_REG_IOUT0(4)    |   //IOUT0激励电流的值  
        AD7124_IO_CTRL1_REG_IOUT_CH1(1) |   //IOUT1激励电流的通道选择位 AIN1
        AD7124_IO_CTRL1_REG_IOUT_CH0(0)     //IOUT0激励电流的通道选择位 AIN0  
    );
	
//		delay_ms(30);
//		AD7124_Write_Reg(AD7124_CH0_MAP_REG,2,0X0000);
		delay_ms(30);
	
//PT1000
        /*ch0 reg*/
        AD7124_Write_Reg(AD7124_CH0_MAP_REG  ,2,
            AD7124_CH_MAP_REG_CH_ENABLE |   //通道0使能
            AD7124_CH_MAP_REG_SETUP(0)  |   //SETUP3
            AD7124_CH_MAP_REG_AINP(2)   |   //AIN2
            AD7124_CH_MAP_REG_AINM(3)       //AIN3   
        );
        AD7124_Write_Reg(AD7124_CFG0_REG,2,
            AD7124_CFG_REG_UNIPOLAR     |   //单极性
            AD7124_CFG_REG_BURNOUT(0)   |   //这些位选择传感器开路检测电流源的幅度|
            AD7124_CFG_REG_REF_BUFP     |
            AD7124_CFG_REG_REF_BUFM     |
            AD7124_CFG_REG_AIN_BUFP     |
            AD7124_CFG_REG_AINN_BUFM    |
            AD7124_CFG_REG_REF_SEL(0)   |   // 基准电压选择00 REFV1+/REFV1-    01REFV2+/REFV2-  10 2.5  11 3.3
            AD7124_CFG_REG_PGA(5)           //增益选择位 32
        );
        AD7124_Write_Reg(AD7124_FILT0_REG,3,    
            AD7124_FILT_REG_FILTER(4)   |       //滤波器类型选择位
			AD7124_FILT_REG_REJ60 		|
            AD7124_FILT_REG_POST_FILTER(0)  |   //后置滤波器类型选择位
            AD7124_FILT_REG_FS(20)  
        ); 
		delay_ms(30);
//PA        
        //REFV2+/REFV2-
        AD7124_Write_Reg(AD7124_CH2_MAP_REG  ,2,
            AD7124_CH_MAP_REG_CH_ENABLE |   //通道2使能
            AD7124_CH_MAP_REG_SETUP(2)  |   //SETUP2
            AD7124_CH_MAP_REG_AINP(6)   |   //AIN6
            AD7124_CH_MAP_REG_AINM(7)       //AIN7
        );
		delay_ms(30);
        AD7124_Write_Reg(AD7124_CFG2_REG,2,
            AD7124_CFG_REG_UNIPOLAR     | 
			AD7124_CFG_REG_AIN_BUFP	|
			AD7124_CFG_REG_AINN_BUFM |
            AD7124_CFG_REG_REF_SEL(2)   |   //AVDD 基准电压源 2.5f。
            AD7124_CFG_REG_PGA(1)           //增益选择位 2
        );
		delay_ms(30);
        AD7124_Write_Reg(AD7124_FILT2_REG,3,
            AD7124_FILT_REG_FILTER(4)   |       //滤波器类型选择位
			AD7124_FILT_REG_REJ60 		|
            AD7124_FILT_REG_POST_FILTER(0)  |   //后置滤波器类型选择位
            AD7124_FILT_REG_FS(20)  
        );
		
		delay_ms(30);
        /*ch1 reg*/
        AD7124_Write_Reg(AD7124_CH1_MAP_REG  ,2,
            AD7124_CH_MAP_REG_CH_ENABLE |   //通道1使能
            AD7124_CH_MAP_REG_SETUP(1)  |   //SETUP1
			
            AD7124_CH_MAP_REG_AINP(4)   |   //AIN
            AD7124_CH_MAP_REG_AINM(5)       //AIN   
        );
		delay_ms(30);
#if PA_V_POLARITY
        AD7124_Write_Reg(AD7124_CFG1_REG,2,
            AD7124_CFG_REG_UNIPOLAR     |  
            AD7124_CFG_REG_AIN_BUFP	|
			AD7124_CFG_REG_AINN_BUFM |
            AD7124_CFG_REG_REF_SEL(1)   |   // 基准电压选择
            AD7124_CFG_REG_PGA(4)           //增益选择位 16
        );
#else
        AD7124_Write_Reg(AD7124_CFG1_REG,2,
            AD7124_CFG_REG_BIPOLAR     |  
            AD7124_CFG_REG_AIN_BUFP	|
			AD7124_CFG_REG_AINN_BUFM |
            AD7124_CFG_REG_REF_SEL(1)   |   // 基准电压选择
            AD7124_CFG_REG_PGA(4)           //增益选择位 16
        );
#endif
		delay_ms(30);
        AD7124_Write_Reg(AD7124_FILT1_REG,3,    
            AD7124_FILT_REG_FILTER(4)   |       //滤波器类型选择位
			AD7124_FILT_REG_REJ60 		|
            AD7124_FILT_REG_POST_FILTER(0)  |   //后置滤波器类型选择位
            AD7124_FILT_REG_FS(20)              //滤波器输出数据速率选择位
        );
		
		delay_ms(30);
		
		 /*ADC_CONTROL寄存器 全功耗 连续工作模式 内部时钟*/
		AD7124_Write_Reg(AD7124_ADC_CTRL_REG ,2,
			AD7124_ADC_CTRL_REG_DATA_STATUS |        //每次数据寄存器读操作之后，状态寄存器内容传输的使能位
			AD7124_ADC_CTRL_REG_POWER_MODE(2)  |     //全功率
			AD7124_ADC_CTRL_REG_REF_EN          |    //内部基准电压使能   
			AD7124_ADC_CTRL_REG_MODE(0)         |    //0000 连续转换模式
			AD7124_ADC_CTRL_REG_CLK_SEL(0)           //内部时钟
		);  
}

/*
	打开通道2 - REF  关闭其他通道
*/
void enble_ch2(void){
	/*打开通道2*/
       AD7124_Write_Reg(AD7124_CH2_MAP_REG  ,2,
            AD7124_CH_MAP_REG_CH_ENABLE |   //
            AD7124_CH_MAP_REG_SETUP(2)  |   //SETUP2
            AD7124_CH_MAP_REG_AINP(6)   |   //AIN6
            AD7124_CH_MAP_REG_AINM(7)       //AIN7
        );
	/*关闭通道0 1*/
	AD7124_Write_Reg(AD7124_CH0_MAP_REG,2,AD7124_CH_MAP_REG_CH_DISABLE);
	AD7124_Write_Reg(AD7124_CH1_MAP_REG,2,AD7124_CH_MAP_REG_CH_DISABLE);
}
/*
	打开通道1 - PAV 关闭其他通道
*/
void enble_ch1(void){
	/*打开通道1*/
	AD7124_Write_Reg(AD7124_CH1_MAP_REG  ,2,
		AD7124_CH_MAP_REG_CH_ENABLE |   //通道使能1
		AD7124_CH_MAP_REG_SETUP(1)  |   //SETUP1
		AD7124_CH_MAP_REG_AINP(4)   |   //AIN
		AD7124_CH_MAP_REG_AINM(5)       //AIN   
	);
	/*关闭通道0 2*/
	AD7124_Write_Reg(AD7124_CH0_MAP_REG,2,AD7124_CH_MAP_REG_CH_DISABLE);
	AD7124_Write_Reg(AD7124_CH2_MAP_REG,2,AD7124_CH_MAP_REG_CH_DISABLE);
}

/*
	打开通道0 - TEMP 关闭其他通道
*/
void enble_ch0(void){
	/*打开通道0*/
	AD7124_Write_Reg(AD7124_CH0_MAP_REG  ,2,
            AD7124_CH_MAP_REG_CH_ENABLE |   //通道3使能
            AD7124_CH_MAP_REG_SETUP(0)  |   //SETUP3
            AD7124_CH_MAP_REG_AINP(2)   |   //AIN2
            AD7124_CH_MAP_REG_AINM(3)       //AIN3   
        );
	/*关闭通道1 2*/
	AD7124_Write_Reg(AD7124_CH1_MAP_REG,2,AD7124_CH_MAP_REG_CH_DISABLE);
	AD7124_Write_Reg(AD7124_CH2_MAP_REG,2,AD7124_CH_MAP_REG_CH_DISABLE);
}

/*
 * 初始化旧版硬件的 AD7124 三通道采集。
 *
 * AD7124_PT100_Init() 先完成公共复位和 PT100 温度通道配置；
 * 本函数随后补齐旧版压力传感器所需的参考电压通道和压力通道配置。
 * 最后强制切回温度通道，使每次上电后的轮询起点确定为：
 * 温度 -> 参考电压 -> 压力。
 */
void AD7124_Legacy_Init(void)
{
    AD7124_PT100_Init();

    /*
     * 系数为 0 时温度计算会失效。该保护与新版 AD7124_Init() 保持一致，
     * 不覆盖用户已经保存在 EEPROM 中的其他标定参数。
     */
    if (svp_cmd.TEMP_COE_A == 0.0f)
    {
        svp_cmd.TEMP_COE_A = 1.0f;
    }

    /*
     * 旧板压力零点只允许在本次上电后重新建立，不能继承上一次运行的 RAM 值。
     * 这样可避免重新上电或切换板卡后，旧零点影响新的深度结果。
     */
    pa_v = 0.0f;
    frist_pav = 0.0f;
    ad7124_legacy_first_depth = 0.0f;
    ad7124_legacy_sum_depth = 0.0f;
    ad7124_legacy_zero_count = 0U;

    enble_ch0();
}


/**
  * @brief  AD7124的读写SPI操作
  * @param  Data : 需要传输的数据
  * @retval SPI读到的数据
  */


uint32_t DATA=0;
uint32_t Rd_Ary[3];

uint32_t AD7124_Read_Data(uint8_t byte)
{
	for(int8_t i=0; i<byte; i++)
	{
		Rd_Ary[i] = SPI_ReadWriteByte(&SPI2_Config,0xFF);
        
	}
	DATA = (Rd_Ary[0]<<16) + (Rd_Ary[1]<<8) + Rd_Ary[2];
	return DATA;
}

void AD7124_Reset(void)
{
	AD7124_CS = 0;
	//提供大于64个写操作，复位AD7124
	for(uint8_t a=0; a<9; a++)
	{
		SPI_ReadWriteByte(&SPI2_Config,0xFF);
	}
	delay_us(60);
	AD7124_CS = 1;
}

//函数:	读AD7124寄存器
//变量:	addr:寄存器地址 byte:要写入的字节数
//返回值: reg  Ye
uint32_t AD7124_Read_Reg(uint8_t addr,uint8_t byte)
{
	uint32_t reg=0;
	AD7124_CS = 0;
	SPI_ReadWriteByte(&SPI2_Config,0x40 | addr);
	
	switch(byte)
	{
		case 1:
			reg = SPI_ReadWriteByte(&SPI2_Config,0XFF);
			break;
		case 2:
			reg = SPI_ReadWriteByte(&SPI2_Config,0XFF);
			reg <<= 8;
			reg |= SPI_ReadWriteByte(&SPI2_Config,0XFF);
			break;
		case 3:
			reg = SPI_ReadWriteByte(&SPI2_Config,0XFF);
			reg <<= 16;
			reg |= SPI_ReadWriteByte(&SPI2_Config,0XFF);
			reg <<= 8;
			reg |= SPI_ReadWriteByte(&SPI2_Config,0XFF);
			break;
	}
	delay_us(100);
	AD7124_CS = 1;
	return reg;
}

//函数:	写AD7124寄存器
//变量:	addr:寄存器地址 byte:要写入的字节数 data 写入值
//返回值:无
uint16_t AD7124_Write_Reg(uint8_t addr,uint8_t byte,uint32_t data)
{
	uint32_t reg=0;
	AD7124_CS = 0;
	SPI_ReadWriteByte(&SPI2_Config,addr);
	
	switch(byte)
	{
		case 1:
			SPI_ReadWriteByte(&SPI2_Config,data >> 0);
			break;
		case 2:
			SPI_ReadWriteByte(&SPI2_Config,data >> 8);
			SPI_ReadWriteByte(&SPI2_Config,data >> 0);
			break;
		case 3:
			SPI_ReadWriteByte(&SPI2_Config,data >> 16);
			SPI_ReadWriteByte(&SPI2_Config,data >> 8);
			SPI_ReadWriteByte(&SPI2_Config,data >> 0);
			break;
	}
	delay_us(100);
	AD7124_CS = 1;
	return reg;
}


//函数:	读取ID
uint16_t Get_AD7124_ID(void)
{
	uint16_t ID;
	ID=0;
	ID=AD7124_Read_Reg(AD7124_ID_REG,1);
	return ID;
}

uint32_t regdata = 0;
uint32_t AD7124_READ_DATAREG(void){
    uint32_t Data;

    AD7124_CS = 0;
    SPI_ReadWriteByte(&SPI2_Config,0x42);			//读操作
    Data = AD7124_Read_Data(3);			//Data采集结果

    /*
     * ADC_CONTROL.DATA_STATUS 已在初始化时使能。每次读完 24 位转换结果后，
     * AD7124 还会继续输出一个状态字节，其中低 4 位就是这次结果所属的通道号。
     * 必须把第 4 个字节完整读走；否则状态字节未被时钟移出，后续寄存器访问和
     * 通道判断都可能出现错位。
     */
    DATA_STATUS = (uint8_t)(SPI_ReadWriteByte(&SPI2_Config,0xFF) & 0x0FU);
	delay_us(100);
    AD7124_CS= 1;
    regdata = Data;
    return Data;
}
float calculate_value(float temp){
    const float R0 =100.0f;
    const float A = 3.9083e-3f;
    const float B = -5.775e-7f;
    float temperature = 0.0f;
    if(temp >= R0){
        temperature = (-A + __sqrtf(A * A - 4.0f * B * (1.0f - temp / R0))) / (2.0f*B);
    }
    return temperature;
}

/*
 * 旧版 AD7124 三通道采集状态机。
 *
 * 每次调用只处理 AD7124 当前完成转换的一个通道，并将下一个需要采集的通道打开。
 * 不能一次性连续读取三个通道：切换模拟通道后必须等待下一次转换完成，
 * 否则压力参考电压、压力值和温度值会发生通道错配。
 */
void AD7124_Legacy_DATA(void)
{
    uint32_t data_reg;

    data_reg = AD7124_READ_DATAREG();
    /* DATA_STATUS 已在 AD7124_READ_DATAREG() 中与本次数据同步取得。 */

    switch (DATA_STATUS)
    {
        case 0:
        {
            float resistance;

            /*
             * 温度通道：保持原 PT100 电阻换算及 EEPROM 标定系数。
             * 温度范围异常时按原逻辑置零，避免异常值参与直读显示和记录。
             */
            resistance = (float)data_reg * 3900.0f / (2.0f * 0x800000) / 32.0f;
            PT100_TEMP = calculate_value(resistance) *
                         svp_cmd.TEMP_COE_A +
                         svp_cmd.TEMP_COE_B;
            if ((PT100_TEMP > 100.0f) || (PT100_TEMP < 0.0f))
            {
                PT100_TEMP = 0.0f;
            }

            /* 温度完成后读取压力参考电压。 */
            enble_ch2();
            break;
        }

        case 2:
        {
            /*
             * 参考电压通道：采用旧工程的单极性换算公式。
             * 压力通道必须使用这次采到的参考值，不能长期复用旧参考值。
             */
            frist_pav = ((float)data_reg * 2.5f) /
                         (2.0f * 0x800000 * AD7124_LEGACY_REFERENCE_GAIN);

            /* 参考电压完成后读取压力传感器输出。 */
            enble_ch1();
            break;
        }

        case 1:
        {
            float sensitivity;
            float raw_depth;
            float scaled_depth;
            float calibrated_depth;

            /*
             * 压力通道采用双极性换算。首次轮询若参考电压尚未有效，
             * 不更新 Depth，直接回到温度通道等待下一轮完整数据。
             */
            if (frist_pav <= 0.0f)
            {
                enble_ch0();
                break;
            }

            pa_v = (frist_pav / AD7124_LEGACY_PRESSURE_GAIN) *
                   ((float)data_reg / 0x800000 - 1.0f);

            /*
             * PA_COE_I 或 PA_COE_M 为零意味着压力标定参数无效。
             * 此时禁止除零并保持上一次已确认的 Depth，避免突然写入异常深度。
             */
            if ((svp_cmd.PA_COE_I == 0.0f) || (svp_cmd.PA_COE_M == 0.0f))
            {
                enble_ch0();
                break;
            }

            /*
             * 以下公式保留旧正式工程的物理换算与补偿顺序：
             * 先将压力电压换算为深度，再应用 PA_COE 和 DEPTH_EC 参数。
             * 这样旧设备 EEPROM 内已有的标定参数可以直接继续使用。
             */
            sensitivity = ((svp_cmd.PA_COE_I / 3.0f) /
                          svp_cmd.PA_COE_M) / 10.0f;
            if (sensitivity == 0.0f)
            {
                enble_ch0();
                break;
            }

            raw_depth = (pa_v * 1000.0f / sensitivity) * 10.0f;
            scaled_depth = raw_depth * svp_cmd.PA_COE_E + svp_cmd.PA_COE_A;

            /*
             * 与旧正式版一致：压力原始换算结果为零时，不把该样本用于上电零点平均。
             * 仅在零点尚未建立时清空统计，防止无效零值和有效样本混合。
             * 已完成零点建立后保留原零点并忽略该异常样本，避免设备在水下遇到
             * 单次零值毛刺后重新把当前水深当作新的零点。
             */
            if (scaled_depth == 0.0f)
            {
                if (ad7124_legacy_zero_count < AD7124_LEGACY_ZERO_SAMPLE_COUNT)
                {
                    ad7124_legacy_zero_count = 0U;
                    ad7124_legacy_sum_depth = 0.0f;
                }
                enble_ch0();
                break;
            }

            calibrated_depth = scaled_depth;

            if ((svp_cmd.PA_COE_E != 1.0f) || (svp_cmd.PA_COE_A != 0.0f))
            {
                calibrated_depth = (calibrated_depth * svp_cmd.DEPTH_EC_X) +
                                   (calibrated_depth - svp_cmd.DEPTH_EC_Y);
            }

            /*
             * 每次上电后的前 15 个有效压力样本用于计算零点偏移。
             * 零点确定后不再改变，保证设备入水后的深度变化不会被动态归零抵消。
             */
            if (ad7124_legacy_zero_count < AD7124_LEGACY_ZERO_SAMPLE_COUNT)
            {
                ad7124_legacy_sum_depth += calibrated_depth;
                ad7124_legacy_zero_count++;

                if (ad7124_legacy_zero_count == AD7124_LEGACY_ZERO_SAMPLE_COUNT)
                {
                    ad7124_legacy_first_depth =
                        ad7124_legacy_sum_depth / AD7124_LEGACY_ZERO_SAMPLE_COUNT;
                }
            }

            Depth = calibrated_depth - ad7124_legacy_first_depth;

            /* 压力完成后回到温度通道，进入下一轮采集。 */
            enble_ch0();
            break;
        }

        default:
            /*
             * 状态寄存器出现未定义通道号时保持当前数据不变。
             * 不强制切换通道，避免故障瞬态下打乱 AD7124 正在进行的转换。
             */
            break;
    }
}






/*
 * AD7124 独占测试中等待一次转换完成。
 *
 * AD7124 的 STATUS.RDY 为 0 表示数据寄存器已经有一组完整的新数据。
 * 这里使用状态寄存器轮询，不依赖 DOUT/RDY 引脚，方便确认芯片是否真的
 * 进入转换状态。250 ms 明显大于当前滤波器配置的正常单次转换等待时间。
 */
static uint8_t AD7124_Diagnostic_WaitDataReady(uint8_t *status, uint16_t timeout_ms)
{
    uint16_t elapsed_ms = 0U;

    while (elapsed_ms < timeout_ms)
    {
        *status = (uint8_t)AD7124_Read_Reg(AD7124_STATUS_REG, 1);

        if ((*status & AD7124_STATUS_REG_RDY) == 0U)
        {
            return 1U;
        }

        delay_ms(1);
        elapsed_ms++;
    }

    return 0U;
}

/*
 * 强制读取一个已经切换完成的通道。
 *
 * 成功时返回 1，并带回完整状态字、24 位 ADC 原始码和错误寄存器；
 * 超时时返回 0，仍会读出错误寄存器，便于分辨是未转换还是芯片报错。
 */
/*
 * 诊断模式下读取一组完整数据帧。
 *
 * 当前 ADC_CTRL 已打开 DATA_STATUS，因此数据寄存器在 24 位转换码之后
 * 还会附带 1 字节状态。本函数必须连续读完 4 字节后再拉高片选，
 * 避免把状态字节残留在本次 SPI 数据帧中而干扰下一条读命令。
 */
static uint8_t AD7124_Diagnostic_ReadChannel(uint32_t *data,
                                              uint8_t *status,
                                              uint8_t *data_status,
                                              uint32_t *error)
{
    if (AD7124_Diagnostic_WaitDataReady(status, 250U) == 0U)
    {
        *data = 0U;
        *data_status = 0U;
        *error = AD7124_Read_Reg(AD7124_ERR_REG, 3);
        return 0U;
    }

    AD7124_CS = 0;
    SPI_ReadWriteByte(&SPI2_Config, 0x42);
    *data = ((uint32_t)SPI_ReadWriteByte(&SPI2_Config, 0xFF) << 16) |
            ((uint32_t)SPI_ReadWriteByte(&SPI2_Config, 0xFF) << 8) |
            ((uint32_t)SPI_ReadWriteByte(&SPI2_Config, 0xFF));
    *data_status = SPI_ReadWriteByte(&SPI2_Config, 0xFF);
    delay_us(100);
    AD7124_CS = 1;

    *error = AD7124_Read_Reg(AD7124_ERR_REG, 3);
    return 1U;
}

/*
 * 输出当前转换状态、三路通道映射及其对应的配置和滤波寄存器。
 * 每次切换通道后打印一次，可以确认硬件实际只有一个通道被使能，
 * 并确认初始化写入值没有在 SPI 通讯过程中丢失。
 */
static void AD7124_Diagnostic_PrintRegisters(void)
{
    printf("REG: ADC=0x%04lx IO1=0x%06lx ST=0x%02lx ERR=0x%06lx\r\n",
           AD7124_Read_Reg(AD7124_ADC_CTRL_REG, 2),
           AD7124_Read_Reg(AD7124_IO_CTRL1_REG, 3),
           AD7124_Read_Reg(AD7124_STATUS_REG, 1),
           AD7124_Read_Reg(AD7124_ERR_REG, 3));
    printf("MAP: CH0=0x%04lx CH1=0x%04lx CH2=0x%04lx\r\n",
           AD7124_Read_Reg(AD7124_CH0_MAP_REG, 2),
           AD7124_Read_Reg(AD7124_CH1_MAP_REG, 2),
           AD7124_Read_Reg(AD7124_CH2_MAP_REG, 2));
    printf("CFG: C0=0x%04lx C1=0x%04lx C2=0x%04lx F0=0x%06lx F1=0x%06lx F2=0x%06lx\r\n",
           AD7124_Read_Reg(AD7124_CFG0_REG, 2),
           AD7124_Read_Reg(AD7124_CFG1_REG, 2),
           AD7124_Read_Reg(AD7124_CFG2_REG, 2),
           AD7124_Read_Reg(AD7124_FILT0_REG, 3),
           AD7124_Read_Reg(AD7124_FILT1_REG, 3),
           AD7124_Read_Reg(AD7124_FILT2_REG, 3));
}
/*
 * 直接从 DATA 寄存器读取完整的 4 字节帧，不依赖 RDY 状态。
 * 本函数只用于诊断：即使芯片没有报告数据就绪，也读取当前数据寄存器，
 * 便于与单次转换后的 STATUS 一起判断转换内核是否曾经更新过数据。
 */
static void AD7124_Diagnostic_ForceReadData(uint32_t *data, uint8_t *data_status)
{
    AD7124_CS = 0;
    SPI_ReadWriteByte(&SPI2_Config, 0x42);
    *data = ((uint32_t)SPI_ReadWriteByte(&SPI2_Config, 0xFF) << 16) |
            ((uint32_t)SPI_ReadWriteByte(&SPI2_Config, 0xFF) << 8) |
            ((uint32_t)SPI_ReadWriteByte(&SPI2_Config, 0xFF));
    *data_status = SPI_ReadWriteByte(&SPI2_Config, 0xFF);
    delay_us(100);
    AD7124_CS = 1;
}

/*
 * 单次转换对照测试。
 *
 * 先让 ADC 进入空闲模式，再只保留 CH0 温度通道并启动一次单次转换。
 * 一秒后无论 RDY 是否变化都强制读取数据帧。若单次转换同样始终 RDY=1，
 * 则问题不在多通道切换或连续模式，而在 AD7124 转换时钟、芯片供电或硬件本身。
 */
static void AD7124_Diagnostic_SingleConversionTest(void)
{
    uint8_t status;
    uint8_t data_status;
    uint32_t data;
    uint32_t error;

    printf("\r\n[SINGLE CONVERSION TEST]\r\n");

    /* 先停止连续转换，确保随后通道和模式切换的状态可重复。 */
    AD7124_Write_Reg(AD7124_ADC_CTRL_REG, 2,
                     AD7124_ADC_CTRL_REG_DATA_STATUS |
                     AD7124_ADC_CTRL_REG_POWER_MODE(2) |
                     AD7124_ADC_CTRL_REG_REF_EN |
                     AD7124_ADC_CTRL_REG_MODE(4) |
                     AD7124_ADC_CTRL_REG_CLK_SEL(0));
    delay_ms(10);

    enble_ch0();
    AD7124_Diagnostic_PrintRegisters();

    /* MODE=1 表示单次转换，完成后芯片会自动进入空闲状态。 */
    AD7124_Write_Reg(AD7124_ADC_CTRL_REG, 2,
                     AD7124_ADC_CTRL_REG_DATA_STATUS |
                     AD7124_ADC_CTRL_REG_POWER_MODE(2) |
                     AD7124_ADC_CTRL_REG_REF_EN |
                     AD7124_ADC_CTRL_REG_MODE(1) |
                     AD7124_ADC_CTRL_REG_CLK_SEL(0));
    delay_ms(1000);

    status = (uint8_t)AD7124_Read_Reg(AD7124_STATUS_REG, 1);
    AD7124_Diagnostic_ForceReadData(&data, &data_status);
    error = AD7124_Read_Reg(AD7124_ERR_REG, 3);
    printf("SINGLE: ADC=0x%04lx ST=0x%02x DATA=0x%06lx DST=0x%02x ERR=0x%06lx\r\n",
           AD7124_Read_Reg(AD7124_ADC_CTRL_REG, 2),
           status,
           data,
           data_status,
           error);

    /* 恢复业务程序采用的连续转换模式，供后续三通道测试继续运行。 */
    AD7124_Write_Reg(AD7124_ADC_CTRL_REG, 2,
                     AD7124_ADC_CTRL_REG_DATA_STATUS |
                     AD7124_ADC_CTRL_REG_POWER_MODE(2) |
                     AD7124_ADC_CTRL_REG_REF_EN |
                     AD7124_ADC_CTRL_REG_MODE(0) |
                     AD7124_ADC_CTRL_REG_CLK_SEL(0));
    delay_ms(100);
}
/*
 * 片内温度传感器单次转换测试。
 *
 * 通道 3 直接选择 AD7124 内部温度传感器(AINP=16)与 AVSS(AINM=17)，
 * 使用片内 2.5 V 基准和独立的 Setup3。该测试不经过板外 PT100、压力
 * 传感器、外部参考电压及其连线，可将故障范围收敛到 AD7124 芯片本身、
 * 模拟/数字供电、去耦电容或 SPI 配置。
 */
static void AD7124_Diagnostic_InternalTemperatureTest(void)
{
    uint8_t power_mode;
    uint8_t status;
    uint8_t data_status;
    uint32_t data;
    uint32_t error;

    printf("\r\n[INTERNAL TEMPERATURE POWER SWEEP]\r\n");

    /* 先停止转换，再配置片内温度传感器通道和片内 2.5 V 基准。 */
    AD7124_Write_Reg(AD7124_ADC_CTRL_REG, 2,
                     AD7124_ADC_CTRL_REG_DATA_STATUS |
                     AD7124_ADC_CTRL_REG_POWER_MODE(0) |
                     AD7124_ADC_CTRL_REG_REF_EN |
                     AD7124_ADC_CTRL_REG_MODE(4) |
                     AD7124_ADC_CTRL_REG_CLK_SEL(0));
    delay_ms(10);

    AD7124_Write_Reg(AD7124_CH0_MAP_REG, 2, AD7124_CH_MAP_REG_CH_DISABLE);
    AD7124_Write_Reg(AD7124_CH1_MAP_REG, 2, AD7124_CH_MAP_REG_CH_DISABLE);
    AD7124_Write_Reg(AD7124_CH2_MAP_REG, 2, AD7124_CH_MAP_REG_CH_DISABLE);
    AD7124_Write_Reg(AD7124_CFG3_REG, 2,
                     AD7124_CFG_REG_BIPOLAR |
                     AD7124_CFG_REG_REF_SEL(2) |
                     AD7124_CFG_REG_PGA(0));
    AD7124_Write_Reg(AD7124_FILT3_REG, 3,
                     AD7124_FILT_REG_FILTER(4) |
                     AD7124_FILT_REG_REJ60 |
                     AD7124_FILT_REG_POST_FILTER(0) |
                     AD7124_FILT_REG_FS(20));
    AD7124_Write_Reg(AD7124_CH3_MAP_REG, 2,
                     AD7124_CH_MAP_REG_CH_ENABLE |
                     AD7124_CH_MAP_REG_SETUP(3) |
                     AD7124_CH_MAP_REG_AINP(16) |
                     AD7124_CH_MAP_REG_AINM(17));
    delay_ms(10);

    printf("INT: CH3=0x%04lx C3=0x%04lx F3=0x%06lx\r\n",
           AD7124_Read_Reg(AD7124_CH3_MAP_REG, 2),
           AD7124_Read_Reg(AD7124_CFG3_REG, 2),
           AD7124_Read_Reg(AD7124_FILT3_REG, 3));

    /*
     * 依次测试低功耗、中功耗、全功耗。
     * 单次转换结束后 ADC_CONTROL 的 MODE 应自动变为待机模式 2；
     * 如果仍保持 MODE=1 且 RDY=1，说明该次转换根本没有完成。
     */
    for (power_mode = 0U; power_mode <= 2U; power_mode++)
    {
        AD7124_Write_Reg(AD7124_ADC_CTRL_REG, 2,
                         AD7124_ADC_CTRL_REG_DATA_STATUS |
                         AD7124_ADC_CTRL_REG_POWER_MODE(power_mode) |
                         AD7124_ADC_CTRL_REG_REF_EN |
                         AD7124_ADC_CTRL_REG_MODE(1) |
                         AD7124_ADC_CTRL_REG_CLK_SEL(0));
        delay_ms(1500);

        status = (uint8_t)AD7124_Read_Reg(AD7124_STATUS_REG, 1);
        AD7124_Diagnostic_ForceReadData(&data, &data_status);
        error = AD7124_Read_Reg(AD7124_ERR_REG, 3);
        printf("INT PM=%u: ADC=0x%04lx ST=0x%02x DATA=0x%06lx DST=0x%02x ERR=0x%06lx\r\n",
               power_mode,
               AD7124_Read_Reg(AD7124_ADC_CTRL_REG, 2),
               status,
               data,
               data_status,
               error);
    }

    /* 恢复旧板三通道配置，保证后续诊断循环从原始条件开始。 */
    AD7124_Legacy_Init();
    delay_ms(100);
}
/*
 * 旧板 AD7124 独占诊断循环。
 *
 * 该函数由 main.c 中的 SVP_AD7124_STANDALONE_TEST 调用。测试期间不启动
 * TDC、文件系统、采集定时器和 FreeRTOS，只访问 SPI2 上的 AD7124。
 *
 * 每轮固定执行：通道 0 温度 -> 通道 2 压力参考 -> 通道 1 压力输出。
 * 每次通道切换后均等待 RDY，再打印完整 STATUS、原始 DATA、ERR 和换算值。
 */
void AD7124_Legacy_DiagnosticLoop(void)
{
    uint32_t cycle = 0U;
    uint32_t data;
    uint32_t error;
    uint16_t id;
    uint8_t status;
    uint8_t data_status;
    uint8_t ready;
    float pressure_reference = 0.0f;

    /* 使用旧板三通道配置，并强制从温度通道开始。 */
    AD7124_Legacy_Init();
    delay_ms(100);
    id = Get_AD7124_ID();

    printf("\r\n===== AD7124 STANDALONE TEST =====\r\n");
    printf("ID=0x%02x ADC_CTRL=0x%04lx ERR=0x%06lx\r\n",
           id,
           AD7124_Read_Reg(AD7124_ADC_CTRL_REG, 2),
           AD7124_Read_Reg(AD7124_ERR_REG, 3));
    printf("CH0=0x%04lx CH1=0x%04lx CH2=0x%04lx\r\n",
           AD7124_Read_Reg(AD7124_CH0_MAP_REG, 2),
           AD7124_Read_Reg(AD7124_CH1_MAP_REG, 2),
           AD7124_Read_Reg(AD7124_CH2_MAP_REG, 2));
    AD7124_Diagnostic_PrintRegisters();
    AD7124_Diagnostic_SingleConversionTest();
    AD7124_Diagnostic_InternalTemperatureTest();

    printf("COE: TEMP_A=%.6f TEMP_B=%.6f PA_I=%.6f PA_M=%.6f PA_E=%.6f PA_A=%.6f\r\n",
           svp_cmd.TEMP_COE_A,
           svp_cmd.TEMP_COE_B,
           svp_cmd.PA_COE_I,
           svp_cmd.PA_COE_M,
           svp_cmd.PA_COE_E,
           svp_cmd.PA_COE_A);

    while (1)
    {
        float resistance;
        float temperature;
        float pressure_voltage;
        float sensitivity;
        float raw_depth;

        cycle++;
        printf("\r\n[AD7124 TEST %lu]\r\n", cycle);

        /* 通道 0：PT100 温度。 */
        enble_ch0();
        AD7124_Diagnostic_PrintRegisters();
        ready = AD7124_Diagnostic_ReadChannel(&data, &status, &data_status, &error);
        if (ready != 0U)
        {
            resistance = (float)data * 3900.0f / (2.0f * 0x800000) / 32.0f;
            temperature = calculate_value(resistance) * svp_cmd.TEMP_COE_A +
                          svp_cmd.TEMP_COE_B;
            printf("CH0 TEMP: ST=0x%02x CH=%u DATA=0x%06lx DST=0x%02x ERR=0x%06lx R=%.3f TEMP=%.3f\r\n",
                   status,
                   (uint8_t)(status & 0x0FU),
                   data,
                   data_status,
                   error,
                   resistance,
                   temperature);
        }
        else
        {
            printf("CH0 TEMP: RDY TIMEOUT ST=0x%02x CH=%u ERR=0x%06lx\r\n",
                   status,
                   (uint8_t)(status & 0x0FU),
                   error);
        }

        /* 通道 2：压力传感器参考电压。 */
        enble_ch2();
        AD7124_Diagnostic_PrintRegisters();
        ready = AD7124_Diagnostic_ReadChannel(&data, &status, &data_status, &error);
        if (ready != 0U)
        {
            pressure_reference = ((float)data * 2.5f) /
                                 (2.0f * 0x800000 * AD7124_LEGACY_REFERENCE_GAIN);
            printf("CH2 REF : ST=0x%02x CH=%u DATA=0x%06lx DST=0x%02x ERR=0x%06lx REF=%.6fV\r\n",
                   status,
                   (uint8_t)(status & 0x0FU),
                   data,
                   data_status,
                   error,
                   pressure_reference);
        }
        else
        {
            pressure_reference = 0.0f;
            printf("CH2 REF : RDY TIMEOUT ST=0x%02x CH=%u ERR=0x%06lx\r\n",
                   status,
                   (uint8_t)(status & 0x0FU),
                   error);
        }

        /* 通道 1：压力传感器输出。 */
        enble_ch1();
        AD7124_Diagnostic_PrintRegisters();
        ready = AD7124_Diagnostic_ReadChannel(&data, &status, &data_status, &error);
        if (ready != 0U)
        {
            pressure_voltage = (pressure_reference / AD7124_LEGACY_PRESSURE_GAIN) *
                               ((float)data / 0x800000 - 1.0f);

            if ((svp_cmd.PA_COE_I == 0.0f) || (svp_cmd.PA_COE_M == 0.0f))
            {
                printf("CH1 PRES: ST=0x%02x CH=%u DATA=0x%06lx DST=0x%02x ERR=0x%06lx V=%.6fV CAL=INVALID\r\n",
                       status,
                       (uint8_t)(status & 0x0FU),
                       data,
                       data_status,
                       error,
                       pressure_voltage);
            }
            else
            {
                sensitivity = ((svp_cmd.PA_COE_I / 3.0f) /
                              svp_cmd.PA_COE_M) / 10.0f;
                raw_depth = (pressure_voltage * 1000.0f / sensitivity) * 10.0f;
                printf("CH1 PRES: ST=0x%02x CH=%u DATA=0x%06lx DST=0x%02x ERR=0x%06lx V=%.6fV RAW_DEPTH=%.3f\r\n",
                       status,
                       (uint8_t)(status & 0x0FU),
                       data,
                       data_status,
                       error,
                       pressure_voltage,
                       raw_depth);
            }
        }
        else
        {
            printf("CH1 PRES: RDY TIMEOUT ST=0x%02x CH=%u ERR=0x%06lx\r\n",
                   status,
                   (uint8_t)(status & 0x0FU),
                   error);
        }

        /* 500 ms 一轮，串口助手可完整接收，不会和正常业务输出混在一起。 */
        delay_ms(500);
    }
}
void AD7124_DATA(void){
	uint32_t data_reg = AD7124_READ_DATAREG();
	/* DATA_STATUS 已在 AD7124_READ_DATAREG() 中与本次数据同步取得。 */
	switch(DATA_STATUS){
		case 0:{
			/*获得电阻值*/
			float Rdata = (float)data_reg * 3900 / (2*0x800000) / 32;
			PT100_TEMP = calculate_value(Rdata) * svp_cmd.TEMP_COE_A + svp_cmd.TEMP_COE_B;
            if(PT100_TEMP > 100.0f || PT100_TEMP < 0.0f)
                PT100_TEMP = 0.0f;
			break;
		}
		default:
			break;
	}
}

void AD7124_Init(void){
    AD7124_PT100_Init();
    //防止系数为0
    if(svp_cmd.TEMP_COE_A == 0)
        svp_cmd.TEMP_COE_A = 1;
}  


