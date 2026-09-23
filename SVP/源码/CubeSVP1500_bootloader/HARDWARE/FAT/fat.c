#include "fat.h"

/*************************     file       *****************************/
const char f_flashDirName[10] = {"0"};    //文件架名称
FIL fsrc;                                       // file objects
uint16_t f_haveFileNumber;                      // 0:/现有文件个数文件个数
/**********************************************************************/

FATFS USERFstFs;
BYTE wwork[4096];
MKFS_PARM opt = {0};
FRESULT ret;

uint8_t Fatfs_unmount(void){
    
    return f_unmount(DISK_PATH);
}

void fat_error_func(void){
    Fatfs_unmount();
    Fatfs_mount(0);
}

/*
    删除路径下最旧文件，直到释放出足够的空间
    path 目标路径
    required_Kbytes 需要释放的KB ---- 1M
    retval_fresult 操作结果
*/
FRESULT free_space_by_deleting_oldest(const char* path,uint32_t required_Kbytes){
    DIR dir;
    FILINFO fno;
    uint32_t free_bytes = 0;                 
    uint8_t need_more_space_flag = 1,attempt=0;
    //最多尝试删除100个文件防止无限循环
    for(attempt = 0; attempt<100 && need_more_space_flag;attempt++){
        FILINFO oldest_file = {0};
        char oldest_path[256] = {0};
        //第一次遍历：寻找最旧文件
        ret = f_opendir(&dir,path);
        if(ret != FR_OK)
            return ret;
        while(1){
            ret = f_readdir(&dir,&fno);
            if(ret != FR_OK || fno.fname[0] == 0) break;
            if(fno.fattrib & (AM_DIR|AM_SYS|AM_HID)) continue;  //跳过目录和系统文件

            //获取当前文件的时间 通过当前文件时间的比对 来判断出最早的创建的文件和最新创建的文件
            uint32_t file_time = (fno.fdate << 16) | fno.ftime;
            uint32_t oldest_time = (oldest_file.fdate << 16) | oldest_file.ftime;
            
            if(oldest_file.fname[0] == 0 || file_time < oldest_time){
                oldest_file = fno;
                snprintf(oldest_path,sizeof(oldest_path),"%s/%s",path,fno.fname);
                printf("[%s]:oldest_path %s\r\n",__func__,oldest_path);
            }
        }
        f_closedir(&dir);
        
        if(oldest_file.fname[0] == 0){
            printf("[%s]:There are no documents.",__func__);    //没有可删除文件
            return FR_DISK_FULL;
        }
        //删除找到的最旧文件
        printf("[%s]:DELETE file:%s (size: %d)\r\n",__func__,oldest_path,oldest_file.fsize);
        ret = f_unlink(oldest_path);
        if(ret != FR_OK){
            printf("[%s]:DELETE file failed (%d).\r\n",__func__,ret);
            return ret;
        }
        free_bytes += oldest_file.fsize;
        
        //检查是否已经释放了足够的空间 
        uint32_t free_clust;
        free_clust = FLASH_FreeSize();
        printf("[%s]:Release:%d,BE left:%d\r\n",__func__,free_bytes,free_clust);
        if(free_clust >= required_Kbytes){  //剩余空间大于1M
            need_more_space_flag = 0;
        }
    }
    return need_more_space_flag ? FR_DISK_FULL : FR_OK;
}
 
void init_filestruct(Pri_File_T *p_file){
    
}

FRESULT FLASH_Fat_Init(uint8_t f_mkfs_flag){
    FRESULT ret;
    
    ret = Fatfs_mount(f_mkfs_flag); 
    if(ret != FR_OK){
        return ret;    //挂载失败
//    if(f_mkfs_flag){    //如果是格式化后挂载，此时磁盘内会清空所有文件 这时候会创建一个 以便后续是否能检测到文件
//        char rtcfilename[255];
//        DS3231_Read_All();
//        DS3231_Read_Time();
//        sprintf(rtcfilename,"20%.2d%.2d%.2d_%.2d%.2d%.2d.txt", DS3231_Time.year,DS3231_Time.mon,DS3231_Time.date,DS3231_Time.hour,DS3231_Time.min,DS3231_Time.sec);
//        printf("[%s]:Create File To Flash - File Name :%s\r\n",__func__,rtcfilename);
//        //createFile(rtcfilename);
//    }
    }
    /*挂载成功 - FLASH信息读取*/
    ret = my_scan_file(DISK_PATH); //读取当前根目录下的文件信息  文件名 修改时间 文件大小 并打印
    if(ret != FR_OK){
        printf("[%s]:Scan File failed.\r\n",__func__);
        return ret;
    }
    
       
    uint32_t flash_KB = FLASH_FreeSize();
    //打印当前FLASH的占用空间KB 判断是否需要释放空间
    if(flash_KB < REQUIRED_SPACE){   
        printf("[%s]:There is not enough space ，Clear up the early files.\r\n",__func__);
        free_space_by_deleting_oldest(DISK_PATH,REQUIRED_SPACE); //释放当前空间
    }
    printf("[%s]:Space Abundant : %dKB\r\n",__func__,flash_KB);
   
    
    
    /*初始化文件结构体*/
    init_filestruct(&pri_file);
    return FR_OK;
}

