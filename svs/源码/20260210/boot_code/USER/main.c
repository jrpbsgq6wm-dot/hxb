#include <string.h>
#include "main.h"
#include "stmflash.h"
#include "stm32f10x.h" 
#include "delay.h"
#include "usart1.h"
#include "ymodem.h"
#include "led.h"

const u8 Program_Version_Buffer[]={"SVS1500_V1.3"};       //要写入到STM32 FLASH的字符串数组 程序版本号
#define Version_Size sizeof(Program_Version_Buffer)/2 
//数组长度
//SN号获取初始化
void program_version_num(void)
{
    STMFLASH_Write(BOOT_SN_ADDR,(u16*)Program_Version_Buffer,Version_Size);
}

uint8_t temp_value[UART1_RXBUFF_SIZE]={0};
char file_name[256];

typedef void (*iap_function)(void);
void IAP_LOAD_APP(uint32_t app_addr){
    SCB->VTOR = FLASH_BASE | 0x00004000;    //更改中断向量表地址    
    iap_function jump2app;
    //给函数指针赋值跳转地址
    jump2app = (iap_function)*(vu32*)(app_addr+4);	//用户代码区第二个字为程序开始地址（复位地址）
    __set_MSP(*(vu32*)app_addr);	//设置主堆栈区
    jump2app();	//跳转到app
}

int main(void)
{  
    /*系统初始化*/
    SystemInit();
    delay_init(72);
    UART1_Init(115200);
    LED_Init();
    
    /*变量声明*/
    uint32_t addr = APP_LOAD_ADDR;//APP程序的起始地址  
    int error_count=0/*错误计数*/,
        filesize=0/*文件大小*/,
        updata_flag = 0/*文件更新状态 0:未完成更新 1:完成更新*/,
        updataing_flag = 0/*开始文件传输标志位: 1文件开始传输 0文件传输结束*/,
        i=0; 
    uint16_t packet_crc=0/*CRC校验*/,
        packet_size=0/*数据包大小*/;
    uint8_t updata_buf[2]={0}/*更新标志位*/;
    uint8_t head_buf[128]={0};
        
    /*程序*/    
    STMFLASH_Read(APP_LOAD_ADDR,(u16*)head_buf,64);
    for(i=0;i<128;i++){ //读取从08004000-0800FFFF的数据是否有程序
        if(head_buf[i] == 0xFF){
            continue;   //如果为0xFF 开始下一次循环 - i==128：存储程序的地址空间为空(0xFF) - 需要更新
        }else{  
            break;      //如果不为0xff 跳出循环 - i<128：存储程序的地址空间不为空 - 可能无需更新
        }
    }
    
    read_flash_flag(UPDATA_FLAG,(u16 *)updata_buf);     //app程序的更新标志位 
    
    if((updata_buf[0] == 0xAA && updata_buf[1] == 0xAA) || (i == 128)){ //需要更新程序 - 标志位 或者 app前128字节是否为全FF
        uint16_t start_c = 0;
        read_flash_flag(UPDATA_APP_BOOT,(u16 *)&start_c); 
        /*判断是否要进行文件传输标志位*/
        if(start_c == 1){
            Send_Byte(C);
            start_c = 0;
            write_flash_flag(UPDATA_APP_BOOT,start_c);
        }
        uint16_t countt = 1;
        while(1){      
            if(ymodem_flag){    //串口接收到数据
                ymodem_flag = 0;
                uint8_t c = temp_value[0];
                uint8_t count = UART1_RxCounter;
                LED0 = ~LED0;
                switch(c){  //获取数据帧头
                    case 0x01:{
                        packet_size = PACKET_SIZE;
                        packet_crc = temp_value[3+packet_size+1] << 8;    //低字节变高字节
                        packet_crc += temp_value[3+packet_size];          //高字节变低字节
                        uint16_t crcc16 = CRC16(&temp_value[PACKET_HEADER],packet_size);                       
                        if((temp_value[1] == 0x00 && temp_value[2] == 0XFF) && (crcc16 == packet_crc)){
                            updataing_flag = 0; //文件传输状态结束
                            updata_flag = 1;    //更新状态标记完成
                            Send_Byte(ACK);
                        }
                        break;
                    }
                    case 0x02:{
                        packet_size = PACKET_1K_SIZE;
                        packet_crc = temp_value[3+packet_size+1] << 8;    //低字节变高字节
                        packet_crc += temp_value[3+packet_size];          //高字节变低字节
                        uint16_t crcc16 = CRC16(&temp_value[PACKET_HEADER],packet_size);
                        if(temp_value[PACKET_SEQNO_INDEX] == ((temp_value[PACKET_SEQNO_COMP_INDEX]^0XFF) & 0XFF) && (crcc16 == packet_crc)){
                            //校验正确
                            if(temp_value[1] == 0x00){  //首帧
                                Erase_Flash_Section(APP_LOAD_ADDR);//擦除从flash
                                filesize = Get_Ymodme_File_Information(temp_value,packet_size);
                                if(filesize == 0){
                                    Send_Byte(NAK);
                                }
                                updataing_flag = 1; //文件传输状态表示为开始传输
                                Send_Byte(ACK);
                            }else{  //数据帧
                                if(updataing_flag){
                                    if(temp_value[1] == countt){    //帧序号和帧计数相同 - 可以进行flash烧录
                                        STMFLASH_Write(addr,(u16*)&temp_value[PACKET_HEADER],PACKET_1K_SIZE/2);   //将文件数据烧写到FLASH
                                        addr += PACKET_1K_SIZE;
                                        Send_Byte(ACK);
                                        countt++;
                                    }else{
                                        Send_Byte(ACK);         //可能出现了和上次相同的序号帧
                                    }
                                }
                            }
                        }else{
                            //校验错误
                            Send_Byte(NAK);
                        }
                        break;
                    }
                    case 0XAA:{ //更新指令
                        updata_flag = 0;
                        updataing_flag = 0; //文件传输状态结束
                        Erase_Flash_Section(APP_LOAD_ADDR); //擦除app部分的程序
                        addr = APP_LOAD_ADDR;
                        Send_Byte(C);
                        break;
                    }
                    default:{
                        error_count++;
                        if(error_count >=5){
                            Send_Byte(CA);
                        }else{
                            Send_Byte(NAK);
                        }
                        break;
                    }
                }
            }else{
                if(updata_flag && !updataing_flag){ //程序更新成功跳转
                    updata_flag = 0;
                    //修改标志位
                    updata_buf[0] = 0x55;
                    updata_buf[1] = 0x55;
                    STMFLASH_Write(UPDATA_FLAG,(u16*)updata_buf,1);
                    IAP_LOAD_APP(APP_LOAD_ADDR);
                }
            } 
        }
    }else{
        IAP_LOAD_APP(APP_LOAD_ADDR);    //无需更新 跳转到app程序执行
    }
}
