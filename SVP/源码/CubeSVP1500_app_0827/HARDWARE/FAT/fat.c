/*
		文件系统
		侯新波
		20250806
*/

#include "fat.h"
/*根文件系统*/
static FATFS fs_FLASH;
static uint8_t work_buf[512];
FIL fsrc;                                       /* File object structure (FIL) */
uint16_t f_haveFileNumber;                      /* 0:/现有文件个数文件个数		 */
FRESULT ret;									/* f_func return status */	
/*总字节数 剩余字节数 使用字节数*/
uint64_t total_bytes,free_bytes,used_bytes;	
extern volatile uint8_t wite_file_flag;
extern volatile uint8_t file_task_pending;
/*
 * TDC 采集路径可能在 TIM2 中断上下文调用开始/停止记录接口。该上下文只能提交
 * 请求，真正修改文件状态机必须由 SVP_FILE 任务串行完成。
 */
static volatile uint8_t file_start_request;
static volatile uint8_t file_stop_request;
/*
 * 记录许可由低电压保护控制。即使 TDC 仍持续得到稳定回波，许可关闭时也不能
 * 新建记录文件，避免低电压状态下出现“关闭后立刻重新打开”的循环。
 */
static volatile uint8_t file_record_enabled = 1;
/*
 * 以下辅助函数仅由文件状态机使用：统一清理记录现场，并在关闭文件前按产生顺序
 * 刷写乒乓缓冲区，避免离水瞬间丢失尚未落盘的记录。
 */
static void pri_reset_record_runtime(void);
static uint8_t pri_flush_record_buffers(void);

/*
	卸载根文件系统
*/
uint8_t Fatfs_unmount(void){
    return f_unmount(DISK_PATH);
}

/*
	FAT根文件系统错误处理
*/
void fat_error_func(void){
    Fatfs_unmount();
    Fatfs_mount();
}


static FRESULT DeleteOldedtFilesExceptCurrent(const char* path, const char* current_file,uint32_t need_bytes){
	DIR dir;
	FILINFO fno;
	FRESULT res;
	uint64_t freed_bytes = 0;
	int max_loop = 100;	//防止无限循环，最多删除100个文件
	while(freed_bytes < need_bytes && max_loop > 0){
		max_loop--;
		//找到最旧文件
		char oldset_name[256] = {0};
		WORD oldset_date = 0xffff;
		WORD oldset_time = 0xffff;
		DWORD oldset_size = 0;
		int found = 0;
		res = f_opendir(&dir,path);
		if(res != FR_OK){
			return FR_NOK;
		}
		while(1){
			res = f_readdir(&dir,&fno);
			if (res != FR_OK || fno.fname[0] == 0) break;
			/*只处理文件，跳过目录*/
			if (!(fno.fattrib & AM_DIR)){
				if(current_file != NULL && strcmp(fno.fname,current_file) == 0){
					continue;
				}
				/*比较日期 找出最旧文件*/    
				if(fno.fdate < oldset_date || (fno.fdate == oldset_date && fno.ftime < oldset_time)){
					oldset_date = fno.fdate;
					oldset_time = fno.ftime;
					oldset_size = fno.fsize;
					snprintf(oldset_name,sizeof(oldset_name),"%s/%s",path,fno.fname);
					found = 1;
				}
			}
		}
		f_closedir(&dir);
		if(!found){
			res = FR_NO_FILE;
			break;
		}
		//删除最旧文件
		printf("delete : %s(size:%d bytes,date:%u,time:%u)\r\n",oldset_name,oldset_size,oldset_date,oldset_time);
		res = f_unlink(oldset_name);
		if(res == FR_OK){
			freed_bytes += oldset_size;
		}else{
			res = FR_NOK;
			break;
		}
	}	
	//重新获取空间信息
	FLASH_FreeSize();
	return res;
}

/*
    删除工作路径下最旧文件，直到释放出足够的空间
    path 目标路径
	current_file 正在写入的文件?
    min_free_mb 需要释放的MB
*/
FRESULT free_space_by_deleting_oldest(const char* path, const char* current_file,uint32_t min_free_mb) {
	uint64_t min_free_bytes = (uint64_t)min_free_mb * 1024 * 1024,need_bytes;
	/*获取当前剩余空间*/
	FLASH_FreeSize();
	/*剩余字节free_bytes*/
	if(free_bytes >= min_free_bytes){
		printf("	Free space enough , no cleanup needed\r\n");
		return FR_OK;
	}
	/*计算需要释放的空间*/
	need_bytes = min_free_bytes - free_bytes;
	return DeleteOldedtFilesExceptCurrent(path,current_file,need_bytes);
}



 
/*
	初始化文件工程结构体
*/
void init_filestruct(Pri_File_T *p_file){
    memset(p_file,0,sizeof(Pri_File_T));
}

