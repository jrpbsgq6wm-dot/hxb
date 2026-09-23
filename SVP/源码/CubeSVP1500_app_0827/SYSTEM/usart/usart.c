#include "usart.h"
#include "delay.h"
#include "fat.h"
#include "power_app.h"
#include "file_fsm.h"
#include "freertos_app.h"


//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
//#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)	
#if 1
#pragma import(__use_no_semihosting)             
    //标准库需要的支持函数                 
    struct __FILE 
    { 
        int handle; 
    }; 

    FILE __stdout;       
    //定义_sys_exit()以避免使用半主机模式    
    void _sys_exit(int x) 
    { 
        x = x; 
    } 
    //重定义fputc函数 
    int fputc(int ch, FILE *f)
    { 	
        while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
        USART1->DR = (u8) ch;      
        return ch;
    }
#endif 

#if EN_USART1_RX   //如果使能了接收
/***************************************************************** */

uint8_t rx_buffer[RX_BUFFER_SIZE];   //接收缓冲区
volatile uint16_t rx_length;         //接收数据长度
volatile uint8_t frame_receied;      //接收完成标志位
UART_HandleTypeDef UART1_Handler;    //串口1句柄
/*DMA*/
uint8_t dma_buffer[RX_BUFFER_SIZE];  //接收缓冲区
/*
 * DMA 空闲中断得到的本帧接收字节数。
 * RX_BUFFER_SIZE 为 1024，必须使用 16 位变量，避免 256 字节以上的帧长度被截断。
 */
volatile uint16_t dma_rx_len;
/*CRC*/
uint16_t CRC_16;
uint8_t CRC_LOW;
uint8_t CRC_HIG;
volatile uint8_t pri_enable_flag = 0;
/***************************************************************** */

//初始化IO 串口1 
//bound:波特率
void uart_init(u32 bound)
{
	//UART 初始化设置
	UART1_Handler.Instance=USART1;					    //USART1
	UART1_Handler.Init.BaudRate=bound;				    //波特率
	UART1_Handler.Init.WordLength=UART_WORDLENGTH_8B;   //字长为8位数据格式
	UART1_Handler.Init.StopBits=UART_STOPBITS_1;	    //一个停止位
	UART1_Handler.Init.Parity=UART_PARITY_NONE;		    //无奇偶校验位
	UART1_Handler.Init.HwFlowCtl=UART_HWCONTROL_NONE;   //无硬件流控
	UART1_Handler.Init.Mode=UART_MODE_TX_RX;		    //收发模式
	HAL_UART_Init(&UART1_Handler);					    //HAL_UART_Init()会使能UART1
    //dma初始化
    USART1_DMA_Init();
    //使能usart1空闲中断
    __HAL_UART_ENABLE_IT(&UART1_Handler,UART_IT_IDLE);
    //启动DMA
    HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
}




//UART底层初始化，时钟使能，引脚配置，中断配置
//此函数会被HAL_UART_Init()调用
//huart:串口句柄
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    //GPIO端口设置
	GPIO_InitTypeDef GPIO_Initure;
	
	if(huart->Instance==USART1)//如果是串口1，进行串口1 MSP初始化
	{
		__HAL_RCC_GPIOA_CLK_ENABLE();			//使能GPIOA时钟
		__HAL_RCC_USART1_CLK_ENABLE();			//使能USART1时钟
	
		GPIO_Initure.Pin=GPIO_PIN_9;			//PA9
		GPIO_Initure.Mode=GPIO_MODE_AF_PP;		//复用推挽输出
		GPIO_Initure.Pull=GPIO_PULLUP;			//上拉
		GPIO_Initure.Speed=GPIO_SPEED_FAST;		//高速
		GPIO_Initure.Alternate=GPIO_AF7_USART1;	//复用为USART1
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);	   	//初始化PA9

		GPIO_Initure.Pin=GPIO_PIN_10;			//PA10
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);	   	//初始化PA10

		HAL_NVIC_SetPriority(USART1_IRQn,0,0);	//
        HAL_NVIC_EnableIRQ(USART1_IRQn);		//使能USART1中断通道
	}
}


