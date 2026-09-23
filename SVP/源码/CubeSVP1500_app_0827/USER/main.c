#include "main.h"
#include "ffconf.h"
#include "file_fsm.h"
#include "freertos_app.h"
#include "task.h"

/*
 * 设备供电线与串口线共用。串口初始化完成后先等待一段时间，再输出启动日志，
 * 让上位机 USB 转串口和终端软件有足够时间完成枚举与打开。
 */
#define W25QXX_FATFS

/*
 * AD7124 独占诊断模式。
 *
 * 置为 1 后，程序完成最小硬件和 EEPROM 参数初始化，随后只执行
 * AD7124 温度、压力参考和压力通道的单独循环测试，不再进入 FatFs、
 * TDC、正常采集定时器或 FreeRTOS。
 *
 * 该模式用于定位底层 AD7124/SPI/模拟输入问题。完成上板测试后必须
 * 恢复为 0，才能回到正常声速剖面仪程序。
 */
#define SVP_AD7124_STANDALONE_TEST    0U

/*
 * 启动日志级别：
 *   0：关闭普通启动日志；
 *   1：只输出关键启动节点和故障信息；
 *   2：输出完整启动日志，便于调试硬件和软件初始化。
 *
 * 这里只控制启动阶段专用的打印，不影响采集数据、显控协议和其他运行阶段的 printf。
 * 后续只需要修改这一个宏即可切换启动日志输出级别。
 */
#define SVP_STARTUP_LOG_LEVEL    0U

#if (SVP_STARTUP_LOG_LEVEL == 0U)
    /* 关闭普通启动日志。 */
    #define SVP_STARTUP_PRINTF(...)        do { } while (0)
    #define SVP_STARTUP_DETAIL_PRINTF(...) do { } while (0)
#elif (SVP_STARTUP_LOG_LEVEL == 1U)
    /* 只保留关键启动节点；硬件逐项成功信息属于详细日志。 */
    #define SVP_STARTUP_PRINTF(...)        printf(__VA_ARGS__)
    #define SVP_STARTUP_DETAIL_PRINTF(...) do { } while (0)
#elif (SVP_STARTUP_LOG_LEVEL == 2U)
    /* 调试模式下输出完整启动日志。 */
    #define SVP_STARTUP_PRINTF(...)        printf(__VA_ARGS__)
    #define SVP_STARTUP_DETAIL_PRINTF(...) printf(__VA_ARGS__)
#else
    #error "SVP_STARTUP_LOG_LEVEL 只能设置为 0、1 或 2"
#endif
#define STARTUP_SERIAL_WAIT_MS    4000U
#define STARTUP_LED_PERIOD_MS     500U				//上电初始化开机延时

/************************************************ TDC ********************************************************************/
volatile uint8_t 		INTSign;							//外部中断触发标志位
volatile uint8_t  		num;                  				//TDC外部中断次数
CircularBuffer 			circular_buffer;             		//声明平均声速环形缓冲区
tdc_pse_t 				tdc_pse;                          	//TDC伪值处理结构体
/**********************************************  FATFS *******************************************************************/
Pri_File_T 				pri_file ;                       	//文件结构体
uint8_t 				buf_num;                            //缓冲区编号
/************************************************ CMD ********************************************************************/
svp_cmd_t 				svp_cmd;                          	//系统参数
/************************************************ ADC ********************************************************************/
float 					Depth=0.0f;                 		//深度
volatile float 			PT100_TEMP = 0.0f;          		//温度
PressureFilter 			PressureFilter_t;					//压力平均窗口结构体
PressureRecorder 		recorder;                   		//自容模式压力阈值结构体
/*************************************************************************************************************************/

/*
* @brief  检测是否进行了更新程序
* @param  None 
* @retval None
* flag： 0xAA - 等待更新    0x55 - 更新完成     0x00 - 正常模式 
*/
void iap_app_flag(void){
    //修改更新标志位
    uint8_t updata_buf;
    AT24C02_Read(IAP_UPDATA_ADDR,&updata_buf,1);
    if(updata_buf == 0x55){ //bootloader已经完成了程序更新，并修改了标志位
		delay_ms(500);
		send_response(0x30,NULL,0);
		delay_ms(500);
    }
    /*将标志位置为正常状态*/
    updata_buf = 0x00;
    AT24C02_Write(IAP_UPDATA_ADDR,&updata_buf,1);
}


/*
	系统配置初始化
		时钟
		延时函数
		串口初始化
	全局配置参数初始化
	硬件配置初始化
		IIC
			eeprom  rtc tmp117 fst800
		SPI
			gp22 ad7124 w25q01
	软件配置初始化
		fatfs
		iap
*/

/**
* @brief  系统初始化
* @param  None
*/
void sys_init(void){
	/*初始化HAL库*/
    HAL_Init();                    	   
	/*初始化系统时钟*/
    Stm32_Clock_Init(96,4,2,4);
    /*delay_init*/
    delay_init(96);            
	/*初始化串口*/
	uart_init(115200); 
	/*
	 * 启动诊断：串口初始化完成后立即输出一条 ASCII 信息。
	 * 如果这条信息也看不到，说明程序没有运行到串口初始化完成，
	 * 或者实际烧录的不是当前工程生成的程序。
	 */
	SVP_STARTUP_PRINTF("BOOT: UART initialized\r\n");
	/* 采集定时器初始化 */
    TIM2_Init(200,CLOCK_PSC);          
	HAL_TIM_Base_Stop_IT(&TIM_Config_2);
}

/**
 * @brief  上电等待
 * @param  None
 */
