// file_fsm.c
#include "file_fsm.h"
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"


// ==================== 静态上下文 ====================
static FileManagerContext ctx = {(FileManagerState)0,0};

// 文件协议响应的公共发送缓冲区。
static uint8_t send_buffer[4 + 4 + MAX_PACKET_SIZE + 10];

// ==================== 内部函数声明 ====================
static void send_file_list_response(void);
static void send_total_packets(uint32_t total_packets);
static void send_data_packet(uint32_t packet_num, uint8_t *data, uint32_t data_len);
static void send_error_response(uint8_t cmd, uint8_t error_code);
static void send_delete_response(uint8_t cmd, uint8_t *data, uint16_t data_len);
static void send_format_response(void);
static void send_batch_delete_response(uint32_t deleted_count);
// 文件命令处理函数。
static void handle_read_file_list(void);
static void handle_download_file(uint8_t *buffer);
static void handle_delete_file(uint8_t *filename_data);
static void handle_format(void);
static void handle_batch_delete(uint8_t *data, uint16_t data_len);
static void restart_uart_dma_receive(void);

/*
 * 下载交互期间，串口空闲中断会停止 DMA 接收。
 * 对于不回复数据的非法下载帧，必须主动重新开启 DMA；
 * 否则显控后续发送的合法帧无法再进入接收缓冲区。
 */
static void restart_uart_dma_receive(void) {
    frame_receied = 0;
    rx_length = 0;
    HAL_UART_Receive_DMA(&UART1_Handler, dma_buffer, RX_BUFFER_SIZE);
}

// ==================== 文件协议响应发送 ====================
void send_response_file(uint8_t cmd, uint8_t *data, uint16_t data_len) {
    int idx = 0;
    int crc_len = data_len + 3;
    send_buffer[idx++] = FRAME_HEAD_RSP;        // 响应帧头：0x55
    send_buffer[idx++] = cmd;                   // 文件命令码
    /* 长度字段：data 区长度加 CRC16 和帧尾长度。 */
    send_buffer[idx++] = crc_len & 0xFF;
    send_buffer[idx++] = (crc_len >> 8) & 0xFF;
    if (data && data_len > 0) {
        memcpy(send_buffer + idx, data, data_len);
        idx += data_len;
    }
    // 按现有文件协议计算 CRC16。
    CRC_16 = CRC16(send_buffer,crc_len+1);
    send_buffer[idx++] = CRC_16&0X00FF;
    send_buffer[idx++] = (CRC_16>>8);
    send_buffer[idx++] = FRAME_TAIL_RSP;        // 响应帧尾：0xAA
	
    /* 直读模式下暂时暂停实时数据输出，防止与文件响应帧交叉。 */
	if(work_status() == 1){
		pri_enable_flag = 1;
	}
	
    // 阻塞发送完整文件协议响应帧。
    if(Send_data(send_buffer,idx) != HAL_OK)
            printf("%s ERROR\r\n",__func__);
	
    // 发送结束后清除旧命令数据并恢复 DMA 接收。
    memset(rx_buffer,0,sizeof(rx_buffer));
    restart_uart_dma_receive();

	if(work_status() == 1){
		pri_enable_flag = 0;
	}
}

static int read_dir_file(){
	DIR dir1;
    FILINFO fno1;
    FRESULT res1;
	int filecount1 = 0;
	res1 = f_opendir(&dir1, SVP_PATH);
	while(1) {
		res1 = f_readdir(&dir1, &fno1);
        /* 调试时可在这里输出目录读取状态。 */
        if (res1 != FR_OK || fno1.fname[0] == 0) {
            /* 目录读取失败或到达目录末尾时结束统计。 */
			break;
		}
        if (fno1.fname[0] == '.') continue;
		filecount1++;
		
	}
    /* 调试时可在这里输出统计出的文件数量。 */
	f_closedir(&dir1);
	return filecount1;
}

/*
    读取当前系统文件。每个回复包最多携带 40 个文件。

    文件列表回复的 data 区格式固定如下：
    [4字节：目录中的文件总数，小端]
    [4字节：当前回复包中的文件数量，小端]
    [N个文件记录：19字节文件名 + 4字节文件大小]

    send_response_file() 负责在上述 data 区前后添加协议头、命令码、长度、CRC 和帧尾。
 */