/*
		文件系统初始化
*/
FRESULT FLASH_Fat_Init(void){
    FRESULT ret;
    //挂载
    ret = Fatfs_mount(); 
    if(ret != FR_OK){
        return ret;    //挂载失败
    }
	//创建工作文件路径
    ret = f_mkdir(SVP_PATH);
    if (ret != FR_OK && ret != FR_EXIST) { 
        f_mount(NULL, "", 1); 
        return ret;
    }
    printf("    File storage path:%s/\r\n",SVP_PATH);
    
    //读取当前工作目录下的文件信息  文件名 修改时间 文件大小 并打印
    ret = my_scan_file(SVP_PATH); 
    if(ret != FR_OK){
        printf("    [%s]:Scan File failed.\r\n",__func__);
        return ret;
    }
		
	//计算当前系统剩余空间
    FLASH_FreeSize();
	printf("	total_bytes = %llu(%fM),free_bytes = %llu(%fM),used_bytes= %llu(%fM)\r\n",total_bytes,(float)total_bytes/(1024*1024),free_bytes,(float)free_bytes/(1024*1024),used_bytes,(float)used_bytes/(1024*1024));
  
	//检测当前剩余空间是否预留10M
	free_space_by_deleting_oldest(SVP_PATH,NULL,5);
	
    /*初始化文件结构体*/
    init_filestruct(&pri_file);
	/*以压力存储压力变化结构体*/
    PressureRecorder_Init(&recorder);
    return FR_OK;
}

/*
	格式化当前文件系统
*/
FRESULT f_mkfs_func(void){
    FRESULT mkfs_res;
    FRESULT mount_res;
    FRESULT mkdir_res;
    MKFS_PARM mkfs_opt = {
            .fmt = FM_FAT
    };

    /*
        格式化前先卸载当前卷。卷可能尚未挂载，因此卸载结果不作为
        格式化是否成功的判断依据；后续三个步骤必须全部成功。
    */
    f_mount(NULL, "0:", 0);

    /*
        不能让重新挂载的返回值覆盖格式化结果。只有 f_mkfs() 成功后，
        才继续挂载新文件系统并创建记录目录。
    */
    mkfs_res = f_mkfs("0:", &mkfs_opt, work_buf, sizeof(work_buf));
    if (mkfs_res != FR_OK) {
        printf("format failed:%d\r\n", mkfs_res);
        return mkfs_res;
    }

    /* 格式化完成后重新挂载新的文件系统。 */
    mount_res = f_mount(&fs_FLASH, "0:", 1);
    if (mount_res != FR_OK) {
        printf("mount after format failed:%d\r\n", mount_res);
        return mount_res;
    }

    /*
        新文件系统必须重新建立工作目录。FR_EXIST 按成功处理，
        兼容目录已存在的边界情况。
    */
    mkdir_res = f_mkdir(SVP_PATH);
    if (mkdir_res != FR_OK && mkdir_res != FR_EXIST) {
        printf("create storage path failed:%d\r\n", mkdir_res);
        f_mount(NULL, "0:", 0);
        return mkdir_res;
    }

    delay_ms(200);
    return FR_OK;
}
/*
    挂载文件系统
*/
FRESULT Fatfs_mount(void){
    FRESULT res_flash;
    res_flash = f_mount(&fs_FLASH, "0:", 1);
    //printf("f_mount result: %d\r\n", res_flash);
    if(res_flash == FR_NO_FILESYSTEM) {
        printf("    Formatting...\r\n");
        MKFS_PARM mkfs_opt = {
            .fmt = FM_FAT,
            .n_fat = 2,
            .align = 0,
            .au_size = 4096
        };
        res_flash = f_mkfs("0:", &mkfs_opt, work_buf, sizeof(work_buf));
        //printf("f_mkfs result: %d\r\n", res_flash);
        if(res_flash == FR_OK) {
            printf("    Format success!\r\n");
            f_mount(NULL, "0:", 0);
            f_mount(&fs_FLASH, "0:", 1);
        }
    } else if(res_flash == FR_OK) {
        printf("    Filesystem OK\r\n");
    } else {
        printf("    Mount failed: %d\r\n", res_flash);
    }
	return res_flash;
}