#endif

void USART1_IRQHandler(void){
    if(__HAL_UART_GET_FLAG(&UART1_Handler,UART_FLAG_IDLE) != RESET){
       
        
        //停止DMA传输
        HAL_UART_DMAStop(&UART1_Handler);
        
        /*
         * DMA 的剩余传输计数由硬件提供。先以 16 位长度保存，
         * 否则接收长度超过 255 字节时会发生截断，导致后续协议解析错误。
         */
        dma_rx_len = (uint16_t)(RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&USART1TxDMA_Handler));

        /*
         * 只有长度落在接收缓冲区范围内才允许复制到命令缓冲区。
         * 异常时立即恢复 DMA 接收，不能让串口因丢弃异常帧而停止。
         */
        if((dma_rx_len > 0U) && (dma_rx_len <= RX_BUFFER_SIZE)){
            memcpy(rx_buffer,dma_buffer,dma_rx_len);
            rx_length = dma_rx_len;
            frame_receied  = 1;
        }else{
            dma_rx_len = 0;
            rx_length = 0;
            frame_receied = 0;
            HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
        }
        
        __HAL_UART_CLEAR_IDLEFLAG(&UART1_Handler);
//        //重新开启传输
//        HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
    }
}

HAL_StatusTypeDef Send_data(uint8_t *pdata,uint16_t size){
    return HAL_UART_Transmit(&UART1_Handler,pdata,size,HAL_MAX_DELAY);
}


void stm32_reset(void){
    HAL_NVIC_SystemReset();
}



