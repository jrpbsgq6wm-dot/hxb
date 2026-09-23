#include "main.h"


#define W25QXX_FATFS 
#define PACKET_SEQNO_INDEX  (1)
#define PACKET_SEQNO_COMP_INDEX (2)
#define PACKET_HEADER   (3)
#define PACKET_TRAILER  (2)

#define PACKET_OVERHEAD (PACKET_HEADER+PACKET_TRAILER)
#define PACKET_SIZE     (128)
#define PACKET_1K_SIZE  (1024)

#define APP_LOAD_ADDR       0x08020000UL
/*
 * STM32F411 的扇区 5 范围为 0x08020000 ~ 0x0803FFFF，共 128 KB。
 * 当前 APP 链接地址就是 0x08020000，因此升级固件的最大允许长度不能超过该范围。
 */
#define APP_MAX_SIZE        (128UL * 1024UL)

typedef void (*pFunction)(void);
void JumpToApplication(uint32_t ApplicationAddress){
    pFunction Jump_to_App;
    uint32_t Jumpaddr;
    if((*(__IO uint32_t*)(ApplicationAddress) & 0x2FFE0000) != 0x20000000){
        return ;
    }
    //设置主堆栈区
    __set_MSP(*(__IO uint32_t*)ApplicationAddress);
    //获取复位处理函数地址
    Jumpaddr = *(__IO uint32_t *)(ApplicationAddress +4);
    Jump_to_App = (pFunction)Jumpaddr;
    
    //重置所有外设
    HAL_RCC_DeInit();
    HAL_DeInit();
    
    //设置向量表偏移
    SCB->VTOR = ApplicationAddress;

    //跳转
    Jump_to_App();
}


void sys_init(){
    
    HAL_Init();                    	    //初始化HAL库    
    Stm32_Clock_Init(96,4,2,4);         //设置时钟,96Mhz
    delay_init(96);                     //初始化systick延时函数
    
    /* IIC - EEPROM RTC */
    IIC_Init();
    
    /* USART */
    usart_bound();                          //初始化串口115200
	LED_Init();                             //初始化LED

}


char file_name[256];
uint8_t file_name_buf[256] = {0};
uint8_t file_size_buf[8] = {0};
volatile int file_size;
uint8_t ymodem_flag = 0;
/**
 * @brief 文件大小的ASSIC码的十六进制转换为int类型
 * @param 字符串数组
 * @retval 
 */
int strToint(uint8_t *str,unsigned int len){
    int res = 0,i=0;
    for(i=0;i<len;i++){
        if(str[i] < '0' || str[i] > '9'){
            return -1;
        }
        
        res = res * 10 + (str[i] - '0');
    }
    return res;
}
int Get_Ymodme_File_Information(uint8_t* data,uint32_t len){
    int i = 0,count = 0,file_size_len;
    for(i=0;i<len;i++){
        if(data[i] == 0x00){
            file_name_buf[count] = '\0';
            break;
        }else{
            file_name_buf[count] = data[i];
            count++;
        }
    } 
    memcpy(file_name,file_name_buf,count);
    count = 0;
    for(i+=1;i<len;i++){
        if(data[i] == 0x20){
            file_size_buf[count] = '\0';
            break;
        }else{
            file_size_buf[count] = data[i];
            count++;
        }
    }
    file_size_len = count;
    file_size = strToint(file_size_buf,file_size_len);
    return file_size;
}

void send_c(void){
    uint8_t buf[1];
    memset(buf,0X43,1);
    Send_data(buf,1);
}
void Send_Ack(void){
    uint8_t buf[1];
    memset(buf,0X06,1);
    Send_data(buf,1);
}
void Send_Nak(void){
    uint8_t buf[1];
    memset(buf,0X15,1);
    Send_data(buf,1);
}
void Send_CA(void){
    uint8_t buf[1];
    memset(buf,0X18,1);
    Send_data(buf,1);
}