FRESULT f_mkfs_func(void){
    return Fatfs_mount(1);
}

/*
    挂载文件系统
        挂载失败后对没有文件系统的错误进行了判断，如果没有文件系统会重新格式化创建文件系统
        其他错误之间返回错误码
    如果参数为return1 则先格式化后挂载
*/
FRESULT Fatfs_mount(uint8_t f_mkfs_flag){

    opt.fmt = FM_FAT;
    opt.n_fat = 1;
    opt.align = 0;
    opt.n_root = 0;
    opt.au_size = 0;
    
    if(f_mkfs_flag == 1 ){  
        f_mkfs("0:",&opt,wwork,sizeof(wwork));
    }
    ret = f_mount(&USERFstFs,DISK_PATH,1);
    if(ret == 13){      //FLASH 没有文件系统 即将进行格式化
        ret = f_mkfs("0:",&opt,wwork,sizeof(wwork));
        if(ret == 0){       //FLASH 是否完成格式化
            ret = f_mount(NULL,DISK_PATH,1);
            ret = f_mount(&USERFstFs,DISK_PATH,1);
            if(ret == FR_OK){       //重新挂载
                return ret;
            }else{
                return ret;
            }
        }else{      //格式化失败
            return ret;
        }
    }else if(ret != FR_OK){     //其他原因
        return ret; 
    }else{      //成功挂载
        return ret;
    }
}




/*******************************************************************************
* Function Name  : uint32_t FLASH_FreeSize(void)
* Description    : 空间占用情况
* Input          : None
* Output         : None
* Return         : 返回剩余空间
* Attention      : None
*******************************************************************************/
uint32_t FLASH_FreeSize(void)
{
    FATFS *fs;
    DWORD fre_clust;
    DWORD freespace;
    ret = f_getfree("0:", &fre_clust, &fs);  /* 必须是根目录，选择磁盘0 */
    if(ret == 0){
        printf("*************** FAT DISK INFO **********************\r\n");
        //总扇区个数
        DWORD tot_sect = (fs->n_fatent - 2) * fs->csize;
        //剩余扇区个数
        DWORD fre_sect = fre_clust * fs->csize;
        //剩余空间 KB
        freespace = (fre_sect*fs->ssize)>>10;
        //总空间 KB
        DWORD totalSpace = (tot_sect*fs->ssize)>>10;
        
        printf("[%s]:FAT TYPE = %d [1:fat12 2fat16 3fat32 4exfat]\r\n",__func__,fs->fs_type);
        printf("[%s]:Sctor size(byte) = %d\r\n",__func__,fs->ssize);
        printf("[%s]:Total space(KB):%d\r\n",__func__,totalSpace);
        printf("[%s]:fre clust count = %d\r\n",__func__,fre_clust);       //剩余簇
        printf("[%s]:free sect count = %d\r\n",__func__,fre_sect);
        printf("[%s]:free space(KB):%d\r\n",__func__,freespace);
    }else{
        
    }
    return freespace;
}

FRESULT createFile(char *filename)
{
    DIR dirs; 
    char PathAndFileName[50];
    sprintf(PathAndFileName,"%s",filename);
    ret = f_open(&fsrc,PathAndFileName,FA_CREATE_ALWAYS);
    if(ret == FR_OK)               //打开文件成功
    {
        f_close(&fsrc);   //关闭文件
    }
    f_closedir(&dirs);  //关闭根目录
    return ret;
}

/*******************************************************************************
* Function Name  : DeletTheFile(char *filename)
* Description    : Delet the file
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
FRESULT DeletTheFile(char *filename)
{
    DIR dirs; 
    char PathAndFileName[50];
    ret = f_chdir("0:");                                    //切换到根目录
    ret = f_opendir(&dirs, f_flashDirName);
    sprintf(PathAndFileName,"%s/%s",f_flashDirName,filename);
    ret = f_chmod(PathAndFileName, AM_ARC, AM_RDO | AM_ARC);  //更改文件属性
    ret = f_unlink(PathAndFileName);
    f_closedir(&dirs);
    return ret;
}

/*
    遍历当前目录 没有文件夹创建
        获取文件个数
        填充文件信息结构体
        
*/
FRESULT my_scan_file(const char* path){
    FILINFO fno;
    DIR dir;
    //构建子目录
    
    //printf("[%s]:SVP_PATH:%s \r\n",__func__,path);
    ret =  f_opendir(&dir,"0:/");
    f_haveFileNumber = 0;
    //读取目录条目
    while(1){
        ret = f_readdir(&dir,&fno);
        if(ret != FR_OK || fno.fname[0] == 0){
            break;  //错误或结束
        }
        if(!(fno.fattrib & AM_DIR)){
            char time_buf[32];
            sprintf(time_buf,"%d/%.2d/%.2d %.2d:%2d:%.2d",YEAR(fno.fdate),MOON(fno.fdate),DATA(fno.fdate),HOU(fno.ftime),MIN(fno.ftime),SEC(fno.ftime));
            printf("[%s]:|--[F]%s %s %dbytes.\r\n",__func__,fno.fname,time_buf,fno.fsize);
            f_haveFileNumber++;
        }
    }
//    printf("[%s]: File count %d\r\n",__func__,f_haveFileNumber);
    f_closedir(&dir);
    return ret;
}