static void send_file_list_response(void) {
    DIR dir;
    FILINFO fno;
    FRESULT res;
    uint8_t response_data[(FILENAME_LEN + 4) * MAX_FILES_PER_PACKET];
    uint8_t data_buf[sizeof(response_data) + 8];
    uint16_t data_idx = 0U;
    uint32_t file_count = 0U;
    uint32_t filecount = 0U;
    uint32_t start_tick;

    /* 打开目录。文件系统短暂忙碌时允许重试，但不能无限阻塞命令任务。 */
    start_tick = get_current_tick();
    do {
        res = f_opendir(&dir, SVP_PATH);
        if (res == FR_OK) {
            break;
        }

        if (is_timeout(start_tick, FILE_LIST_TIMEOUT_MS)) {
            printf("open dir timeout %d\r\n", res);
            send_error_response(CMD_READ_FILE_LIST, STATUS_READ_ERROR);
            return;
        }
        HAL_Delay(10);
    } while (1);

    /*
     * 协议需要先给出目录文件总数，因此先独立扫描一次目录。
     * 正式读取时仍从当前 dir 的起始位置开始，不会遗漏第一个文件。
     */
    filecount = (uint32_t)read_dir_file();
    start_tick = get_current_tick();

    while (1) {
        /* 读取目录也必须受超时保护，防止文件系统异常时卡住文件命令处理。 */
        if (is_timeout(start_tick, FILE_LIST_TIMEOUT_MS)) {
            break;
        }

        res = f_readdir(&dir, &fno);
        if ((res != FR_OK) || (fno.fname[0] == 0)) {
            break;
        }
        if (fno.fname[0] == '.') {
            continue;
        }

        /*
         * 单个文件记录固定为 19 字节文件名和 4 字节文件大小。
         * 缓冲区上限与 MAX_FILES_PER_PACKET 对应；此保护用于防止宏被修改后溢出。
         */
        if ((data_idx + FILENAME_LEN + 4U) > sizeof(response_data)) {
            break;
        }

        memset(response_data + data_idx, 0, FILENAME_LEN);
        strncpy((char *)(response_data + data_idx), fno.fname, FILENAME_LEN);
        data_idx += FILENAME_LEN;

        response_data[data_idx++] = (uint8_t)(fno.fsize & 0xFFU);
        response_data[data_idx++] = (uint8_t)((fno.fsize >> 8) & 0xFFU);
        response_data[data_idx++] = (uint8_t)((fno.fsize >> 16) & 0xFFU);
        response_data[data_idx++] = (uint8_t)((fno.fsize >> 24) & 0xFFU);
        file_count++;

        if (file_count >= MAX_FILES_PER_PACKET) {
            /*
             * data_idx 只表示文件记录长度，绝不能加上前 8 字节协议字段后
             * 再拿去复制 response_data；否则会从 response_data 越界读取。
             */
            memcpy(&data_buf[0], &filecount, sizeof(filecount));
            memcpy(&data_buf[4], &file_count, sizeof(file_count));
            memcpy(&data_buf[8], response_data, data_idx);
            send_response_file(CMD_READ_FILE_LIST, data_buf, (uint16_t)(data_idx + 8U));

            data_idx = 0U;
            file_count = 0U;
            HAL_Delay(500);
            start_tick = get_current_tick();
        }
    }

    f_closedir(&dir);

    /*
     * 发送最后一个不足 40 个文件的包。
     * 当目录为空时也回复一个“总数=0、本包数=0”的列表包；
     * 当文件数量刚好为 40 的整数倍时 file_count 为 0，不再额外发送空包。
     */
    if ((file_count > 0U) || (filecount == 0U)) {
        memcpy(&data_buf[0], &filecount, sizeof(filecount));
        memcpy(&data_buf[4], &file_count, sizeof(file_count));
        if (data_idx > 0U) {
            memcpy(&data_buf[8], response_data, data_idx);
        }
        send_response_file(CMD_READ_FILE_LIST, data_buf, (uint16_t)(data_idx + 8U));
    }
}

static void send_total_packets(uint32_t total_packets) {
    uint8_t data[4];
    data[0] = total_packets & 0xFF;
    data[1] = (total_packets >> 8) & 0xFF;
    data[2] = (total_packets >> 16) & 0xFF;
    data[3] = (total_packets >> 24) & 0xFF;
    send_response_file(CMD_DOWNLOAD_FILE, data, 4);
}