static void startup_wait_for_serial(void)
{
    uint32_t elapsed_ms = 0;
    LED_Init();

    while (elapsed_ms < STARTUP_SERIAL_WAIT_MS) {
        LED1 = !LED1;
        LED2 = 0;
        delay_ms(STARTUP_LED_PERIOD_MS);
        elapsed_ms += STARTUP_LED_PERIOD_MS;
    }
}

/**
 * @brief  硬件初始化
 * @param  None
 */
void hardware_init(void){
	/*
	 * LED 引脚已在 startup_wait_for_serial() 中初始化。正常运行后的 500 ms
	 * 状态灯、RTC 和电池服务由 FreeRTOS 状态任务接管，不再启动 TIM3 中断。
	 */
	SVP_STARTUP_DETAIL_PRINTF("    LED Init Success \r\n");
	/*IIC - EEPROM RTC*/
    IIC_Init();
	SVP_STARTUP_DETAIL_PRINTF("    EEPROM OR RTC Init Success \r\n");
	FST800_Init();
	SVP_STARTUP_DETAIL_PRINTF("    FST800 Init Success \r\n");
	/*电池*/
	ltc2943_init(); 
	SVP_STARTUP_DETAIL_PRINTF("    Battery Init Success \r\n");
	/*外部中断初始化*/
    EXTI_Init();
	SVP_STARTUP_DETAIL_PRINTF("    EXTI_Init Init Success \r\n");
	 /*SPI初始化*/
    SPI1_Init();
	SVP_STARTUP_DETAIL_PRINTF("    SPI1(TDC) Init Success \r\n");
	SPI2_Init();
	SVP_STARTUP_DETAIL_PRINTF("    SPI2(ADC) Init Success \r\n");
	SPI3_Init();
	SVP_STARTUP_DETAIL_PRINTF("    SPI3(FLASH_SPI) Init Success \r\n");
}

/**
 * @brief  软件初始化
 * @param  None
 */
void software_init(void){
	/*iap*/
	iap_app_flag();
	SVP_STARTUP_DETAIL_PRINTF("    IAP Init Success\r\n");
	
	/*QSPIFLASH-FATFS*/
	FLASH_Fat_Init();
	//
	SVP_STARTUP_DETAIL_PRINTF("    Fatfs Init Success \r\n");
    /*
     * 根据硬件上实际存在的压力传感器自动选择采集链路：
     * - 新板：IIC 数字压力传感器，AD7124 仅负责 PT100 温度；
     * - 旧板：AD7124 同时负责压力和温度。
     *
     * 识别失败时不得进入正常工作和文件记录，避免设备在压力数据无效时
     * 输出看似正常但实际错误的剖面数据。
     */
	 
    if (SVP_Sensor_Init() == 0U) {
        while (1) {
			printf("    Sensor Init Failed, system halted\r\n");
            /*
             * 故障状态下仅让红灯闪烁，提示压力采集硬件未识别。
             * 此时没有启动采集定时器、TDC 和 FreeRTOS 任务。
             */
            LED2 = 1;
			LED1 = !LED2;
            delay_ms(250);
        }
    }
	/*TDC*/
	TDC_GP22_Init();
	SVP_STARTUP_DETAIL_PRINTF("    TDC Init Success CAL:%f\r\n",bytes_Cal);
	/*调整工作模式 (根据工作模式决定启用定时器4/5初始化并使能)*/
	timework_init();
	/*打开采集定时器*/
	HAL_TIM_Base_Start_IT(&TIM_Config_2);
}

/**
 * @brief  参数配置初始化
 * @param  None
 */
void config_init(void){
	/*系统参数*/
	SVP_CMD_INIT(&svp_cmd);
	/*rtc参数配置*/
    if(svp_cmd.RTC_FLAG == 0){
		SVP_STARTUP_DETAIL_PRINTF("	set etc\r\n");
        rtc_date_time();
    }
}


int main(void)
{   
    sys_init();
		SVP_STARTUP_PRINTF("BOOT: before startup wait\r\n");
    startup_wait_for_serial();
		SVP_STARTUP_PRINTF("BOOT: after startup wait\r\n");
		SVP_STARTUP_PRINTF("\r\nSVP_Hardware_Init:\r\n");
	hardware_init();
		SVP_STARTUP_PRINTF("\r\nSVP_Config_Init:\r\n");
	config_init();

#if (SVP_AD7124_STANDALONE_TEST == 1U)
    /*
     * 独占诊断必须在 software_init() 之前运行：
     * - 避免正常传感器适配层、TDC 和 TIM2 同时访问硬件；
     * - 不挂载或写入文件系统；
     * - config_init() 已完成 EEPROM 参数读取，可同时检查压力和温度标定参数。
     *
     * AD7124_Legacy_DiagnosticLoop() 内部不会返回。
     */
    AD7124_Legacy_DiagnosticLoop();
#endif

		SVP_STARTUP_PRINTF("\r\nSVP_Software_Init:\r\n");
	software_init();
		SVP_STARTUP_PRINTF("\r\n");
		SVP_STARTUP_PRINTF("%s _ Device Start ok\r\n",FW_APP);
	
	
    /*
     * 所有原有初始化均完成后才创建任务。
     */
    if (freertos_app_create() != pdPASS) {
        printf("FreeRTOS AppTask create failed\r\n");
        while (1) {
        }
    }

    printf("FreeRTOS tasks created\r\n");
    vTaskStartScheduler();

    /* 调度器正常启动后不会返回；若返回，通常表示空闲任务创建失败。 */
    printf("FreeRTOS scheduler failed\r\n");
    while (1) {
    }
}