/*******************************************************************************
* Function Name  : void Wfile(char *filename,uint16_t* writebuf,uint32_t count)
* Description     :Read the file
* Input          : filename,writedbuf addr,count
* Output         : None
* Return         : None
*******************************************************************************/
void Wfile(char *filename,uint16_t* writebuf,uint32_t count)
{
    uint32_t bw;   
    DIR dirs;     
    char *PathAndFileName = mymalloc(50); //路径和文件名
    
    ret = f_chdir("0:");                //切换到根目录
    ret =  f_opendir(&dirs,f_flashDirName);
    if(ret != FR_OK)
    {
        ret = f_mkdir(f_flashDirName);    //创建文件夹
        ret =  f_opendir(&dirs,f_flashDirName);
    }
    sprintf(PathAndFileName,"%s/%s",f_flashDirName,filename);
    ret = f_open(&fsrc,PathAndFileName,FA_CREATE_ALWAYS | FA_WRITE);     //创建并对文档写操作
    if(ret == FR_OK)               //打开文件成功
    {
        ret = f_write(&fsrc,writebuf,count,&bw);                         //写入数据       
        f_close(&fsrc);   //关闭文件
    }
    myfree(PathAndFileName);
    f_closedir(&dirs);
}

/*将当前时间返回为字符类型*/
char* file_name_time(void){
    static char rtcfilename[32];
    sprintf(rtcfilename,"20%.2d%.2d%.2d_%.2d%.2d%.2d.txt",
                            DS3231_Time.year,DS3231_Time.mon,DS3231_Time.date,DS3231_Time.hour,DS3231_Time.min,DS3231_Time.sec);
    return rtcfilename;
}



void debug_fil(FIL* ffile){
    printf("FIL.OBJ.FS = %p\r\n",ffile->obj.fs);
    printf("OBJ ID = %u\r\n",ffile->obj.id);
    printf("fil_flag = 0x%x\r\n",ffile->flag);
    printf("fil_err = %u\r\n",ffile->err);
    printf("file fptr = 0x%x\r\n",ffile->fptr);
    printf("file clust = %d\r\n",ffile->clust);
}


/*
    读取所有文件名
*/
void read_all_file_name_func(void){
    FILINFO fno;
    DIR dir;
   
    uint8_t cmd_index= 0;
    FileInfo * myfilename = mymalloc(sizeof(FileInfo));
    if(myfilename == NULL)
        printf("[%s]:Create fileinfo failed\r\n",__func__);
    
    my_scan_file(DISK_PATH);
    uint32_t havefilecount = f_haveFileNumber;
    
    //组装首帧响应帧
    tx_buffer[cmd_index++] = RX_CMD_TAIL;
    tx_buffer[cmd_index++] = READ_ALL_FILE_NAME;
    tx_buffer[cmd_index++] = 7;
    memcpy(&tx_buffer[cmd_index],&havefilecount,4);
    cmd_index += 4;
    CRC_16 = CRC16(tx_buffer,7);
    tx_buffer[7] =  CRC_16&0X00FF;
    tx_buffer[8] = (CRC_16>>8);
    tx_buffer[9] = TX_CMD_TAIL;
    
    if(Send_data(tx_buffer,10) != HAL_OK)
        printf("%s ERROR\r\n",__func__);
    //
    memset(rx_buffer,0,sizeof(rx_buffer));
    frame_receied = 0;
    HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
    if(havefilecount == 0)
        return;
    while(1){
        if(rx_buffer[0] == 0x0F){
            frame_receied = 0;
            HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
            break;
        }
        else if(rx_buffer[0] == 0xF0){
            Send_data(tx_buffer,10);
        }
    }
    
    
    //组装中间数据帧
    ret =  f_opendir(&dir,"0:/");   //打开根目录
    while(havefilecount != 0){
        ret = f_readdir(&dir,&fno);
        if(ret != FR_OK || fno.fname[0] == 0){
            break;  //错误或结束
        }
        if(!(fno.fattrib & AM_DIR)){    //跳过目录
            memcpy(myfilename->filename,fno.fname,strlen(fno.fname));
            myfilename->attrib = fno.fattrib;
            myfilename->filesize = fno.fsize;   //byte;
            myfilename->mod_time = fno.ftime;
            
            cmd_index = 2;
            tx_buffer[cmd_index++] = strlen(myfilename->filename);
            memcpy(&tx_buffer[CMD_DATA_INDEX],myfilename->filename,strlen(myfilename->filename));
            cmd_index += strlen(myfilename->filename);
            uint32_t fbuf = myfilename->filesize;
            memcpy(&tx_buffer[CMD_DATA_INDEX],&fbuf,4);
            cmd_index += 4;
            
            
            CRC_16 = CRC16(tx_buffer,strlen(myfilename->filename) + 4 +2+1);
            tx_buffer[cmd_index++] =  CRC_16&0X00FF;
            tx_buffer[cmd_index++] = (CRC_16>>8);
            tx_buffer[cmd_index++] = TX_CMD_TAIL;
            
            
            if(Send_data(tx_buffer,cmd_index) != HAL_OK)
                printf("%s ERROR\r\n",__func__);
            memset(rx_buffer,0,sizeof(rx_buffer));
            
            while(1){
                if(rx_buffer[0] == 0x0F){
                    frame_receied = 0;
                    HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
                    break;
                }
                else if(rx_buffer[0] == 0xF0){
                    Send_data(tx_buffer,10);
                }
            }
            cmd_index = 0;
            havefilecount --;
        }
    }

    f_closedir(&dir);   //关闭目录

    //组装尾帧响应帧
    while(1){
        if(rx_buffer[0] == 0x0F){
            frame_receied = 0;
            HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
            break;
        }
        else if(rx_buffer[0] == 0xF0){
            Send_data(tx_buffer,10);
        }
    }
    tx_buffer[cmd_index++] = RX_CMD_TAIL;
    tx_buffer[cmd_index++] = READ_ALL_FILE_NAME;
    tx_buffer[cmd_index++] = 7;
    memset(&tx_buffer[cmd_index],0xff,4);
    cmd_index += 4;
    CRC_16 = CRC16(tx_buffer,7);
    tx_buffer[7] =  CRC_16&0X00FF;
    tx_buffer[8] = (CRC_16>>8);
    tx_buffer[9] = TX_CMD_TAIL;
    
    if(Send_data(tx_buffer,7+2+1) != HAL_OK)
        printf("%s ERROR\r\n",__func__);
    
    myfree(myfilename);
}