/*******************************************************************************
* Function Name  : uint32_t FLASH_FreeSize(void)
* Description    : 空间占用情况
* Input          : None
* Output         : None
* Return         : 返回剩余空间
* Attention      : None
*******************************************************************************/
void FLASH_FreeSize(void)
{
    FATFS *fs;
    DWORD fre_clust;
	DWORD totalSpace;
	DWORD freespace;
	FRESULT res = f_getfree("0:",&fre_clust,&fs);
	if(res != FR_OK){
		printf("get free failed\r\n");
	}
	//总簇数 = FAT表项总数 -2
	totalSpace = fs->n_fatent - 2;
	freespace = fs->csize;	//每个簇扇区数
	
	total_bytes = (uint64_t)totalSpace * freespace * 512;
	free_bytes = (uint64_t)fre_clust * freespace * 512;
	used_bytes = total_bytes - free_bytes;
	
//	printf("Total capacity: %llu bytes (%.2f MB)\r\n",total_bytes,(total_bytes/(1024.0*1024)));
//	printf("Used  capacity: %llu bytes (%.2f MB)\r\n",used_bytes,(used_bytes/(1024.0*1024)));
//	printf("Free  capacity: %llu bytes (%.2f MB)\r\n",free_bytes,(free_bytes/(1024.0*1024)));
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
    FRESULT ret;  // 添加 ret 变量定义
//    DIR dirs; 
	//FILINFO fno;
	char path[30] = {0};
	//printf("%s",filename);
	sprintf(path,"%s/%s",SVP_PATH,filename);
	//printf("%s",path);
    ret = f_unlink(path);
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
    ret =  f_opendir(&dir,path);
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
            //printf("	[%s]:|--[F]%s/%s %s %dbytes.\r\n",__func__,SVP_PATH,fno.fname,time_buf,fno.fsize);
            f_haveFileNumber++;
        }
    }
    f_closedir(&dir);
    return ret;
}


/*将当前时间返回为字符类型*/
char* file_name_time(void){
    static char rtcfilename[32];
    sprintf(rtcfilename,"20%.2d%.2d%.2d_%.2d%.2d%.2d.txt",
                            DS3231_Time.year,DS3231_Time.mon,DS3231_Time.date,DS3231_Time.hour,DS3231_Time.min,DS3231_Time.sec);
    return rtcfilename;
}



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
    //计算允许的阈值范围 10%
    float margin = pressureStep * 0.1f;
    //检查是否在平均范围
    if(fabsf(currentPressure - nearstStep) <= margin){
        targetpressure = nearstStep;
        return targetpressure;
    }
    targetpressure = 999.99f;
    return targetpressure;
}



/*
 * 文件已经关闭或尚未打开时调用。该函数只复位记录运行状态，不修改自容模式的
 * 采样参数，也不清除 TDC 的入水稳定判据，确保再次稳定入水后可以重新建文件。
 */
static void pri_reset_record_runtime(void){
    pri_file.wfile_status = 0;
    memset(pri_file.wfile_buf_a, 0, sizeof(pri_file.wfile_buf_a));
    memset(pri_file.wfile_buf_b, 0, sizeof(pri_file.wfile_buf_b));
    pri_file.file_position = 0;
    pri_file.data_ok_flag = 0;
    pri_file.currnet_num = 0;
    pri_file.abuf_index = 0;
    pri_file.bbuf_index = 0;
    pri_file.write_to_file_byte_count_a = 0;
    pri_file.write_to_file_byte_count_b = 0;
    pri_file.buf_count = 0;
    pri_file.ave_count = 0;
    pri_file.wfcount = 0;
    wite_file_flag = 0;
    PressureRecorder_Init(&recorder);
    pri_file.file_state = STATE_FILE_IDLE;
}

