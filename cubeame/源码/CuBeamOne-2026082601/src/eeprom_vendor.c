/*******************************eeprom_vendor.h-2024-06-20************************/
/******************************显控厂商信息 - eeprom (测试使用)************************/
#include "eeprom_vendor.h"

/********************************************************************************
 * 名称：                    write_vendor_info()
 * 功能：                    将厂商信息写入到eeprom中
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
uint8_t eeprom_map_flag = 0;


int eeprom_init()
{
    int fd_eeprom = open(EEPROM_DEV_NAME, O_RDWR);
    DBG("fp_eeprom:%d\n",fd_eeprom);
    if (fd_eeprom < 0)
    {
        perror("Can't open eeprom\n"); //打开iic设备文件失败
        return -1;
    }
    return fd_eeprom;

}
/********************************************************************************
 * 名称：                    eeprom_write_vendor()
 * 功能：                    将显控程序发送过来的厂商信息更新在eeprom 如果没有就用程序默认的
 * 入口参数：            	 eeprom 文件描述符 
 * 出口参数：            	 正常退出返回0 错误返回-1
 *********************************************************************************/
int eeprom_write_vendor(int fd_eeprom){
    char vendor_write_buffer[EEPROM_WR_SIZE_VENDOR];
    int recv_len,write_len;
    char DataHead[4] = {'#','#','R','E'}; // 回复头
    memset(vendor_write_buffer,0,sizeof(vendor_write_buffer));
    DBG("write vendor to eeprom\n");
    recv_len = recv(Connect_fd, vendor_write_buffer, EEPROM_WR_SIZE_VENDOR, 0); 
    if (recv_len < 0)
    {
        error_process();
        return -1;
    }
    DBG("Vendor Info: %s\nRecv_from_spper_msg_len: %d\n",vendor_write_buffer,recv_len);
    vendor_write_buffer[recv_len] = '\0';
    //向eeprom写入数据
    if(lseek(fd_eeprom, EEPROM_SEEK_VENDOR, SEEK_SET)== -1){
        perror("Set eeprom_vendor_info failed\n");
        close(fd_eeprom);
        return -1; 
    }
    //将新的厂商信息写入到eeprom
    write_len = write(fd_eeprom,vendor_write_buffer,recv_len+1);
    if( write_len < 0 ){
        perror("Write Vendor Info to eeprom failed\n");
        close(fd_eeprom);
        return -1;
    }
#if 0
    if (send(Connect_fd, DataHead, SIZE_OF_LONG, 0) < 0){
        close(fd_eeprom);
        error_process();
        return -1;
    }
#endif
    DBG("write vendor to eeprom access !!!\n");
    return 0;
}

/********************************************************************************
 * 名称：                    eeprom_read_vendor()
 * 功能：                    读取eeprom中厂商信息并发送给显控程序
 * 入口参数：            	 eeprom 文件描述符 
 * 出口参数：            	 正常退出返回0 错误返回-1
 *********************************************************************************/
int eeprom_read_vendor(int fd_eeprom){
    char vendor_read_buffer[EEPROM_WR_SIZE_VENDOR];
    char send_data_buffer[EEPROM_WR_SIZE_VENDOR + 5]; //
    memset(send_data_buffer,0,sizeof(send_data_buffer));
    memset(vendor_read_buffer, 0, sizeof(vendor_read_buffer));
    int read_len,str_len;
    DBG("read vendor from eeprom\n");
    //读取eeprom硬件信息，放入缓冲区后，发送给显控程序
    memset(vendor_read_buffer,0,sizeof(vendor_read_buffer));
    if(lseek(fd_eeprom, EEPROM_SEEK_VENDOR, SEEK_SET)== -1){
        perror("Set mtd_vendor_info failed\n");
        close(fd_eeprom);
        return -1; 
    }
    //读取eeprom中的厂商信息
    read_len = read(fd_eeprom,vendor_read_buffer,EEPROM_WR_SIZE_VENDOR);
    if( read_len < 0){
        perror("Read Vendor Info from eeprom failed\n");
        close(fd_eeprom);
        return -1;
    }
    str_len = strlen(vendor_read_buffer);
#if 0
    if(read_len == 0){
        strcpy(vendor_read_buffer,"Use the <<HE_WRITE + MSG instruction to write and then read");
        read_len = strlen(vendor_read_buffer);
    }
#endif 
    DBG("Vendor Info: %s read_from_eeprom_msg_len: %d strlen: %d\n",vendor_read_buffer,read_len,str_len);

    //回复消息给显控程序  头+data
    send_data_buffer[0] = '#';
    send_data_buffer[1] = '#';
    send_data_buffer[2] = 'R';
    send_data_buffer[3] = 'E';
    send_data_buffer[4] = str_len;
    memcpy(send_data_buffer+5,vendor_read_buffer,str_len);

#if 1
    pthread_mutex_lock(&mut);
    if(send(Connect_fd,send_data_buffer,str_len+5,0) < 0){
        perror("send error\n");
        close(fd_eeprom);
        error_process();
        return -1;
    }
    pthread_mutex_unlock(&mut);
#endif 
    DBG("%d\n%s\n",strlen(vendor_read_buffer),vendor_read_buffer);
    DBG("read vendor from eeprom over !!!\n");
    return 0;
}

/********************************************************************************
 * 名称：                   eeprom_option()
 * 功能：                    eeprom读写选项
 * 入口参数：            	 无
 * 出口参数：            	 成功执行返回0 失败返回-1 
 *********************************************************************************/

int eeprom_option(void){
    DBG("recv upper commond : <<HE\n");
    char DataHead[6];
    int svp_recv_head;
    int fd_eeprom;
    //接收完整显控指令
    svp_recv_head = recv(Connect_fd, DataHead, 6, 0);
    if (svp_recv_head <= 0)
	{
		error_process();
		return -1;
	}
    //打开eeprom设备
    fd_eeprom = eeprom_init();
    if(fd_eeprom == -1){
        return -1;
    }
    if(DataHead[0] == '_' && DataHead[1] == 'R' && DataHead[2] == 'E' && DataHead[3] == 'A' && DataHead[4] == 'D' && DataHead[5] == '_'){
        //读取eeprom硬件信息,并回复消息给显控程序
        DBG("read eeprom vendor msg send to upper\n");
        if(eeprom_read_vendor(fd_eeprom) == -1){
            perror("eeprom_read_vendor error\n");
            close(fd_eeprom);
            return -1;
        }
    }else if(DataHead[0] == '_' && DataHead[1] == 'W' && DataHead[2] == 'R' && DataHead[3] == 'I' && DataHead[4] == 'T' && DataHead[5] == 'E'){
        //硬件信息写入eeprom,并回复消息给显控程序
        DBG("write vendor msg to eeprom\n");
        if(eeprom_write_vendor(fd_eeprom) == -1){
            perror("eeprom_write_vendor error\n");
            close(fd_eeprom);
            return -1;
        }
    }
    close(fd_eeprom);
    return 0;
}

//动态创建一个新的i2c设备，设备为24c256 eeprom芯片 地址为0x55 将eeprom映射为文件
void eeprom_file_mapping(){
    FILE* file;
    char* buf = "24c256 0x55";
    file = fopen("/sys/bus/i2c/devices/i2c-0/new_device","w");
    if(file == NULL){
        perror("file open failed");
        return -1;
    }
    size_t wlen = fwrite(buf,sizeof(char),strlen(buf),file);
    DBG("%d\n",wlen);
    DBG("file open succeeded\n");
    eeprom_map_flag = 1;    //完成映射
    fclose(file);
}