/*
    下载文件:通过文件名 读取文件数据通过串口发出
*/
void download_file(void){
    FIL dfile;
    
    uint32_t fdata_size = rx_buffer[2]-2-1;
    char *fn_buf = mymalloc(fdata_size);     //分配指定文件名长度的缓冲区
    if(fn_buf == NULL)
        printf("[%s]:malloc error\r\n",__func__);
    memcpy(fn_buf,&rx_buffer[CMD_DATA_INDEX],fdata_size);   //拷贝当前需要下载的文件名到缓冲区

    //文件名创建完成 打开文件
    ret = f_open(&dfile,fn_buf,FA_READ);
    if(ret != FR_OK){
        printf("[%s]:f_open error  -- %d\r\n",__func__,ret);
        //卸载重新挂载
        fat_error_func();
        //打开文件
        ret = f_open(&dfile,fn_buf,FA_READ);
        if(ret != FR_OK){
            printf("[%s]:f_open error -- %d\r\n",__func__,ret);
        }
    }
    
    //获取文件大小 构建第一帧 
    size_t fsize = f_size(&dfile);
//    printf("%d %s\r\n",fsize,fn_buf);
    uint32_t cmd_index = 0;
    tx_buffer[cmd_index++] = TX_CMD_HEAD;
    tx_buffer[cmd_index++] = DOWNLOAD_FILE;
    tx_buffer[cmd_index++] = 7;
    memcpy(&tx_buffer[cmd_index],&fsize,sizeof(fsize));
    cmd_index += sizeof(fsize);
    CRC_16 = CRC16(tx_buffer,7);
    tx_buffer[cmd_index++] =  CRC_16&0X00FF;
    tx_buffer[cmd_index++] = (CRC_16>>8);
    tx_buffer[cmd_index++] = TX_CMD_TAIL;
    if(Send_data(tx_buffer,cmd_index) != HAL_OK)
        printf("%s ERROR\r\n",__func__);
    
    //判断响应帧 构建数据帧
        //重新启动DMA接收
    frame_receied = 0;
    HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
    while(1){
        if(rx_buffer[0] == 0x0F){
            memset(rx_buffer,0,sizeof(rx_buffer));
            frame_receied = 0;
            HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
            break;
        }
        else if(rx_buffer[0] == 0xF0){
            Send_data(tx_buffer,10);
            memset(rx_buffer,0,sizeof(rx_buffer));
            frame_receied = 0;
            HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
        }
    }
    if(fsize != 0){
        //定义缓冲区
        char *fdatabuf = mymalloc(DOWNLOAD_FILE_BUF_SIZE);          //定义250字节缓冲区 index0-249
                          
        uint32_t offset = 0;
        int dframe_count = fsize / DOWNLOAD_FILE_BUF_SIZE + 1;     //数据帧计数
        uint32_t rcount;
        while(dframe_count > 0 && (offset<fsize)){
            //移动文件指针到读写的位置
            if(f_lseek(&dfile,offset) != FR_OK){
                printf("[%s]:f_lseek error\r\n",__func__);
                f_close(&fsrc);   //关闭文件
            }
            ret = f_read(&dfile,fdatabuf,DOWNLOAD_FILE_BUF_SIZE,&rcount);
            if(ret  != FR_OK){
                printf("f_read_failed %d\r\n",ret);
            }
            //响应帧
            cmd_index = CMD_LEN_INDEX;
            tx_buffer[cmd_index++] = DOWNLOAD_FILE_CRC_LEN;
            if(rcount < DOWNLOAD_FILE_BUF_SIZE){    //不足250字节 剩余填充0xff
                memset(&fdatabuf[rcount],0xff,DOWNLOAD_FILE_BUF_SIZE-rcount);
            }
            memcpy(&tx_buffer[cmd_index],fdatabuf,DOWNLOAD_FILE_BUF_SIZE);
            cmd_index += DOWNLOAD_FILE_BUF_SIZE;
            CRC_16 = CRC16(tx_buffer,DOWNLOAD_FILE_CRC_LEN);
            tx_buffer[cmd_index++] =  CRC_16&0X00FF;
            tx_buffer[cmd_index++] = (CRC_16>>8);
            tx_buffer[cmd_index++] = TX_CMD_TAIL;
            if(Send_data(tx_buffer,cmd_index) != HAL_OK)
                printf("%s ERROR\r\n",__func__);
            
            memset(rx_buffer,0,sizeof(rx_buffer));
            //判断响应帧 构建数据帧
                //清空接收缓冲区
            while(1){
                if(rx_buffer[0] == 0x0F){
                    dframe_count -= 1;
                    offset += rcount;       //记录文件偏移
                    frame_receied = 0;
                    HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
                    break;
                }else if(rx_buffer[0] == 0xF0){
                    Send_data(tx_buffer,255);
                    memset(rx_buffer,0,sizeof(rx_buffer));
                    frame_receied = 0;
                    HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
                }
                
            }
        }
         myfree(fdatabuf);
    }
    
    //构建尾帧
    while(1){
        if(rx_buffer[0] == 0x0F){
            memset(rx_buffer,0,sizeof(rx_buffer));
            frame_receied = 0;
            HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
            break;
        }
        else if(rx_buffer[0] == 0xF0){
            Send_data(tx_buffer,10);
            memset(rx_buffer,0,sizeof(rx_buffer));
            frame_receied = 0;
            HAL_UART_Receive_DMA(&UART1_Handler,dma_buffer,RX_BUFFER_SIZE);
        }
    }
    
    cmd_index = 0;
    tx_buffer[cmd_index++] = RX_CMD_TAIL;
    tx_buffer[cmd_index++] = DOWNLOAD_FILE;
    tx_buffer[cmd_index++] = 7;
    memset(&tx_buffer[cmd_index],0xff,4);
    cmd_index += 4;
    CRC_16 = CRC16(tx_buffer,7);
    tx_buffer[7] =  CRC_16&0X00FF;
    tx_buffer[8] = (CRC_16>>8);
    tx_buffer[9] = TX_CMD_TAIL;
    
    if(Send_data(tx_buffer,7+2+1) != HAL_OK)
        printf("%s ERROR\r\n",__func__);
    
    //关闭文件
    f_close(&dfile);
    myfree(&fn_buf);
}