/*指令配置结构体*/
uasrt_cmd tx_cmd[] = {
    /*指令码                       AT24C02存储地址        CRC校验长度      响应帧长度           内容字节长      R/W   函数指针*/
    {READ_PARAM,                SVP_BOUND_ADDR,                 2+4+1,      2+1+4+2+1,          4/*uint32_t*/,  1,      NULL},
    {READ_PORT,                 SVP_TYPE_ADDR,                  2+2+1,      2+1+2+2+1,          2/*uint16_t*/,  1,      NULL},
    {READ_FACTORY_DATA,         SVP_FACTORY_DATA_ADDR,          4+2+1,      2+1+4+2+1,          4/*uint32_t*/,  1,      NULL},
    {READ_SN,                   SVP_SN_ADDR,                    16+2+1,     2+1+16+2+1,         16/*char*/,     1,      NULL},
    {READ_BLOCK_DIS,            SVP_PROBE_DISTANCE_ADDR,        4+2+1,      2+1+4+2+1,          4/*float*/,     1,      NULL},
    {READ_LEAVE_VOLTAGE,        SVP_OUTLIERS_THRESHOLD_ADDR,    4+2+1,      2+1+4+2+1,          4/*float*/,     1,      NULL},
    {READ_KALMAN_BUF_SIZE,      SVP_KLAMAN_OBSERVATIONS_ADDR,   2+2+1,      2+1+2+2+1,          2/*uint16_t*/,  1,      NULL},
    {READ_PRI_BOUND,            SVP_FREQ_ADDR,                  4+2+1,      2+1+4+2+1,          4/*uint32_t*/,  1,      NULL},
    {READ_TDC_COE,              SOUND_VELOCITY_COE_A1_ADDR,     24+2+1,     2+1+24+2+1,         4*6/*float*/,   1,      NULL},
    {READ_USE_COE_SCOPE,        COE_USE_A2_B2,                  8+2+1,      2+1+8+2+1,          4*2/*float*/,   1,      NULL},
    {READ_FRIST_WAVE_VOLTAGE,   FRIST_WAVE_VOLTAGE,             2+4+1,      2+1+4+2+1,          4/*uint32_t*/,  1,      NULL},
    //10                
    {WRITE_PARAM,               SVP_BOUND_ADDR,                 2+4+1,      2+1+4+2+1,          4,              0,      NULL},
    {WRITE_PORT,                SVP_TYPE_ADDR,                  2+2+1,      2+1+2+2+1,          2,              0,      NULL},
    {WRITE_FACTORY_DATA,        SVP_FACTORY_DATA_ADDR,          4+2+1,      2+1+4+2+1,          4,              0,      NULL},  
    {WRITE_SN,                  SVP_SN_ADDR,                    16+2+1,     2+1+16+2+1,         16,             0,      NULL},  
    {WRITE_BLOCK_DIS,           SVP_PROBE_DISTANCE_ADDR,        4+2+1,      2+1+4+2+1,          4,              0,      gp22_parameter_set},
    {WRITE_LEAVE_VOLTAGE,       SVP_OUTLIERS_THRESHOLD_ADDR,    4+2+1,      2+1+4+2+1,          4,              0,      gp22_parameter_set},
    {WRITE_KALMAN_BUF_SIZE,     SVP_KLAMAN_OBSERVATIONS_ADDR,   2+2+1,      2+1+2+2+1,          2,              0,      NULL},
    {WRITE_PRI_BOUND,           SVP_FREQ_ADDR,                  4+2+1,      2+1+4+2+1,          4,              0,      NULL}, 
    {WRITE_TDC_COE,             SOUND_VELOCITY_COE_A1_ADDR,     24+2+1,     2+1+24+2+1,         4*6,            0,      gp22_parameter_set},
    {WRITE_USE_COE_SCOPE,       COE_USE_A2_B2,                  8+2+1,      2+1+8+2+1,          4*2,            0,      gp22_parameter_set},
    {WRITE_FRIST_WAVE_VOLTAGE,  FRIST_WAVE_VOLTAGE,             4+2+1,      2+1+4+2+1,          4,              0,      stm32_reset},
    {WRITE_RTC_DATE_TIME,       WRITE_RTC_ADDR,                 8+2+1,      2+1+8+2+1,          8,/*uint64_t*/  0,      rtc_date_time},
    //22                
    {READ_ALL_FILE_NAME,        FILE_STATUS_ADDR,               0,       	0,          		0,/*uint32_t*/  3,      NULL},
    {DOWNLOAD_FILE,             FILE_STATUS_ADDR,               0,          0,                  0,              3,      NULL},
    {DELETE_FILE,               FILE_STATUS_ADDR,               0,          0,                  0,              3,      NULL},
    {MKFS_DISK,                 FILE_STATUS_ADDR,               0,          0,                  0,              3,      NULL},
    //26
    {SET_MODE,                  WORK_MODE_ADDR,                 1+1+4+2+1,  2+1+1+1+4+2+1,      6,              4,      timework_init},  //设置自容模式还是串口输出模式
    {READ_RTC_DATA_TIME,        0,                              8+2+1,      2+1+8+2+1,          8,/*uint64_t*/  5,      NULL},
    {READ_MODE,                 WORK_MODE_ADDR,                 1+1+4+2+1,  2+1+1+1+4+2+1,      6,              1,      NULL},
    {WRITE_PA_COE,              PA_COE_ADDR,                    16+2+1,     2+1+16+2+1,         4*4,            0,      NULL},
    {READ_PA_COE,               PA_COE_ADDR,                    16+2+1,     2+1+16+2+1,         4*4,            1,      NULL},
    {ENABLE_PRI,                0,                              0+2+1,      2+1+0+2+1,          0,              6,      NULL},
    {DISABLE_PRI,               0,                              0+2+1,      2+1+0+2+1,          0,              6,      NULL},
    {RESET_CMD,                 0,                              0+2+1,      2+1+0+2+1,          0,              7,      NULL},
    {WRITE_TEMP_COE,            TEMPERATURE_COE_ADDR,           16+2+1,     2+1+16+2+1,         4*4,            0,      NULL},
    {READ_TEMP_COE,             TEMPERATURE_COE_ADDR,           16+2+1,     2+1+16+2+1,         4*4,            1,      NULL},
    {WRITE_SV_SN,               SV_SN_ADDR,                     8+2+1,      2+1+8+2+1,          8,              0,      NULL},
    {READ_SV_SN,                SV_SN_ADDR,                     8+2+1,      2+1+8+2+1,          8,              1,      NULL},
    {WRITE_TEMP_SN,             TEMP_SN_ADDR,                   8+2+1,      2+1+8+2+1,          8,              0,      NULL},
    {READ_TEMP_SN,              TEMP_SN_ADDR,                   8+2+1,      2+1+8+2+1,          8,              1,      NULL},
    {WRITE_PRESSURE_SN,         PRESSURE_SN_ADDR,               8+2+1,      2+1+8+2+1,          8,              0,      NULL},
    {READ_PRESSURE_SN,          PRESSURE_SN_ADDR,               8+2+1,      2+1+8+2+1,          8,              1,      NULL},
    {READ_CURRENT_POWER,        0,                              4+2+1,      2+1+4+2+1,          4,              8,      NULL},
    {BATTREY_CALIBRATION,       0,                              0+2+1,      2+1+0+2+1,          0,              8,      NULL},
    {SET_DEPTH_EC,              DEPTH_EC,                       8+2+1,      2+1+8+2+1,          8,              0,      NULL}, 
	{READ_DEPTH_EC,             DEPTH_EC,                       8+2+1,      2+1+8+2+1,          8,              1,      NULL},   
	//46	
	{LINK_STATE,				0,								0+2+1,		2+1+0+2+1,			0,				9,		NULL},
	{UPDATA_SYS,				0,								0 	 ,		0		 ,			0,				0,		NULL},
	{BATCH_DELETE,				0,								0 	 ,		0		 ,			0,				3,		NULL},
	{UNLINK_STATE,				0,								0+2+1,		2+1+0+2+1,			0,				9,		NULL},
};      