/*首次创建 向文件中写入文件头 - 1024byte*/
uint8_t frist_create_file(void){
    uint32_t head_log_len = 1024;
    char s_sn[9];
    s_sn[8] = '\0';
    char t_sn[9];
    t_sn[8] = '\0';
    char d_sn[9];
    d_sn[8] = '\0';
    strncpy(s_sn,svp_cmd.SV_SN_BUF,8);
    strncpy(t_sn,svp_cmd.TEMP_SN_BUF,8);
    strncpy(d_sn,svp_cmd.PRESSURE_SN_BUF,8);
    
    char head_buf[1024];
    int header_length = snprintf(head_buf, sizeof(head_buf),
        "[Header]\r\n"
            "Data=20%d-%d-%d\r\n"                   //当前日期
            "Time=%d:%d:%d\r\n"                     //当前时间
            "Model=%d(1:Depth,0:Freq)\r\n"          //采样模式
            "SamplingValue=%f/%d\r\n"               //采样频率/压力值
            "FW=%s\r\n"                             //固件版本
            "SerialNumber=%s\r\n"                   //设备序列号
            "TotslMrmory=128MB\r\n"               //总内存
            "MemoryFree=%lluKB\r\n"                   //剩余内存
            "CurrentPower=%.2f%%\r\n"				//当前电量
            "Voltage=%.2f\r\n"						//当前电压
            "Current=%.2f\r\n"						//当前电流
        "[Temperature]\r\n"           //温度
            "T_COE_A=%f\r\n"                        //系数
            "T_COE_B=%f\r\n"                        //系数
            "T_COE_C=%f\r\n"                        //系数
            "T_COE_D=%f\r\n"                        //系数
            "T_SensorNUM=%s\r\n"                    //温度传感器编号
        "[SoundVelocity]\r\n"         //声速                   
            "SV_PL=%f\r\n"                          //声速-探头长度
            "SV_IWT=%d\r\n"                         //声速-出入水阈值
            "SV_COE_A1=%f,B1=%f\r\n"                //系数
            "SV_COE_A2=%f,B2=%f\r\n"                //系数
            "SV_COE_A3=%f,B3=%f\r\n"                //系数
            "SV_MR=0.0-%f-%f-2000m/s\r\n"           //系数应用范围
            "SV_FWV=%d\r\n"                         //声速第一波电压
            "SV_SensorNUM=%s\r\n"                   //声速传感器编号
        "[Depth]\r\n"                 //压力      
            "D_COE_A=%f\r\n"                        //系数
            "D_COE_E=%f\r\n"                        //系数
            "D_COE_I=%f\r\n"                        //系数
            "D_COE_M=%f\r\n"                        //系数
            "D_SensorNUM=%s\r\n"                    //压力传感器编号
        "[END]\r\n"
        "\r\n"
        "Output Format:\r\n"
        "   Data Time SV,Depth,Temperature\r\n",
        DS3231_Time.year,
        DS3231_Time.mon,
        DS3231_Time.date,
        DS3231_Time.hour,
        DS3231_Time.min,
        DS3231_Time.sec,
        pri_file.auto_wmode,
        (float)svp_cmd.WORK_VALUE,
		svp_cmd.WORK_VALUE,
        FW_APP,
        svp_cmd.SN,
        free_bytes/1024,
        SOC,
		voltage,
		current,
        svp_cmd.TEMP_COE_A,
        svp_cmd.TEMP_COE_B,
        svp_cmd.TEMP_COE_C,
        svp_cmd.TEMP_COE_D,
        t_sn,
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
        s_sn,
        svp_cmd.PA_COE_A,
        svp_cmd.PA_COE_E,
        svp_cmd.PA_COE_I,
        svp_cmd.PA_COE_M,
        d_sn
    );
    if(header_length < 0 || header_length >= (int)sizeof(head_buf)){
        return 0;
    }
    uint32_t index = (uint32_t)header_length;
    memset(&head_buf[index],0x20,head_log_len-index);
    head_buf[1024-1] = 0x0a;
    head_buf[1024-2] = 0x0d;
    uint32_t head_count;
    //移动文件指针到读写的位置
    if(f_lseek(&pri_file.file_s,0) != FR_OK){
        f_close(&pri_file.file_s);   //关闭文件
        return 0;
    }
    /*写入到文件中*/
    ret = f_write(&pri_file.file_s, head_buf, head_log_len, &head_count);
    if(ret != FR_OK || head_count != head_log_len){
        f_close(&pri_file.file_s);
        return 0;
    }
    //刷新缓冲区(确保数据写入存储设备)
    if(f_sync(&pri_file.file_s) != FR_OK){
        f_close(&pri_file.file_s);   //关闭文件
        return 0;
    }

    /* 文件头已经占用 1024 字节，后续采样数据必须从其末尾继续写入。 */
    pri_file.file_position = head_count;
    return 1;
}

/*
 * 向当前记录文件写入一段完整缓冲区。返回 0 表示底层写入失败；调用者必须停止
 * 当前状态机步骤，因为本函数已经完成文件关闭和运行状态复位。
 */
uint8_t w_data_to_file(char *buf,uint32_t count,uint8_t bufflag){
    UINT rcount = 0;
    FRESULT wret;

    (void)bufflag;
    if(buf == NULL || count == 0){
        return 1;
    }
    wret = f_write(&pri_file.file_s, buf, count, &rcount);
    if(wret != FR_OK || rcount != count){
        pri_file.wf_flag = 1;
        f_sync(&pri_file.file_s);
        f_close(&pri_file.file_s);
        pri_reset_record_runtime();
        return 0;
    }

    //文件指针偏移
    pri_file.file_position += rcount;
    return 1;
}

/*
 * 离水或模式切换需要关闭文件时调用。先刷写已经填满、等待写入的缓冲区，再刷写
 * 当前仍在填充的缓冲区；写入顺序严格按数据产生顺序保持。
 */
static uint8_t pri_flush_record_buffers(void){
    uint32_t write_len;

    if(pri_file.data_ok_flag == 1){
        write_len = pri_file.write_to_file_byte_count_a;
        if(write_len > FILE_DATA_BUF_SIZE) write_len = FILE_DATA_BUF_SIZE;
        if(write_len != 0 && !w_data_to_file(pri_file.wfile_buf_a, write_len, 1)) return 0;
        write_len = pri_file.bbuf_index;
        if(write_len > FILE_DATA_BUF_SIZE) write_len = FILE_DATA_BUF_SIZE;
        if(write_len != 0 && !w_data_to_file(pri_file.wfile_buf_b, write_len, 2)) return 0;
    }else if(pri_file.data_ok_flag == 2){
        write_len = pri_file.write_to_file_byte_count_b;
        if(write_len > FILE_DATA_BUF_SIZE) write_len = FILE_DATA_BUF_SIZE;
        if(write_len != 0 && !w_data_to_file(pri_file.wfile_buf_b, write_len, 2)) return 0;
        write_len = pri_file.abuf_index;
        if(write_len > FILE_DATA_BUF_SIZE) write_len = FILE_DATA_BUF_SIZE;
        if(write_len != 0 && !w_data_to_file(pri_file.wfile_buf_a, write_len, 1)) return 0;
    }else if(pri_file.currnet_num == 2){
        write_len = pri_file.bbuf_index;
        if(write_len > FILE_DATA_BUF_SIZE) write_len = FILE_DATA_BUF_SIZE;
        if(write_len != 0 && !w_data_to_file(pri_file.wfile_buf_b, write_len, 2)) return 0;
    }else{
        write_len = pri_file.abuf_index;
        if(write_len > FILE_DATA_BUF_SIZE) write_len = FILE_DATA_BUF_SIZE;
        if(write_len != 0 && !w_data_to_file(pri_file.wfile_buf_a, write_len, 1)) return 0;
    }

    return 1;
}