/*
    删除指定文件 删除成功返回返回帧
*/
void delete_file_func(void){
    uint32_t fdata_size = rx_buffer[2]-2-1;
    char *fn_buf = mymalloc(fdata_size);     //分配指定文件名长度的缓冲区
    if(fn_buf == NULL)
        printf("[%s]:malloc error\r\n",__func__);
    memcpy(fn_buf,&rx_buffer[CMD_DATA_INDEX],fdata_size);   //拷贝当前需要下载的文件名到缓冲区

    uint8_t cmd_index = 0;
    if(DeletTheFile(fn_buf) == FR_OK){
        tx_buffer[cmd_index++] = TX_CMD_HEAD;
        tx_buffer[cmd_index++] = DELETE_FILE;
        tx_buffer[cmd_index++] = rx_buffer[2]+2+1;
        memcpy(&tx_buffer[cmd_index],fn_buf,fdata_size);
        cmd_index+=fdata_size;
        CRC_16 = CRC16(tx_buffer,rx_buffer[2]+2+1);
        tx_buffer[cmd_index++] =  CRC_16&0X00FF;
        tx_buffer[cmd_index++] = (CRC_16>>8);
        tx_buffer[cmd_index++] = TX_CMD_TAIL;
    
        if(Send_data(tx_buffer,cmd_index) != HAL_OK)
            printf("%s ERROR\r\n",__func__);
    }
    myfree(fn_buf);
}
    