#define TX_CMD_COUNT    (sizeof(tx_cmd) / sizeof(tx_cmd[0]))

/*
* @brief 判断指令是否需要关闭当前正在记录的文件
* @param  cmd: 指令码
*/
static uint8_t cmd_needs_record_close(uint8_t cmd)
{
    if(cmd >= TX_CMD_COUNT){
        return 0;
    }

    switch(cmd){
        case DOWNLOAD_FILE:
        case DELETE_FILE:
        case MKFS_DISK:
        case SET_MODE:
        case RESET_CMD:
        case UPDATA_SYS:
        case BATCH_DELETE:
            return 1;
        default:
            break;
    }

    /*
     * 普通 EEPROM 写命令不再默认截断当前剖面记录。自容记录进行时，
     * 这类命令会在 cmd_ProcessData() 中被直接忽略且不返回响应。
     */
    return 0;
}

void send_response(uint8_t cmd, uint8_t *data, uint8_t data_len) {
    int idx = 0;
	int cmd_len = data_len;
    data_len += 3;
	uint8_t send_buffer[1024] = {0};
    send_buffer[idx++] = FRAME_HEAD_RSP;        
    send_buffer[idx++] = cmd;                   
    send_buffer[idx++] = data_len;
    
    if (data && data_len > 0) {
        memcpy(send_buffer + idx, data, cmd_len);
        idx += cmd_len;
    }
    // CRC16
    CRC_16 = CRC16(send_buffer,data_len);
    send_buffer[idx++] = CRC_16&0X00FF;
    send_buffer[idx++] = (CRC_16>>8);
    
    send_buffer[idx++] = FRAME_TAIL_RSP;      
    /*判断当前工作模式是否为直读模式，因为直读模式下会有数据发送，需要先停止发送数据避免与指令进行混淆*/
	if(work_status() == 1){
		pri_enable_flag = 1;
	}
	
    if(Send_data(send_buffer,idx) != HAL_OK)
            printf("%s ERROR\r\n",__func__);
	
    /*恢复直读模式下数据发送*/
	if(work_status() == 1){
		pri_enable_flag = 0;
	}
	
    memset(rx_buffer,0,sizeof(rx_buffer));
    HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
}

/*
    读取命令的响应帧构成
*/
void r_cmd_composition(uasrt_cmd *cmd_buf){
    if(validate_download_request(rx_buffer,rx_length) == -1){
        printf("CRC ERROR\r\n");
        return ;
    }
    uint8_t *value = mymalloc(cmd_buf->data_Bsize);    //分配指定长度的数据
    /*获取数据*/
    AT24C02_Read(cmd_buf->addr,value,cmd_buf->data_Bsize);
    send_response(cmd_buf->cmd_code,value,cmd_buf->data_Bsize);
    myfree(value);
}