/*压力变化结构体初始化*/
void PressureRecorder_Init(PressureRecorder *recorder) {
    recorder->last_recorded_depth = 0.0f;
    recorder->is_initialized = 1;
    recorder->noise_thread = 0.005f;
}


/*以压力变化记录压力到文件*/
void ProcessPressureSample(PressureRecorder *recorder, float new_pressure,float step) {
    char wbuf[128];
    DS3231_Read_All();
    DS3231_Read_Time();
    //检查参数有效性
    if(recorder == NULL || !recorder->is_initialized){
        /*记录数据*/
        sprintf(wbuf,
         "20%.2d/%.2d/%.2d %.2d:%.2d:%.2d,%8.3f,%8.3f,%8.3f\r\n",
                DS3231_Time.year,
                DS3231_Time.mon,
                DS3231_Time.date,
                DS3231_Time.hour,
                DS3231_Time.min,
                DS3231_Time.sec,
                0.0f,
                0.0f,
                0.0f
        );
    }else{
        /*记录数据*/
        sprintf(wbuf,
         "20%.2d/%.2d/%.2d %.2d:%.2d:%.2d,%8.3f,%8.3f,%8.3f\r\n",
                DS3231_Time.year,
                DS3231_Time.mon,
                DS3231_Time.date,
                DS3231_Time.hour,
                DS3231_Time.min,
                DS3231_Time.sec,
                pri_sv,
                new_pressure,
                PT100_TEMP
        );
    }
    //计算当前与上一次深度差
    float depth_diff = fabs(new_pressure - recorder->last_recorded_depth);
    
    //噪声过滤(小波动直接return)
    if(depth_diff < recorder->noise_thread){
        return;
    }
    
    if(depth_diff >= step){
        /*更新上一次深度数据*/
        recorder->last_recorded_depth = new_pressure;
        /*记录当前深度数据*/
        pri_file.buf_count = strlen(wbuf);
        //判断a b缓冲区是否在使用，如果都未使用，那么优先使用a缓冲区
        if(pri_file.currnet_num == 0 || pri_file.currnet_num == 1){
            //判断当前缓冲区剩余空间是否能将当前wbuf中的数据写入完
            uint32_t abcount = FILE_DATA_BUF_SIZE - pri_file.abuf_index;
            if(abcount < pri_file.buf_count){
				//记录A缓冲区有效长度
				pri_file.write_to_file_byte_count_a = pri_file.abuf_index;
				//将数据写入到b缓冲区
				memcpy(&pri_file.wfile_buf_b[pri_file.bbuf_index],wbuf,pri_file.buf_count);
				//更新b缓冲区下标
				pri_file.bbuf_index += pri_file.buf_count;
				pri_file.abuf_index = 0;
				//文件写入标志位置1 
				pri_file.data_ok_flag = 1;
				pri_file.currnet_num = 2;
            }else{//缓冲区剩余空间足够
				//将数据写入到缓冲区中
				//printf("time 5 p irq a\r\n");
				memcpy(&pri_file.wfile_buf_a[pri_file.abuf_index],wbuf,pri_file.buf_count);   
				//abuf下标增加
				pri_file.abuf_index += pri_file.buf_count;
            }
        }else if(pri_file.currnet_num == 2){    //使用B缓冲区填充数据
            //判断当前缓冲区剩余空间是否能将当前buf中的数据写入完
            uint32_t bbcount = FILE_DATA_BUF_SIZE - pri_file.bbuf_index;
            if(bbcount < pri_file.buf_count){
				//printf("time 5 p irq b\r\n");
				//printf("%d\r\n%d\r\n %s\r\n",pri_file.bbuf_index,strlen(pri_file.wfile_buf_b),pri_file.wfile_buf_b);
				//记录B缓冲区有效长度
				pri_file.write_to_file_byte_count_b = pri_file.bbuf_index;
				//将数据写入到a缓冲区
				memcpy(&pri_file.wfile_buf_a[pri_file.abuf_index],wbuf,pri_file.buf_count);
				//更新a缓冲区下标
				pri_file.abuf_index += pri_file.buf_count;
				pri_file.bbuf_index = 0;
				//文件写入标志位置1 
				pri_file.data_ok_flag = 2;
				pri_file.currnet_num = 1;
            }else{//缓冲区剩余空间足够
				//printf("time 5 p irq b\r\n");
				//将数据写入到缓冲区中
				memcpy(&pri_file.wfile_buf_b[pri_file.bbuf_index],wbuf,pri_file.buf_count);  
				//bbuf下标增加
				pri_file.bbuf_index += pri_file.buf_count;
            }
        }
    }
}