/*
    删除磁盘所有文件 - 格式化磁盘
*/
void mkfs_disk_func(void){
    uint8_t cmd_index = 0;
    ret = f_mkfs_func();
    if(ret == FR_OK){
        tx_buffer[cmd_index++] = TX_CMD_HEAD;
        tx_buffer[cmd_index++] = DELETE_FILE;
        tx_buffer[cmd_index++] = 0X03;
        CRC_16 = CRC16(tx_buffer,rx_buffer[2]);
        tx_buffer[cmd_index++] =  CRC_16&0X00FF;
        tx_buffer[cmd_index++] = (CRC_16>>8);
        tx_buffer[cmd_index++] = TX_CMD_TAIL;
    
        if(Send_data(tx_buffer,cmd_index) != HAL_OK)
            printf("%s ERROR\r\n",__func__);   
    }   
}


/*
    自容模式下的参数配置
*/
void pri_auto_parameter(void){
//    uint8_t* buf = mymalloc(4);
//    //基准模式
//    pri_file.auto_wmode = svp_cmd.AUTO_PRI_MODE;
//    pri_file.pri_auto_bound = 50;
//    TIM5_Init((pri_file.pri_auto_bound*10),CLOCK_PSC);

//    //压力基准阈值
//    if(svp_cmd.AUTO_PA_VALUE == 0.0f){
//        float bb = 0.5f;
//        memcpy(buf,&bb,4);
//        pri_file.pri_auto_pa_value = 0.5f;
//        AT24C02_Write(AUTO_PRI_PA_VALUE_ADDR,buf,4);
//    }else{
//        pri_file.pri_auto_pa_value =  svp_cmd.AUTO_PA_VALUE;
//    }
//    //频率基准值
//    if(svp_cmd.AUTO_PRI_FREQ == 0){
//        uint32_t bbb = 100;
//        pri_file.pri_auto_bound = bbb;
//        memcpy(buf,&bbb,4);
//        AT24C02_Write(AUTO_PRI_FREQ_ADDR,buf,4);
//    }else{
//        pri_file.pri_auto_bound = svp_cmd.AUTO_PRI_FREQ;
//    }

//    myfree(buf);
}


void init_autothrshold(AUTO_THRESHOLD_t *autop){
    autop->now_pa = 0.0f;
    autop->last_pa = 0.0f;
}


/*
    输出数据到文件中
    (开机自检状态正常、没有指令输入)
*/

/*
    currentPressure 当前压力
    pressureStep 步长
    targetpressure 范围区间
    处于有效区间返回当前有效区间压力点  否则返回999.99
*/
float isNearPressureStep(float currentPressure,float pressureStep){
    float targetpressure;
    if(currentPressure < 0.0f){
        return 0;
    }
    //计算最近的步长倍数
    float nearstStep = roundf(currentPressure/pressureStep)*pressureStep;
    //计算允许的阈值范围
    float margin = pressureStep * 0.1f;
    //检查是否在平均范围
    if(fabsf(currentPressure - nearstStep) <= margin){
        targetpressure = nearstStep;
        return targetpressure;
    }
    targetpressure = 999.99f;
    return targetpressure;
}