/*
    写入命令的响应帧构成
*/
void w_cmd_composition(uasrt_cmd *cmd_buf){
    if(validate_download_request(rx_buffer,rx_length) == -1){
        printf("CRC ERROR\r\n");
        return ;
    }
    uint8_t *value = mymalloc(cmd_buf->data_Bsize);    //分配指定长度的数据
    if(value == NULL)
        printf("%s MALLOC ERROR",__func__);
    
    /*获取需要写入的数据*/
    memcpy(value,&rx_buffer[CMD_DATA_INDEX],cmd_buf->data_Bsize);
    AT24C02_Write(cmd_buf->addr,value,cmd_buf->data_Bsize);
    
    /*释放空间*/
    myfree(value);
    
    //重新系数参数等
    SVP_CMD_INIT(&svp_cmd);
    
	/*构造响应帧*/
    r_cmd_composition(cmd_buf);
	
    /*赋值 - 调用准备好的赋值函数*/
    if(cmd_buf->func != NULL){
        cmd_buf->func();
    }
    
}


/*设置模式*/
void set_mode_func(uasrt_cmd *cmd_buf){
    if(validate_download_request(rx_buffer,rx_length) == -1){
        printf("CRC ERROR\r\n");
        return ;
    }
    
    uint8_t *value = mymalloc(cmd_buf->data_Bsize);    //分配指定长度的数据
    if(value == NULL)
        printf("%s MALLOC ERROR",__func__);
    
    /*获取需要写入的数据*/
    memcpy(value,&rx_buffer[CMD_DATA_INDEX],cmd_buf->data_Bsize);
    AT24C02_Write(cmd_buf->addr,value,cmd_buf->data_Bsize);
    
    /*释放空间*/
    myfree(value);
	
    //重新系数参数等
    SVP_CMD_INIT(&svp_cmd);
    
	/*构造响应帧*/
    r_cmd_composition(cmd_buf);
	
    /*赋值 - 调用准备好的赋值函数*/
    if(cmd_buf->func != NULL){
        //my_scan_file(DISK_PATH);
        cmd_buf->func();
    }
		
    
}

//设置CRC时间
void set_rtc_data_time(uasrt_cmd *cmd_buf){
    if(validate_download_request(rx_buffer,rx_length) == -1){
        printf("CRC ERROR\r\n");
        return ;
    }
	uint8_t databuffer[cmd_buf->data_Bsize];
	uint8_t idx = 0;
	databuffer[idx++] = DS3231_Time.AM_PM;
    databuffer[idx++] = DS3231_Time.sec;
    databuffer[idx++] = DS3231_Time.min;
    databuffer[idx++] = DS3231_Time.hour;
    databuffer[idx++] = DS3231_Time.day;
    databuffer[idx++] = DS3231_Time.date;
    databuffer[idx++] = DS3231_Time.mon;
    databuffer[idx++] = DS3231_Time.year;
	send_response(cmd_buf->cmd_code,databuffer,cmd_buf->data_Bsize);
}

/*  程序升级
 */
void program_upgrade(void){
    /*
     * 显控发来 AA 30 后，APP 通过 EEPROM 标志请求 BOOT 进入升级模式：
     * - 0xAA：BOOT 应留在升级接收流程，不跳转旧 APP；
     * - 0x55：BOOT 已完成升级，APP 启动后会回复升级完成；
     * - 0x00：正常状态。
     */
    uint8_t buf = 0xAA;
    uint8_t verify_buf = 0x00;

    if(AT24C02_Write(IAP_UPDATA_ADDR,&buf,1) != HAL_OK){
        printf("IAP flag write failed\r\n");
        return;
    }

    if((AT24C02_Read(IAP_UPDATA_ADDR,&verify_buf,1) != HAL_OK) ||
       (verify_buf != buf)){
        printf("IAP flag verify failed\r\n");
        return;
    }
	freertos_i2c1_unlock();
    /*
     * 标志确认成功后立即复位。BOOT 上电读取到 0xAA 后会保持在 YMODEM
     * 接收状态，并向显控发送字符 'C' 请求传输固件。
     */
    NVIC_SystemReset();
}