float pa_pbuf = 0.0f;
//以频率模式记录数据
void freq_record(void){
    pri_file.ave_count++;
    pri_file.wfcount++;
    //如果当前到达频率记录时间 - 填充缓冲区
    if(pri_file.wfcount == (pri_file.pri_auto_bound/100)){  
        char wbuf[128];
        //获取RTC时间
        DS3231_Read_All();;
        DS3231_Read_Time();
        sprintf(wbuf,
         "20%.2d/%.2d/%.2d %.2d:%.2d:%.2d,%8.3f,%8.3f,%8.3f\r\n",
                DS3231_Time.year,
                DS3231_Time.mon,
                DS3231_Time.date,
                DS3231_Time.hour,  
                DS3231_Time.min,
                DS3231_Time.sec,
                pri_sv,
                Depth,
                PT100_TEMP
        );
        //printf("%s\r\n",wbuf);
        pri_file.buf_count = strlen(wbuf);    
        
        //判断a b缓冲区是否在使用，如果都未使用，那么优先使用a缓冲区
        if(pri_file.currnet_num == 0 || pri_file.currnet_num == 1){
            //判断当前缓冲区剩余空间是否能将当前wbuf中的数据写入完
            uint32_t abcount = FILE_DATA_BUF_SIZE - pri_file.abuf_index;
            if(abcount < pri_file.buf_count){
                //printf("%d\r\n%d\r\n %s\r\n",pri_file.abuf_index,strlen(pri_file.wfile_buf_a),pri_file.wfile_buf_a);
                //记录A缓冲区有效长度
                pri_file.write_to_file_byte_count_a = pri_file.abuf_index;
                //将数据写入到b缓冲区
                memcpy(&pri_file.wfile_buf_b[pri_file.bbuf_index],wbuf,pri_file.buf_count);
                //更新b缓冲区下标
                pri_file.bbuf_index += pri_file.buf_count;
                pri_file.abuf_index = 0;
                //文件写入标志位置1 
                pri_file.data_ok_flag = 1;
                pri_file.currnet_num = 2;
                    
            }else{//缓冲区剩余空间足够
                //将数据写入到缓冲区中
                //printf("time 5 p irq a\r\n");
                memcpy(&pri_file.wfile_buf_a[pri_file.abuf_index],wbuf,pri_file.buf_count);   
                //abuf下标增加
                pri_file.abuf_index += pri_file.buf_count;
            }
        }else if(pri_file.currnet_num == 2){    //使用B缓冲区填充数据
            //判断当前缓冲区剩余空间是否能将当前buf中的数据写入完
            uint32_t bbcount = FILE_DATA_BUF_SIZE - pri_file.bbuf_index;
            if(bbcount < pri_file.buf_count){
                //printf("time 5 p irq b\r\n");
                //printf("%d\r\n%d\r\n %s\r\n",pri_file.bbuf_index,strlen(pri_file.wfile_buf_b),pri_file.wfile_buf_b);
                //记录B缓冲区有效长度
                pri_file.write_to_file_byte_count_b = pri_file.bbuf_index;
                //将数据写入到a缓冲区
                memcpy(&pri_file.wfile_buf_a[pri_file.abuf_index],wbuf,pri_file.buf_count);
                //更新a缓冲区下标
                pri_file.abuf_index += pri_file.buf_count;
                pri_file.bbuf_index = 0;
                //文件写入标志位置1 
                pri_file.data_ok_flag = 2;
                pri_file.currnet_num = 1;
            }else{//缓冲区剩余空间足够
                //printf("time 5 p irq b\r\n");
                //将数据写入到缓冲区中
                memcpy(&pri_file.wfile_buf_b[pri_file.bbuf_index],wbuf,pri_file.buf_count);  
                //bbuf下标增加
                pri_file.bbuf_index += pri_file.buf_count;
            }
        }
        //清空标志位等
        pri_file.ave_count = 0;
        pri_file.wfcount = 0;       
        memset(wbuf,0,sizeof(wbuf));
    }    
}