// 发送下载数据包：包号、有效数据长度和文件数据。
static void send_data_packet(uint32_t packet_num, uint8_t *data, uint32_t data_len) {
    uint8_t packet_data[4 + 4 + MAX_PACKET_SIZE];
    int idx = 0;
    // 包号采用 4 字节小端格式。
    packet_data[idx++] = packet_num & 0xFF;
    packet_data[idx++] = (packet_num >> 8) & 0xFF;
    packet_data[idx++] = (packet_num >> 16) & 0xFF;
    packet_data[idx++] = (packet_num >> 24) & 0xFF;
    // 本包有效文件数据长度采用 4 字节小端格式。
    packet_data[idx++] = data_len & 0xFF;
    packet_data[idx++] = (data_len >> 8) & 0xFF;
    packet_data[idx++] = (data_len >> 16) & 0xFF;
    packet_data[idx++] = (data_len >> 24) & 0xFF;
    // 复制当前文件分包的数据内容。
    if (data && data_len > 0) {
        memcpy(packet_data + idx, data, data_len);
        idx += data_len;
    }
    send_response_file(CMD_DOWNLOAD_FILE, packet_data, idx);
}

// 发送文件操作失败响应。
static void send_error_response(uint8_t cmd, uint8_t error_code) {
    /*
     * 文件操作失败时统一回复固定的 4 字节错误内容：
     * FF 55 FF 55。外层仍由 send_response_file() 追加完整协议头、
     * 命令码、长度、CRC16 和帧尾。
     */
    static const uint8_t data[4] = {0xFF, 0x55, 0xFF, 0x55};

    (void)error_code;
    send_response_file(cmd, (uint8_t *)data, sizeof(data));
}

// 发送删除文件响应。
static void send_delete_response(uint8_t cmd, uint8_t *data, uint16_t data_len) {
    send_response_file(cmd, data, data_len);
}

// 发送格式化成功响应。
static void send_format_response(void) {
    send_response_file(CMD_FORMAT, NULL, 0);
}

// 发送批量删除完成响应，内容为成功删除的文件数量。
static void send_batch_delete_response(uint32_t deleted_count) {
    uint8_t data[4];
    data[3] = (deleted_count >> 24) & 0xFF;
    data[2] = (deleted_count >> 16) & 0xFF;
    data[1] = (deleted_count >> 8) & 0xFF;
    data[0] = deleted_count & 0xFF;
    send_response_file(CMD_BATCH_DELETE, data, 4);
}


// ==================== 文件命令处理 ====================

/*
 * 处理读取文件列表命令。
 */
static void handle_read_file_list(void) {
    /* 收到读取文件列表请求后，同步扫描目录并回复一帧或多帧列表。 */
    ctx.state = STATE_READING_FILE_LIST;
    send_file_list_response();
    ctx.state = STATE_IDLE;
}

/*
 * 处理下载文件命令。
 */