/* 
    开始取消输出
 */
void pri_mode_func(uasrt_cmd *cmd_buf){
	if(validate_download_request(rx_buffer,rx_length) == -1){
        printf("CRC ERROR\r\n");
        return ;
    }
    if(cmd_buf->cmd_code == ENABLE_PRI && svp_cmd.WORK_MODE_FLAG != 0x00){
        //开始输出 0x20
        HAL_TIM_Base_Start_IT(&TIM_Config_4);
    }else if(cmd_buf->cmd_code == DISABLE_PRI){
        //停止输出 0x21
        HAL_TIM_Base_Stop_IT(&TIM_Config_4);
    }
	send_response(cmd_buf->cmd_code,NULL,cmd_buf->data_Bsize);
}




void reset_func(uasrt_cmd *cmd_buf){
	if(validate_download_request(rx_buffer,rx_length) == -1){
        printf("CRC ERROR\r\n");
        return ;
    }
        /*
            出厂日期        2025.4.24
            挡板距离        5.0
            入水阈值        50
            系数              1.0 0.0 1.0 0.0 1.0 0.0
            系数应用范围      1300 - 1500
            第一波电压       15
            RTC时间           2024.4.24
            工作模式        频率自容
        */
	//清除当前eeprom
    AT24C02_EraseALL(); 
	
    svp_cmd_t reset_set;
	
    reset_set.BOUND = 115200;
    reset_set.TYPE = 0;
    reset_set.DATE.year = 2026;
    reset_set.DATE.mon = 5;
    reset_set.DATE.date = 21;
	memset(reset_set.SN,0,sizeof(reset_set.SN));
    reset_set.PROBE_DISTANCE = 4.0f;
    reset_set.OUTLIERS_THRESHOLD = 50;
    reset_set.KLAMAN_OBSERVATIONS = 0;
    reset_set.FREQ_BOUND = 0;
    reset_set.SOUND_VELOCITY_COE_A1 = 1.0f;
    reset_set.SOUND_VELOCITY_COE_B1 = 1.0f;
    reset_set.SOUND_VELOCITY_COE_A2 = 0.0f;
    reset_set.SOUND_VELOCITY_COE_B2 = 0.0f;
    reset_set.SOUND_VELOCITY_COE_A3 = 0.0f;
    reset_set.SOUND_VELOCITY_COE_B3 = 0.0f;
    reset_set.COE_2_SPOCE = 1300.00f;
    reset_set.COE_3_SPOCE = 1400.00f;
    reset_set.FRIST_WAVE_V = 15;
    reset_set.RTC_DATE_TIME.year = 0;
    reset_set.RTC_DATE_TIME.mon = 0;
    reset_set.RTC_DATE_TIME.date = 0;
    reset_set.RTC_DATE_TIME.day = 0;
    reset_set.RTC_DATE_TIME.hour = 0;
    reset_set.RTC_DATE_TIME.min = 0;
    reset_set.RTC_DATE_TIME.sec = 0;
    reset_set.RTC_DATE_TIME.ampm = 0;
    reset_set.RTC_FLAG = 0; //RTC开始初始化
    reset_set.FILE_STATUS = 0;
    reset_set.WORK_MODE_FLAG = 0;
    reset_set.WORK_SET_MODE_FLAG = 0;
    reset_set.WORK_VALUE = 100;
    reset_set.PA_COE_A = 0.0;
    reset_set.PA_COE_E = 1.0;
    reset_set.PA_COE_I = 0.0;
    reset_set.PA_COE_M = 0.0;
	reset_set.IAP_FLAG = 0;
	reset_set.FULL_BATTERY_MAH = 0;
	reset_set.TEMP_COE_A = 0.0f;
    reset_set.TEMP_COE_B = 0.0f;
    reset_set.TEMP_COE_C = 0.0f;
    reset_set.TEMP_COE_D = 0.0f;
	memset(reset_set.SV_SN_BUF,0,sizeof(reset_set.SV_SN_BUF));
	memset(reset_set.TEMP_SN_BUF,0,sizeof(reset_set.TEMP_SN_BUF));
	memset(reset_set.PRESSURE_SN_BUF,0,sizeof(reset_set.PRESSURE_SN_BUF));
	reset_set.BATTERY_CAL_FLAG = 0;
	reset_set.DEPTH_EC_X = 0.0f;
	reset_set.DEPTH_EC_Y = 0.0f;
    AT24C02_Write(0,(uint8_t*)&reset_set,sizeof(reset_set));
	send_response(cmd_buf->cmd_code,NULL,cmd_buf->data_Bsize);
	
    //重启
    NVIC_SystemReset();
}