/*自容模式记录数据到文件*/
void pri_data_to_file(void){ 
	char  wfile_buf[FILE_DATA_BUF_SIZE] = {0};         	//文件写缓冲区
	uint32_t write_len = 0;
	//判断状态机
	switch(pri_file.file_state){	//无需记录文件
		case STATE_FILE_IDLE:
			//什么都不做
			break;
		case STATE_READLY:
			DS3231_Read_All();
			DS3231_Read_Time();
			sprintf(pri_file.filename,"%s/20%.2d%.2d%.2d_%.2d%.2d%.2d.txt",
			SVP_PATH,DS3231_Time.year,
			DS3231_Time.mon,
			DS3231_Time.date,
			DS3231_Time.hour,
			DS3231_Time.min,
			DS3231_Time.sec );
			//文件指针位置归0
            pri_file.file_position = 0;
			//检查剩余文件空间
            FLASH_FreeSize();
			/*比对剩余空间与阈值剩余空间*/
            if(free_bytes < ((uint64_t)REQUIRED_SPACE * 1024 * 1024)){   
                //printf("[%s]:There is not enough space ，Clear up the early files.\r\n",__func__);
                free_space_by_deleting_oldest(SVP_PATH,NULL,REQUIRED_SPACE); //释放空间
            }
			/*创建文件*/
            ret = f_open(&pri_file.file_s,pri_file.filename,FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
            if(ret != FR_OK){
				//printf("f_open(file) - ret%d\r\n",ret);
                //卸载重新挂载
                fat_error_func();
                //打开文件
                ret = f_open(&pri_file.file_s,pri_file.filename,FA_CREATE_ALWAYS | FA_WRITE);
                if(ret != FR_OK){
                    /* 创建失败后完整复位，下一次稳定入水仍可重新发起建文件。 */
                    pri_file.wf_flag = 1;
                    pri_reset_record_runtime();
                    break;
                }
            }
			//文件已成功创建并打开
            pri_file.wfile_status = 1;
			//写入文件头；失败时不能进入记录状态，避免后续向已关闭句柄写数据。
            if(!frist_create_file()){
                pri_file.wf_flag = 1;
                pri_reset_record_runtime();
                break;
            }
			//文件头完成且文件指针已定位到头部末尾，再允许写入采样记录。
			pri_file.file_state = STATE_WRITEING_FILE_DATA;
			break;
		case STATE_WRITEING_FILE_DATA:
			//移动文件指针到读写的位置
			if(f_lseek(&pri_file.file_s,pri_file.file_position) != FR_OK){
				f_close(&pri_file.file_s);   //关闭文件
                pri_file.wf_flag = 1;
                pri_reset_record_runtime();
				break ;
			}
			/**********************************************************************************************************************/       
			//判断写入到文件中是否根据 压力变化 如果是就要达到设置阈值再写入到文件中
			if(pri_file.auto_wmode){    
				ProcessPressureSample(&recorder,Depth,pri_file.pri_auto_pa_value*100);  //1:根据深度变化
			}else{  
				freq_record();	 														//0:频率写入文件
			}    
			/**********************************************************************************************************************/	
			//文件写入操作	
            switch(pri_file.data_ok_flag){
                case 1:
                    pri_file.data_ok_flag= 0;
                    buf_num = 1;
                    write_len = pri_file.write_to_file_byte_count_a;
                    if(write_len > FILE_DATA_BUF_SIZE) write_len = FILE_DATA_BUF_SIZE;
                    memcpy(wfile_buf,pri_file.wfile_buf_a,write_len);
                    memset(pri_file.wfile_buf_a,0,sizeof(pri_file.wfile_buf_a));
                    pri_file.write_to_file_byte_count_a = 0;
                    break;
                case 2:
                    pri_file.data_ok_flag= 0;
                    buf_num = 2;
                    write_len = pri_file.write_to_file_byte_count_b;
                    if(write_len > FILE_DATA_BUF_SIZE) write_len = FILE_DATA_BUF_SIZE;
                    memcpy(wfile_buf,pri_file.wfile_buf_b,write_len);
                    memset(pri_file.wfile_buf_b,0,sizeof(pri_file.wfile_buf_b));
                    pri_file.write_to_file_byte_count_b = 0;
                    break;
                default:
                    break;
            }
            //根据标志位,将缓冲区数据写入文件中
            if(write_len != 0){
                if(!w_data_to_file(wfile_buf,write_len,buf_num)){
                    break;
                }
                memset(wfile_buf,0,sizeof(wfile_buf));
            }
			break;
		case STATE_FILE_CLOSING:
			if(pri_file.wfile_status){
				//移动文件指针到读写的位置
				if(f_lseek(&pri_file.file_s,pri_file.file_position) != FR_OK){
					f_close(&pri_file.file_s);   //关闭文件
                    pri_file.wf_flag = 1;
                    pri_reset_record_runtime();
					break ;
				}

                /* 先写满缓冲区，再写当前缓冲区，不能使用 else-if 漏掉其中一块。 */
                if(!pri_flush_record_buffers()){
                    break;
                }

				//刷新缓冲区(确保数据写入存储设备)
                if(f_sync(&pri_file.file_s) != FR_OK){
                    pri_file.wf_flag = 1;
                }
				//关闭文件
                if(f_close(&pri_file.file_s) != FR_OK){
                    pri_file.wf_flag = 1;
                }
			}
            pri_reset_record_runtime();
			break;
		default:
			break;
    }
}

Filewritedata_State_t file_state_return(void){
	return pri_file.file_state;
}

/*
 * 仅由 SVP_FILE 任务调用：真正执行开始记录的状态迁移。
 */
static void pri_start_record_apply(void){
	if(pri_file.file_state == STATE_FILE_IDLE){
		//printf("start write file \r\n");
        /* 新一轮稳定入水记录开始前清除上一次写入错误状态。 */
        pri_file.wf_flag = 0;
		pri_file.file_state = STATE_READLY;
	}
}

/*
 * 仅由 SVP_FILE 任务调用：真正执行停止记录的状态迁移和文件关闭请求。
 */
static void pri_stop_record_apply(void){
    if(pri_file.file_state == STATE_WRITEING_FILE_DATA ){
        //printf("close write file \r\n");
        pri_file.file_state = STATE_FILE_CLOSING;
    }else if(pri_file.file_state == STATE_READLY){
        /* 文件尚未真正创建，直接取消本次稳定入水的待建文件请求。 */
        pri_reset_record_runtime();
    }
}

/*
 * 可由 TDC 的 TIM2 采集路径或电源检测路径调用。这里只置位请求，避免与
 * SVP_FILE 任务同时改写 pri_file.file_state。
 */
void pri_start_record(void){
    /*
     * 显控连接后，本次上电禁止自容记录。
     * 即使 TDC 后续再次判断到稳定入水，也不能重新提交建文件请求。
     */
    if((file_record_enabled == 0) || (workmode_flag != 0U)){
        return;
    }
    file_start_request = 1;
    file_task_pending = 1;
}

/*
 * 低电压保护调用本接口关闭记录许可。关闭许可时同时提交停止请求；恢复许可后
 * 不会直接建文件，仍必须由 TDC 的稳定入水条件重新发起开始记录请求。
 */
void pri_set_record_enable(uint8_t enable){
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();
    file_record_enabled = (enable != 0) ? 1 : 0;
    if(file_record_enabled == 0){
        file_start_request = 0;
        file_stop_request = 1;
        file_task_pending = 1;
    }
    __set_PRIMASK(primask);
}

/*
 * 停止请求优先级高于开始请求。离水、低电压等情形到来后，文件任务会先关闭或
 * 取消待创建的文件，避免已经离水后仍开始记录。
 */
void pri_stop_record(void){
    file_stop_request = 1;
    file_task_pending = 1;
}

/**
 * @brief 处理文件记录请求。该函数应在 SVP_FILE 任务中被调用，以确保对文件记录状态的修改是线程安全的。
 *        当有停止请求时，优先处理停止请求，取消任何未处理的开始请求。否则，如果有开始请求且记录功能已启用，则处理开始请求。
 *        在处理请求时，短暂禁用中断以原子地获取当前请求位，防止在处理请求期间有新的请求被提交。
 *        恢复中断后，任何新的请求将保留到下一次文件任务执行，不会丢失。
*/
void pri_process_record_requests(void){
    uint8_t start_request;
    uint8_t stop_request;
    uint32_t primask;

    /*
     * TDC 的 TIM2 路径可能在本函数运行期间提交新请求。这里只短暂关闭中断，原子地
     * 取走当前请求位；恢复中断后的新请求会保留到下一次文件任务执行，不会丢失。
     */
    primask = __get_PRIMASK();
    __disable_irq();
    start_request = file_start_request;
    stop_request = file_stop_request;
    file_start_request = 0;
    file_stop_request = 0;
    __set_PRIMASK(primask);

    if(stop_request != 0){
        /* 停止优先于开始，离水或模式切换后不会继续创建新文件。 */
        pri_stop_record_apply();
    }else if((start_request != 0) &&
             (file_record_enabled != 0) &&
             (workmode_flag == 0U)){
        pri_start_record_apply();
    }
}

/*
 * 供模式切换、格式化、下载等串口文件命令调用。调用者已持有 I2C1 互斥锁，因而
 * 与 SVP_FILE 任务串行；这里还会清除尚未处理的入水请求，避免切换模式后误建文件。
 */
void pri_flush_record_now(void){
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();
    file_start_request = 0;
    file_stop_request = 0;
    __set_PRIMASK(primask);

    if(pri_file.file_state == STATE_WRITEING_FILE_DATA){
        pri_file.file_state = STATE_FILE_CLOSING;
    }
    if(pri_file.file_state == STATE_FILE_CLOSING){
        pri_data_to_file();
    }else if(pri_file.file_state == STATE_READLY){
        pri_reset_record_runtime();
    }else{
        wite_file_flag = 0;
    }
}