static void handle_download_file(uint8_t *buffer) {
    uint32_t last_activity = get_current_tick();
    FRESULT ret;
    char f_path[64];
    /*
     * 显控协议中的文件名固定为 19 字节，不包含字符串结束符。
     * 本地数组额外预留 1 字节 '\0'，只影响 C 字符串处理，不改变串口协议。
     */
    char f_name[FILENAME_LEN + 1] = {0};
    ctx.state = STATE_DOWNLOAD_WAIT_FILENAME;
    /* 请求帧从 buffer[4] 开始携带固定 19 字节文件名。 */
    memcpy(f_name, &buffer[4], FILENAME_LEN);
	
    /* 组合 FatFs 路径时使用已补结束符的本地文件名。 */
    snprintf(f_path, sizeof(f_path), "%s/%s", SVP_PATH, f_name);
    // 以只读方式打开显控请求下载的文件。
    ret = f_open(&ctx.download.file, f_path, FA_READ);
    if (ret != FR_OK) {
        printf("OPEN FILE FAILED\r\n");
        fat_error_func();
        ret = f_open(&ctx.download.file, f_path, FA_READ);
        if (ret != FR_OK) {
            printf("open download: %s, error=%d\r\n", f_path, ret);
            send_error_response(CMD_DOWNLOAD_FILE, STATUS_FILE_NOT_FOUND);
            ctx.state = STATE_IDLE;
            return;
        }
    }
    ctx.state = STATE_DOWNLOAD_FILE_OPENED;
    // 获取文件总字节数。
    ctx.download.file_size = f_size(&ctx.download.file);
    // 按 MAX_PACKET_SIZE 计算显控需要请求的总包数。
    ctx.download.total_packets = (ctx.download.file_size + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;
    if (ctx.download.total_packets == 0) {
        ctx.download.total_packets = 1;
    }
    // 首先向显控回复总包数，随后等待显控逐包请求。
    send_total_packets(ctx.download.total_packets);
    ctx.state = STATE_DOWNLOADING;
	
    while(1){
        // 显控 5 秒未发送有效下载交互帧时，关闭文件并退出下载状态。
        if (is_timeout(last_activity, DOWNLOAD_TIMEOUT_MS)){
            printf("download timeout\r\n");
            f_close(&ctx.download.file);
            ctx.state = STATE_IDLE;
            break;
        }
		if(frame_receied){
            // 校验当前下载交互帧，成功时返回协议长度字段。
			int data_len = validate_download_request_file(rx_buffer, rx_length);
			if (data_len == -1) {
				printf("CRC failed\r\n");
				f_close(&ctx.download.file);
				ctx.state = STATE_IDLE;
				break;
			}
            /* 只有 CRC 正确的下载交互帧才刷新通信活动时间。 */
			last_activity = get_current_tick();
            uint8_t *rx_content = rx_buffer + 4;  // 内容起始：帧头、命令码和 2 字节长度之后
            if (data_len-3 == 4) {  // 4 字节内容表示显控请求指定包号
				uint32_t packet_num = (rx_content[3] << 24) | (rx_content[2] << 16) |
									  (rx_content[1] << 8) | rx_content[0];
				if (packet_num >= ctx.download.total_packets) {
					printf("invalid packet num\r\n");
					restart_uart_dma_receive();
					continue;
				}
                // 定位、读取并回复显控请求的文件分包。
				uint32_t offset = packet_num * MAX_PACKET_SIZE;
				uint32_t to_read = MAX_PACKET_SIZE;
				if (offset + to_read > ctx.download.file_size) {
					to_read = ctx.download.file_size - offset;
				}
				f_lseek(&ctx.download.file, offset);
				uint8_t file_buffer[MAX_PACKET_SIZE];
				UINT bytes_read;
				f_read(&ctx.download.file, file_buffer, to_read, &bytes_read);
				send_data_packet(packet_num, file_buffer, bytes_read);
				ctx.download.current_packet = packet_num;
				continue;
            }else if(data_len-3 == 1) {  // 1 字节内容表示控制命令
                if (rx_content[0] == 0x01) {  // 请求重发最近一次发送的包
					uint32_t offset = ctx.download.current_packet * MAX_PACKET_SIZE;
					uint32_t to_read = MAX_PACKET_SIZE;
					if (offset + to_read > ctx.download.file_size) {
						to_read = ctx.download.file_size - offset;
					}
					f_lseek(&ctx.download.file, offset);
					uint8_t file_buffer[MAX_PACKET_SIZE];
					UINT bytes_read;
					f_read(&ctx.download.file, file_buffer, to_read, &bytes_read);
					send_data_packet(ctx.download.current_packet, file_buffer, bytes_read);
					continue;
                }else if (rx_content[0] == 0xFF) {  // 显控确认下载完成
					f_close(&ctx.download.file);
					ctx.state = STATE_IDLE;
					break;
				}
			}
            /*
             * CRC 正确但内容不是当前下载状态可识别的请求时，不回复数据；
             * 仍需恢复 DMA，使显控能够继续发送下一帧。
             */
			restart_uart_dma_receive();
		}else{
            /* 下载等待期间让出 CPU，避免命令任务空转占满处理器。 */
			vTaskDelay(pdMS_TO_TICKS(1));
		}
	}
    ctx.state = STATE_IDLE;
}

// 处理删除单个文件命令。
static void handle_delete_file(uint8_t *buffer) {  // buffer 从第 4 字节开始携带固定长度文件名
    /*
     * 回复显控时仍只发送前 19 字节；额外的结束符仅用于 DeletTheFile()
     * 内部拼接 FatFs 路径，避免把栈中的后续数据误当作文件名的一部分。
     */
    char f_name[FILENAME_LEN + 1] = {0};
	FRESULT res;
    ctx.state = STATE_DELETING_FILE;  // 删除过程内禁止并发文件操作
    memcpy(f_name, &buffer[4], FILENAME_LEN);
	
    /* 调试时可直接指定固定文件路径调用 f_unlink()。 */
	
    res = DeletTheFile(f_name);  // 由底层函数拼接目录路径并删除文件
    
    if (res != FR_OK) {
        printf("delete error:%d\r\n",res);
        send_error_response(CMD_DELETE_FILE, STATUS_DELETE_ERROR);
    } else {
        /* 删除成功时回传固定 19 字节文件名。 */
        send_delete_response(CMD_DELETE_FILE, (uint8_t*)f_name, FILENAME_LEN);
    }
    ctx.state = STATE_IDLE;
}

// 处理格式化文件系统命令。
static void handle_format(void) {
	ctx.state = STATE_FORMATTING;
    if(f_mkfs_func() == FR_OK){
        send_format_response();
    }else{
        /*
         * 格式化失败时不向显控发送成功帧，也不额外发送失败协议帧。
         * 显控会因未收到成功回复而超时；详细失败原因由底层串口日志输出。
         */
        printf("format command failed\r\n");
    }
    ctx.state = STATE_IDLE;
}

// 处理批量删除文件命令。
static void handle_batch_delete(uint8_t *data, uint16_t data_len) {
    /* 批量删除请求的 data 区第一个 4 字节为文件数量。 */
    ctx.state = STATE_BATCH_DELETING;
    // 按协议解析需要删除的文件数量。
    uint32_t file_count = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
    /* 调试时可在这里输出批量删除数量。 */
    uint32_t expected_len = 4 + file_count * FILENAME_LEN + 2/*CRC*/ + 1/*55*/;
    if (data_len < expected_len) {
        /* 请求帧长度不足时拒绝执行删除。 */
        send_error_response(CMD_BATCH_DELETE, STATUS_PARAM_ERROR);
        ctx.state = STATE_IDLE;
        return;
    }
    uint32_t deleted_count = 0;
    uint8_t *ptr = data + 4;
    /*
     * 批量删除协议中每个文件名固定为 19 字节。数组额外保留结束符，
     * 使每次传入 DeletTheFile() 的文件名都是有效 C 字符串。
     */
    char filename[FILENAME_LEN + 1] = {0};
    for (uint32_t i = 0; i < file_count; i++) {
        /* 每轮复制后 filename[19] 仍保持为 '\0'。 */
        memcpy(filename, ptr, FILENAME_LEN);
        FRESULT res = DeletTheFile(filename);
        if (res == FR_OK) {
            deleted_count++;
            /* 调试时可在这里记录成功删除的文件名。 */
        } else {
            /* 单个文件删除失败时继续处理后续文件。 */
        }
        ptr += FILENAME_LEN;
    }
    /* 回复实际成功删除的文件数量。 */
    send_batch_delete_response(deleted_count);
    ctx.state = STATE_IDLE;
}



// ==================== 对外状态机接口 ====================

/*
 * 初始化文件管理状态机。
 */
void file_manager_fsm_init(void) {
    memset(&ctx, 0, sizeof(ctx));
    ctx.state = STATE_IDLE;
}

/*
 * 文件操作主入口。
 * 调用前已由串口命令层确认命令属于文件操作；这里再次校验文件协议 CRC，
 * 然后按命令码进入文件列表、下载、删除、格式化或批量删除处理。
 */





void file_manager_fsm_process(uint8_t *buffer, int len) {
    uint8_t cmd = buffer[1];
    int data_len = validate_download_request_file(buffer,len);
    if(data_len == -1){
        return; // 文件协议 CRC 校验失败时不执行任何文件操作
    }
    // 预留状态机活动时间字段，当前下载超时由下载循环独立维护。
    ctx.last_activity_time = 0;  // 暂未接入统一状态机超时计时
    // 根据文件命令码分派到对应处理函数。
    switch (cmd) {
        case CMD_READ_FILE_LIST:
            handle_read_file_list();
            break;
            
        case CMD_DOWNLOAD_FILE:
            handle_download_file(rx_buffer);
            break;
            
        case CMD_DELETE_FILE:
            handle_delete_file(rx_buffer);
            break;
            
        case CMD_FORMAT:
            handle_format();
            break;
            
        case CMD_BATCH_DELETE:
            handle_batch_delete(&buffer[4], data_len);
            break;
            
        default:
            /* 未定义的文件命令不处理。 */
            break;
    }
}

FileManagerState file_manager_fsm_get_state(void) {
    return ctx.state;
}

void file_manager_fsm_reset(void) {
    printf("reste state\r\n");
    
    if (ctx.state == STATE_DOWNLOAD_FILE_OPENED || 
        ctx.state == STATE_DOWNLOADING ||
        ctx.state == STATE_DOWNLOAD_WAIT_RETRANS) {
        f_close(&ctx.download.file);
    }
    
    memset(&ctx, 0, sizeof(ctx));
    ctx.state = STATE_IDLE;
}

void file_manager_fsm_timeout_check(uint32_t current_tick, uint32_t timeout_ms) {
    if (ctx.state != STATE_IDLE && ctx.state != STATE_ERROR) {
        if (current_tick - ctx.last_activity_time > timeout_ms) {
            printf("State timeout %d reset\r\n", ctx.state);
            file_manager_fsm_reset();
        }
    }
}

uint8_t file_manager_fsm_get_error(void) {
    return ctx.error_code;
}