uint32_t cccc;
float value;
void pri_data_to_file(void){ 
    //判断开始记录标志位
    if(pri_file.start_flag){
        //更新文件名标志位 -> 记录新文件
        if(pri_file.updata_filename_flag == 0){
            sprintf(pri_file.filename,"20%.2d%.2d%.2d_%.2d%.2d%.2d.txt",DS3231_Time.year,
                                                            DS3231_Time.mon,
                                                            DS3231_Time.date,
                                                            DS3231_Time.hour,
                                                            DS3231_Time.min,
                                                            DS3231_Time.sec
                                                );
            //使用了新文件 指针位置归0
            pri_file.file_position = 0;
            pri_file.updata_filename_flag = 1;
            
            /*打开文件*/
            //文件名创建完成 打开文件
            ret = f_open(&pri_file.file_s,pri_file.filename,FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
            if(ret != FR_OK){
                printf("[%s]:f_open error  -- %d\r\n",__func__,ret);
                //卸载重新挂载
                fat_error_func();
                //打开文件
                ret = f_open(&pri_file.file_s,pri_file.filename,FA_CREATE_ALWAYS | FA_WRITE);
                if(ret != FR_OK){
                    printf("[%s]:f_open error -- %d\r\n",__func__,ret);
                    return ;
                }
            }else{
                pri_file.wfile_status = 1;
            }
            /*
                第一次打开文件 向文件中写入文件头
                包含    声速相关系数
                        压力系数
            */
            char* head_buf = mymalloc(500);
            if(head_buf == NULL)
                printf("mymalloc error\r\n");
            
            sprintf(head_buf,"Log:\r\nPROBE_DISTANCE %f\r\nOUTLIERS_THRESHOLD %d\r\nSOUND_VELOCITY_COE_A1 %f SOUND_VELOCITY_COE_B1 %f\r\nSOUND_VELOCITY_COE_A2 %f SOUND_VELOCITY_COE_B2 %f\r\nSOUND_VELOCITY_COE_A3 %f SOUND_VELOCITY_COE_B3 %f\r\n 0- %f - %f -1600\r\nFRIST_WAVE_V %d\r\nPA_COE_A %f\r\nPA_COE_E %f\r\nPA_COE_I %f\r\nPA_COE_M %f\r\n",
                        svp_cmd.PROBE_DISTANCE,
                        svp_cmd.OUTLIERS_THRESHOLD,
                        svp_cmd.SOUND_VELOCITY_COE_A1,
                        svp_cmd.SOUND_VELOCITY_COE_B1,
                        svp_cmd.SOUND_VELOCITY_COE_A2,
                        svp_cmd.SOUND_VELOCITY_COE_B2,
                        svp_cmd.SOUND_VELOCITY_COE_A3,
                        svp_cmd.SOUND_VELOCITY_COE_B3,
                        svp_cmd.COE_2_SPOCE,
                        svp_cmd.COE_3_SPOCE,
                        svp_cmd.FRIST_WAVE_V,
                        svp_cmd.PA_COE_A,
                        svp_cmd.PA_COE_E,
                        svp_cmd.PA_COE_I,
                        svp_cmd.PA_COE_M
            );
            uint32_t index = strlen(head_buf);
            memset(&head_buf[index],0x20,500-strlen(head_buf));
            head_buf[500-1] = 0x0a;
            head_buf[500-2] = 0x0d;
            uint32_t head_count;
            //移动文件指针到读写的位置
            if(f_lseek(&pri_file.file_s,0) != FR_OK){
                printf("[%s]:f_lseek error\r\n",__func__);
                f_close(&fsrc);   //关闭文件
                return ;
            }
            /*写入到文件中*/
            ret = f_write(&pri_file.file_s, head_buf, 500, &head_count);
            if(ret != FR_OK){
                printf("f_write error %d\r\n",ret);
                f_close(&pri_file.file_s);
                return ;
            }
            //刷新缓冲区(确保数据写入存储设备)
            if(f_sync(&pri_file.file_s) != FR_OK){
                printf("f_sync error\r\n");
                f_close(&pri_file.file_s);   //关闭文件
                return ;
            }
            pri_file.file_position = strlen(head_buf);
            myfree(head_buf);
        }
        
        
        //移动文件指针到读写的位置
        if(f_lseek(&pri_file.file_s,pri_file.file_position) != FR_OK){
            printf("[%s]:f_lseek error\r\n",__func__);
            f_close(&fsrc);   //关闭文件
            return ;
        }
        
        
        
       
        //判断写入到文件中是否根据 压力变化 如果是就要达到设置阈值再写入到文件中
        if(pri_file.auto_wmode){    //1:根据压力变化
            if(nowData.pressure > 0){
                //判断当前压力 是否在平均区间 且返回当前的区间
                auto_thrshold.now_pa = isNearPressureStep(nowData.pressure,0.05);
            }
            
            //判断范围是否在阈值的±10%之内 0.05就是（0.05±0.005）
            if(auto_thrshold.now_pa != 999.99f){
                //判断当前记录有效区间是否为上一次的有效区间
                if(auto_thrshold.now_pa != auto_thrshold.last_pa){
                    //开始平均标志位
                    pri_file.pa_flag = 1;
                    auto_thrshold.buf_pa = auto_thrshold.now_pa;
                    pri_file.ave_count = 0;
                    auto_thrshold.last_pa = auto_thrshold.now_pa;
                    
                    if(pri_file.pa_flag){   //记录原始累加数据
                        pri_file.sv_ave_buf += pri_sv;
                        pri_file.temp_ave_buf += nowData.temperature;
                        pri_file.pa_ave_buf += nowData.pressure;
                        pri_file.ave_count++;
                    }
                }
               
            }else if(pri_file.pa_flag){   //离开平均有效区间并且区间不连续 有平均数据
                //平均 写入到缓冲区
                pri_file.pa_flag = 0;
                char abuf[128];
                sprintf(abuf,
                "20%.2d/%.2d/ %.2d,%.2d:%.2d:%.2d, %f, %f, %f\r\n",
                    DS3231_Time.year,
                    DS3231_Time.mon,
                    DS3231_Time.date,
                    DS3231_Time.hour,
                    DS3231_Time.min,
                    DS3231_Time.sec,
                    pri_file.sv_ave_buf/pri_file.ave_count,
                    pri_file.temp_ave_buf/pri_file.ave_count,
                    pri_file.pa_ave_buf/pri_file.ave_count
                    );
                pri_file.abuf_count = strlen(abuf);
                pri_file.write_to_file_byte_count += pri_file.abuf_count;
                memcpy(&pri_file.wfile_buf_a[pri_file.abuf_index],abuf,pri_file.abuf_count);
                //清空标志位等
                pri_file.sv_ave_buf = 0.0f;
                pri_file.temp_ave_buf = 0.0f;
                pri_file.pa_ave_buf = 0.0f;
                pri_file.ave_count = 0;
                
                if(pri_file.abuf_index ==0)
                    pri_file.abuf_index += pri_file.abuf_count;
                else
                    pri_file.abuf_index += pri_file.abuf_count;
            
                
                //判断当前缓冲区被填充数据
                if(pri_file.abuf_index  > 51){
                    __disable_irq();
                    uint32_t count;
                    ret = f_write(&pri_file.file_s, pri_file.wfile_buf_a, pri_file.abuf_index, &count);
                    if(ret != FR_OK){
                        printf("f_write error %d\r\n",ret);
                        f_close(&pri_file.file_s);
                        return ;
                    }
                    //刷新缓冲区(确保数据写入存储设备)
                    if(f_sync(&pri_file.file_s) != FR_OK){
                        printf("f_sync error\r\n");
                        f_close(&pri_file.file_s);   //关闭文件
                        return ;
                    }
                    //清空缓冲区
                    memset(pri_file.wfile_buf_a,0,FILE_DATA_BUF_SIZE);
                    //文件指针偏移
                    pri_file.file_position += pri_file.abuf_index;
                    //下标清空
                    pri_file.abuf_index = 0;
                    __enable_irq();
                }
            }
        }else{
            //      ms      50ms触发      
            //10Hz 100                      
            //2Hz  500      
            //1Hz  1000
            //count == 频率/50
            /*记录平均数据 不要0*/
            pri_file.sv_ave_buf += pri_sv;
            pri_file.temp_ave_buf += nowData.temperature;
            pri_file.pa_ave_buf += nowData.pressure;
            pri_file.ave_count++;
            pri_file.wfcount++;
            
            if(pri_file.wfcount == (pri_file.pri_auto_bound/50)){
                //计算一次平均值
                char abuf[128];
                sprintf(abuf,
                "20%.2d/%.2d/ %.2d,%.2d:%.2d:%.2d, %f, %f, %f\r\n",
                    DS3231_Time.year,
                    DS3231_Time.mon,
                    DS3231_Time.date,
                    DS3231_Time.hour,
                    DS3231_Time.min,
                    DS3231_Time.sec,
                    pri_file.sv_ave_buf/pri_file.ave_count,
                    pri_file.temp_ave_buf/pri_file.ave_count,
                    pri_file.pa_ave_buf/pri_file.ave_count);
                
                pri_file.abuf_count = strlen(abuf);
                pri_file.write_to_file_byte_count += pri_file.abuf_count;
                //将数据写入到缓冲区中
                memcpy(&pri_file.wfile_buf_a[pri_file.abuf_index],abuf,pri_file.abuf_count);
                //清空标志位等
                pri_file.sv_ave_buf = 0.0f;
                pri_file.temp_ave_buf = 0.0f;
                pri_file.pa_ave_buf = 0.0f;
                pri_file.ave_count = 0;
                pri_file.wfcount = 0;
                
                if(pri_file.abuf_index ==0)
                    pri_file.abuf_index += pri_file.abuf_count;
                else
                    pri_file.abuf_index += pri_file.abuf_count;
            }
            
            //判断当前缓冲区被填充的数据是否过半 写入数据到文件中
            if(pri_file.abuf_index  > (FILE_DATA_BUF_SIZE/2)){
                __disable_irq();
                uint32_t count;
                ret = f_write(&pri_file.file_s, pri_file.wfile_buf_a, pri_file.abuf_index, &count);
                if(ret != FR_OK){
                    printf("f_write error %d\r\n",ret);
                    f_close(&pri_file.file_s);
                    return ;
                }
                //刷新缓冲区(确保数据写入存储设备)
                if(f_sync(&pri_file.file_s) != FR_OK){
                    printf("f_sync error\r\n");
                    f_close(&pri_file.file_s);   //关闭文件
                    return ;
                }
                //清空缓冲区
                memset(pri_file.wfile_buf_a,0,FILE_DATA_BUF_SIZE);
                //文件指针偏移
                pri_file.file_position += pri_file.abuf_index;
                //下标清空
                pri_file.abuf_index = 0;
                __enable_irq();
            }
        }
        
        /*10s*/
        cccc++;
        if(cccc == 20){
            pri_file.start_flag = 0;
        }       
    }else{  //停止记录 - 离水
        //文件已经打开
        if(pri_file.wfile_status){
            
            printf("pri_file.write_to_file_byte_count %d\r\n",pri_file.write_to_file_byte_count);
            
            char rfbuf[2048];
            memset(rfbuf,0,sizeof(rfbuf));
            uint32_t rcount;
            //移动文件指针到读写的位置
            if(f_lseek(&pri_file.file_s,0) != FR_OK){
                printf("[%s]:f_lseek error\r\n",__func__);
                f_close(&fsrc);   //关闭文件
                return ;
            }
            ret = f_read(&pri_file.file_s,rfbuf,2048,&rcount);
            if(ret  != FR_OK){
                printf("f_read_failed %d\r\n",ret);
            }
            printf("%s\r\n",rfbuf);
            
            
            //关闭当前文件
            f_close(&fsrc);
            pri_file.wfile_status = 0;
        }
    }
}