/*
    当前电量及电池校准功能函数
*/
void battery_power_func(uasrt_cmd *cmd_buf){
    if(validate_download_request(rx_buffer,rx_length) == -1){
        printf("CRC ERROR\r\n");
        return ;
    }
    //构造返回帧
	uint8_t buf[cmd_buf->data_Bsize];
    if(cmd_buf->cmd_code == READ_CURRENT_POWER){
		memcpy(buf,&SOC,cmd_buf->data_Bsize);
        send_response(cmd_buf->cmd_code,buf,cmd_buf->data_Bsize);
    }else{
		;
    }
}

/*
	设备连接状态
*/
static void link_status(uasrt_cmd *cmd_buf){
	if(validate_download_request(rx_buffer,rx_length) == -1){
        printf("CRC ERROR\r\n");
        return ;
    }
    /*
     * 显控连接标志只保存在 RAM 中：
     * 本次上电收到连接指令后，停止当前自容记录并禁止再次创建记录文件。
     * 复位或断电后，workmode_flag 会重新初始化为 0。
     */
	workmode_flag = 1U;
    battery_level_report_reset();
    pri_flush_record_now();
	send_response(cmd_buf->cmd_code,NULL,cmd_buf->data_Bsize);
}

/*
	设备断开连接状态
*/
static void unlink_status(uasrt_cmd *cmd_buf){
	if(validate_download_request(rx_buffer,rx_length) == -1){
        printf("CRC ERROR\r\n");
        return ;
    }
    /*
     * 显控断开后，只清除本次上电连接标志，不修改 EEPROM 里的工作模式。
     * 这样自容模式在未连接状态下可以恢复正常入水记录。
     */
	workmode_flag = 0U;
    battery_level_report_reset();
	send_response(cmd_buf->cmd_code,NULL,cmd_buf->data_Bsize);
}

/*
    CRC校验 成功返回长度字段
*/
int validate_download_request(uint8_t *buffer, int len) {
    if (buffer[0] != 0xAA) {
        printf("帧头错误\r\n");
        return -1;
    }
    uint16_t data_len = buffer[2];
    // CRC校验
    CRC_16 = CRC16(buffer, data_len);
    CRC_LOW = CRC_16 & 0x00ff;
    CRC_HIG = (CRC_16 >> 8);
    if((CRC_LOW != rx_buffer[CMD_CRC_L_INDEX(len)]) || (CRC_HIG != rx_buffer[CMD_CRC_H_INDEX(len)])){
        printf("CRC校验失败\r\n");
        return -1;
    }
    return data_len;
}

/*
    file指令 CRC校验 成功返回长度字段
*/
int validate_download_request_file(uint8_t *buffer, int len) {
    if (buffer[0] != 0xAA) {
        printf("帧头错误\r\n");
        return -1;
    }
    uint16_t data_len = (buffer[3] << 8) | buffer[2];
    // CRC校验
    CRC_16 = CRC16(buffer, data_len+1);
    CRC_LOW = CRC_16 & 0x00ff;
    CRC_HIG = (CRC_16 >> 8);
    if((CRC_LOW != rx_buffer[CMD_CRC_L_INDEX(len)]) || (CRC_HIG != rx_buffer[CMD_CRC_H_INDEX(len)])){
        printf("CRC校验失败\r\n");
        return -1;
    }
    return data_len;
}