uint32_t u_indx;
int main(void)
{   
	sys_init();         //系统初始化
    uint16_t packet_crc/*CRC校验*/,packet_size/*数据包大小*/;
    int error_count/*错误计数*/,updata_flag = 0/*文件更新状态 0:未完成更新 1:完成更新*/,updataing_flag = 0/*开始文件传输标志位: 1文件开始传输 0文件传输结束*/; 
    uint16_t crcc16;
    //判断eeprom标志位   AA 需要更新 55更新完成
    uint8_t updata_buf;
    updata_flag = AT24C02_Read(IAP_UPDATA_ADDR,&updata_buf,1);
	
//    updata_buf = 0X55;
//    AT24C02_Write(IAP_UPDATA_ADDR,&updata_buf,1);
//	
	
//    while(1){
//        if(frame_receied){
//            if(rx_buffer[1] == 0x00){
//                Send_Ack();
//            }
//            send_c();
//            memset(rx_buffer,0,sizeof(rx_buffer));
//            frame_receied = 0;
//            rx_length = 0;
//        }
//    }
    
    /*
     * 新版 APP 已超过 64 KB，不能再像旧实现一样把全部固件暂存在 RAM。
     * u_indx 表示已经成功写入 APP Flash 的字节数，每收到一个 1 KB 数据帧就
     * 立即写入 Flash，因此此处只需要保存长度，不需要大容量升级缓存。
     */
    u_indx = 0U;
    if(updata_buf == 0xAA){    
        send_c();
        uint16_t countt = 1;
        while(1){
            if(frame_receied){
                frame_receied = 0;
                uint8_t c = rx_buffer[0];
                switch(c){
                    case 0x01:
                        packet_size = PACKET_SIZE;
                        packet_crc = rx_buffer[3+packet_size+1] << 8;    //低字节变高字节
                        packet_crc += rx_buffer[3+packet_size];          //高字节变低字节
                        crcc16 = CRC16(&rx_buffer[PACKET_HEADER],packet_size);
                        if((rx_buffer[1] == 0x00 && rx_buffer[2] == 0XFF) && (crcc16 == packet_crc)){
                            updataing_flag = 0; //文件传输状态结束
                            updata_flag = 1;    //更新状态标记完成
                            /*
                             * 数据帧已经在接收过程中逐包写入 Flash。
                             * 此处只保留原协议的结束确认，不再重复从 RAM 整体写入。
                             */
                            Send_Ack();
                        }
                        break;
                    case 0x02:{
                            if(rx_buffer[1] == 0x00){  //首帧
                                /*
                                 * 升级首帧到达后只擦除 APP 所在的扇区 5。
                                 * APP 起始地址为 0x08020000，扇区容量为 128 KB，
                                 * 可容纳当前约 70 KB 的 FreeRTOS APP。
                                 */
                                if(Erase_sector(APP_LOAD_ADDR) == HAL_OK){
                                    u_indx = 0U;
                                    countt = 1U;
                                    updataing_flag = 1; //文件传输状态表示为开始传输
                                    Send_Ack();
                                }else{
                                    updataing_flag = 0;
                                    Send_CA();
                                }
                            }else{  //数据帧
                                if(updataing_flag){
                                    if(rx_buffer[1] == countt){    //帧序号和帧计数相同 - 可以进行flash烧录
                                        LED1 = !LED1;
                                        /*
                                         * 接收一个 1 KB 数据包后立即写入 APP Flash。
                                         * 旧代码在这里 memcpy 到 64 KB 的 updatabuf，
                                         * 当固件超过 64 KB 时会越界并破坏内存。
                                         */
                                        if(u_indx > (APP_MAX_SIZE - PACKET_1K_SIZE)){
                                            /* 固件超过 APP 扇区容量，终止本次升级。 */
                                            updataing_flag = 0;
                                            Send_CA();
                                        }else if(STMFLASH_Write_NoErase(APP_LOAD_ADDR + u_indx,
                                                                        &rx_buffer[PACKET_HEADER],
                                                                        PACKET_1K_SIZE) == HAL_OK){
                                            u_indx += PACKET_1K_SIZE;
                                            memset(rx_buffer,0x00,sizeof(rx_buffer));
                                            Send_Ack();
                                            countt++;
                                        }else{
                                            /*
                                             * 写 Flash 失败后不能继续接收后续帧，
                                             * 否则会得到不完整 APP。显控收到 CA 后应重新开始升级。
                                             */
                                            updataing_flag = 0;
                                            Send_CA();
                                        }
                                    }else{
                                        Send_Ack();        //可能出现了和上次相同的序号帧
                                    }
                                }
                            }
                        break;
                    }
                    case 0XAA:{ //更新指令
                        updata_flag = 0;
                        updataing_flag = 0; //文件传输状态结束
                        Erase_sector(APP_LOAD_ADDR);//擦除flash扇区 
                        send_c();
                        break;
                    }
                    default:{
                        error_count++;
                        if(error_count >=5){
                            Send_CA();
                        }else{
                            Send_Nak();
                        }
                        break;
                    }
                }
                frame_receied = 0;
                rx_length = 0;
            }else{
                if(updata_flag && !updataing_flag){ //程序更新成功跳转
                    updata_flag = 0;
                    //修改标志位
                    updata_buf = 0x55;
                    AT24C02_Write(IAP_UPDATA_ADDR,&updata_buf,1);
                    JumpToApplication(APP_LOAD_ADDR);
                }
            } 
        }
    }else{
        JumpToApplication(APP_LOAD_ADDR);
    }
    
}