/*
* @brief 处理接收到的指令数据
* @param  None
*/
void cmd_ProcessData(){
    if(frame_receied){
        if(rx_buffer[CMD_HEAD_INDEX] == RX_CMD_HEAD){
                uint8_t cmd  = rx_buffer[CMD_CODE_INDEX]; //指令码 也可用于返回指令帧构成结构体的下标
                if(cmd < TX_CMD_COUNT){
                    if( cmd == UPDATA_SYS){
                        program_upgrade();
                    }else{
                        int valid_len;
                        if(tx_cmd[cmd].rw == 3){
                            valid_len = validate_download_request_file(rx_buffer,rx_length);
                        }else{
                            valid_len = validate_download_request(rx_buffer,rx_length);
                        }

                        if(valid_len != -1){
                            /*
                             * 自容模式正在记录时禁止普通 EEPROM 写入：不写 EEPROM、
                             * 不回复配置帧，避免打断连续剖面文件，也避免显控误以为修改成功。
                             */
                            if((tx_cmd[cmd].rw == 0) &&
                               (svp_cmd.WORK_MODE_FLAG == 0x00) &&
                               (workmode_flag == 0U) &&
                               (file_state_return() != STATE_FILE_IDLE)){
                                /*
                                 * 自容记录期间不允许普通 EEPROM 写入。
                                 * 显控已经连接后，workmode_flag 为 1，因此允许参数配置。
                                 */
                            }else{
								/* 判断该指令是否需要关闭当前正在记录的文件。 */
                                if(cmd_needs_record_close(cmd)){
                                    pri_flush_record_now();
                                }
                                switch(tx_cmd[cmd].rw ){
                                    case 1:
                                        r_cmd_composition(&tx_cmd[cmd]);    /*普通读指令*/
                                        break;
                                    case 0:
                                        w_cmd_composition(&tx_cmd[cmd]);    /*普通写指令*/
                                        break;
                                    case 3:
                                        //file_composition(&tx_cmd[cmd]);     /*文件操作指令*/
                                        file_manager_fsm_process(rx_buffer,rx_length);//   文件操作函数
                                        break;
                                    case 4:
                                        set_mode_func(&tx_cmd[cmd]);        /*工作模式指令*/
                                        break;
                                    case 5:
                                        set_rtc_data_time(&tx_cmd[cmd]);    /*RTC时间指令*/
                                        break;
                                    case 6:
                                        pri_mode_func(&tx_cmd[cmd]);        /*未读写EEPROM指令*/
                                        break;
                                    case 7:
                                        reset_func(&tx_cmd[cmd]);           /*恢复出厂设置*/
                                        break;
                                    case 8:
                                        battery_power_func(&tx_cmd[cmd]);   /*电池电量*/
                                        break;
                                    case 9:
                                        if(cmd == LINK_STATE){
                                            link_status(&tx_cmd[cmd]);      /*连接状态*/
                                        }else if(cmd == UNLINK_STATE){
                                            unlink_status(&tx_cmd[cmd]);    /*断开连接状态*/
                                        }
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                }
//            }
        }
        //重新启动DMA传输
        frame_receied = 0;
        HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
    }
}



/*------------------------------------------------------------------
** 函数原型: int CRC16 (const char *nData, int wLength)
** 功能描述: CRC16校验
** 输入参数: const char *nData, int wLength
** 调用模块: None
** 返 回 值: wCRCWord
** 说    明：
-------------------------------------------------------------------*/
static const int wCRCTable[] =
{
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

int CRC16(const void *_nData, uint16_t wLength)
{
    const char *nData;
    unsigned char nTemp;
    unsigned char CRCHI = 0XFF;
    unsigned char CRCLO = 0XFF;
    nData = (const char *)_nData;
    while (wLength--) 
    {
        nTemp = CRCHI ^ (*(nData++)); 	
        CRCHI = CRCLO ^ wCRCTable[nTemp]; 		
        CRCLO = wCRCTable[nTemp]>>8; 
    }
    return ((unsigned short)CRCLO << 8 | CRCHI);
} // End: CRC16

