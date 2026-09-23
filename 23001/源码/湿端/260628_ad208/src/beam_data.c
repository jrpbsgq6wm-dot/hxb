#include "beam.h"

/********************************************************************************
 * 模块：FPGA中断数据组包与显控发送
 * 说明：由原 beam.c 按功能拆分，函数体保持原有逻辑。
 ********************************************************************************/

/********************************************************************************
 * 名称：                    my_copy
 * 功能：                    汇编实现memcpy函数
 * 入口参数：            	 volatile unsigned char *dst, volatile unsigned char *src, int sz
 * 出口参数：            	 无
 *********************************************************************************/
//void __attribute__((noinline)) my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz)
void my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz)
{
	if (sz & 63) {
		sz = (sz & -64) + 64;
	}
	asm volatile (
			"NEONCopyPLD:                          \n"
			"    VLDM %[src]!,{d0-d7}                 \n"
			"    VSTM %[dst]!,{d0-d7}                 \n"
			"    SUBS %[sz],%[sz],#0x40                 \n"
			"    BGT NEONCopyPLD                  \n"
			: [dst]"+r"(dst), [src]"+r"(src), [sz]"+r"(sz) : : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
}

/*
void __attribute__((noinline)) my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz)
{
	if (sz & 63) 
	{
		sz = (sz & -64) + 64;
	}
	__asm__ __volatile__(
			"NEONCopyPLD:                          \n"
			"    VLDM %[src]!,{d0-d7}                 \n"
			"    VSTM %[dst]!,{d0-d7}                 \n"
			"    SUBS %[sz],%[sz],#0x40                 \n"
			"    BGT NEONCopyPLD                  \n"
			: [dst]"+r"(dst), [src]"+r"(src), [sz]"+r"(sz) : : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
}
*/
/********************************************************************************
 * 名称：                    checksum
 * 功能：                    和校验
 * 入口参数：            	 char *buf, unsigned int nword
 * 出口参数：            	 无
 *********************************************************************************/
unsigned short checksum(char *buf, unsigned int nword)  
{  
	unsigned long sum;    
	for(sum = 0; nword > 0; nword--){
		sum += *buf++;              /*获取buf的累加和*/     
	}
	sum  = (sum>>16) + (sum&0xffff);/*获取sum高16位与低16位的和*/  
	sum += (sum>>16);     
	return ~sum;  
}  
/********************************************************************************
 * 名称：                    send_all_package_to_upper
 * 功能：                    ARM向上位机发送声呐数据和传感器数据等总包
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void send_all_package_to_upper(void)
{
	//DBG("s\n");
	//send_crc_S = crc16_2(ptr_to_send_S, 0,total_len-6);//计算CRC
	//sum_check=checksum(ptr_to_send_S, total_len-6);
	// 发送报头4字节
	//

	/*unsigned int max_dst_size;
	  unsigned int compressed_data_size;
	  unsigned int rv,comp_size;
	  double timeuse;
	  LZ4_stream_t state1;
	  printf("total_len=%d\n",total_len);
	  max_dst_size = LZ4_compressBound(total_len);
	  printf("max_dst_size=%d\n",max_dst_size);
	//gettimeofday(&start_time,NULL);
	char *compressed_data= malloc(max_dst_size);
	if(compressed_data==NULL){
	printf("calloc failed !!!\n");
	return;
	}
	gettimeofday(&start_time,NULL);
	compressed_data_size = LZ4_compress_fast(ddr_sonar_data,compressed_data, total_len, max_dst_size,64000);
	//compressed_data_size = LZ4_compress_fast_extState(&state1,ddr_sonar_data,compressed_data, total_len, max_dst_size,1);
	//compressed_data_size = LZ4_compress_default(ddr_sonar_data,compressed_data, total_len, max_dst_size);
	gettimeofday(&end_time,NULL);
	printf("compressed_data_size=%d\n",compressed_data_size);
	if(compressed_data_size<0){
	printf("compress failed !!!\n");
	return;

	}
	//gettimeofday(&end_time,NULL);
	timeuse=1000000*(end_time.tv_sec-start_time.tv_sec)+end_time.tv_usec-start_time.tv_usec;
	printf("timeuse=%f\n",timeuse/1000);
	compressed_data = (char *)realloc(compressed_data, compressed_data_size);
	char *decompressed_data= malloc(total_len);
	if(decompressed_data==NULL){
	printf("calloc failed !!!\n");
	return;
	}

	rv = LZ4_decompress_safe(compressed_data, decompressed_data, compressed_data_size, total_len);
	printf("rv=%d\n",rv);
	if(rv<1){
	printf("decompress failed!!!\n");
	return;
	}*/




	if (send_all_bytes(Connect_fd, DataHead_S, SIZE_OF_LONG) < 0) 
	{       
		error_process();
		return; 
	}
	// 发送4字节总长度
	if (send_all_bytes(Connect_fd, &total_len, SIZE_OF_LONG) < 0) 
	{       
		error_process();
		return; 
	}
	/*if (send(Connect_fd, (char *)&compressed_data_size , SIZE_OF_LONG, 0)< 0) 
	  {          
	  error_process();                                             
	  return;                                                      
	  }          

	  if (send(Connect_fd, (char *)(compressed_data), compressed_data_size, 0)< 0) 
	  {       
	  error_process();
	  return; 
	  }  */

	//free(decompressed_data);
	//free(compressed_data);
	//发送46+整包ddr+传感器+crc+报尾
	if (send_all_bytes(Connect_fd, (char *)(ddr_sonar_data+SONAR_DATA_OFFSET-SIZE_OF_SEND_UPPER_PACKAGE_FIRST), total_len)< 0) 
	{       
		error_process();
		return; 
	}  
	DBG("Send byte is: %d\n",total_len);
	memset(ddr_sonar_data,0,sizeof(ddr_sonar_data));
}

static int is_upper_8001_connected(void)
{
	return (netStatus_8001 == 1 && Connect_fd_8001 > 0);
}

static int send_all_package_to_8001(void)
{
	if (!is_upper_8001_connected())
	{
		send_to_8001_flag = 0;
		return -1;
	}

	if (send_all_bytes(Connect_fd_8001, DataHead_S, SIZE_OF_LONG) < 0)
	{
		error_process_8001();
		return -1;
	}
	if (send_all_bytes(Connect_fd_8001, &total_len, SIZE_OF_LONG) < 0)
	{
		error_process_8001();
		return -1;
	}
	if (send_all_bytes(Connect_fd_8001, (char *)(ddr_sonar_data+SONAR_DATA_OFFSET-SIZE_OF_SEND_UPPER_PACKAGE_FIRST), total_len)< 0)
	{
		error_process_8001();
		return -1;
	}

	DBG("Send byte to 8001 is: %d\n",total_len);
	memset(ddr_sonar_data,0,sizeof(ddr_sonar_data));
	return 0;
}
/********************************************************************************
 * 名称：                    lookfor_five_sensor_num
 * 功能：                    确定几个传感器各自的条数
 * 入口参数：            	 const int *ptr_sensor1,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5
 * 出口参数：            	 无
 *********************************************************************************/
void lookfor_five_sensor_num(const int *ptr_sensor1,const int *ptr_sensor2,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5)
{
	sensor1_num = 0;
	sensor2_num = 0;
	sensor3_num = 0;
	sensor4_num = 0;
	sensor5_num = 0;
	int m;
	for (m=0;m<100;m++)
	{
		size_t offset1 = m * 64;
        size_t offset2 = offset1 + 1;
		// 内存偏移检查
        if (offset2 >= 0x100000) {
            Debug("clear_sensor_data warning: offset out of range at m=%d\n", m);
            break;
        }
		/* 
			判断内存指向的地址空间帧头是否为0xDDDDDDDD
		*/
		if ((*(ptr_sensor1+offset1)==0xDDDDDDDD)&&(*(ptr_sensor1+offset2)==(ptr_fpga_frame_first->FrameNumber-1)))
		{
			sensor1_num++;
		}
		if ((*(ptr_sensor2+offset1)==0xDDDDDDDD)&&(*(ptr_sensor2+offset2)==(ptr_fpga_frame_first->FrameNumber-1)))
		{
			sensor2_num++;
		}
		if ((*(ptr_sensor3+offset1)==0xDDDDDDDD)&&(*(ptr_sensor3+offset2)==(ptr_fpga_frame_first->FrameNumber-1)))
		{
			sensor3_num++;
		}
		if ((*(ptr_sensor4+offset1)==0xDDDDDDDD)&&(*(ptr_sensor4+offset2)==(ptr_fpga_frame_first->FrameNumber-1)))
		{
			sensor4_num++;
		}
		if ((*(ptr_sensor5+offset1)==0xDDDDDDDD)&&(*(ptr_sensor5+offset2)==(ptr_fpga_frame_first->FrameNumber-1)))
		{
			sensor5_num++;
		}
	}
}

/********************************************************************************
 * 名称：                    copy_sensor_data
 * 功能：                    将传感器信息拷贝到ddr_sonar_data数组的指定位置中
 * 入口参数：            	 若干
 * 出口参数：            	 无
 *********************************************************************************/
void copy_sensor_data(const int *ptr_sensor1,const int *ptr_sensor2,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5)
{
	lookfor_five_sensor_num(ptr_sensor1,ptr_sensor2,ptr_sensor3,ptr_sensor4,ptr_sensor5);
	my_copy((unsigned char *)(ddr_sonar_data + ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST), (unsigned char *)ptr_sensor1, sensor1_num*SIZE_OF_ONE_PING_SENSOR); 
	my_copy((unsigned char *)(ddr_sonar_data + ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG + sensor1_num * SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor2, sensor2_num * SIZE_OF_ONE_PING_SENSOR); 
	my_copy((unsigned char *)(ddr_sonar_data + ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG *2 + sensor1_num * SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor3, sensor3_num * SIZE_OF_ONE_PING_SENSOR); 
	my_copy((unsigned char *)(ddr_sonar_data + ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG *3+(sensor1_num+sensor3_num)*SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor4, sensor4_num*SIZE_OF_ONE_PING_SENSOR);
	my_copy((unsigned char *)(ddr_sonar_data + ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG *4+(sensor1_num+sensor3_num+sensor4_num)*SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor5,sensor5_num*SIZE_OF_ONE_PING_SENSOR);
}

/********************************************************************************
 * 名称：                    copy_letf_three_sensor_num_to_ddr
 * 功能：                    将除了GGA_ZDA的另外3个传感器信息的条数信息拷贝到ddr_sonar_data数组的指定位置中
 * 入口参数：            	 若干
 * 出口参数：            	 无
 *********************************************************************************/
void copy_letf_three_sensor_num_to_ddr(void)
{
	*((int *)(ddr_sonar_data+ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + sensor1_num*SIZE_OF_ONE_PING_SENSOR))=sensor2_num;//gga条数

	*((int *)(ddr_sonar_data+ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG + (sensor1_num+sensor2_num)*SIZE_OF_ONE_PING_SENSOR))=sensor3_num;//heading条数

	*((int *)(ddr_sonar_data+ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG*2 + (sensor1_num+sensor2_num+sensor3_num) * SIZE_OF_ONE_PING_SENSOR)) = sensor4_num; //motion条数

	*((int *)(ddr_sonar_data+ptr_send_to_upper_package_first->SonarDataLength + SONAR_DATA_OFFSET + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG*3+(sensor1_num+sensor2_num+sensor3_num+sensor4_num)*SIZE_OF_ONE_PING_SENSOR)) =sensor5_num;//SVT条数
}

/********************************************************************************
 * 名称：                    copy_sensor_data_from_pingpang_buf
 * 功能：                    这个函数调用上述几个函数完成从乒乓缓存拷贝传感器数据到ddr_sonar_data数组的指定位置中
 * 入口参数：            	 若干
 * 出口参数：            	 无
 *********************************************************************************/
void copy_sensor_data_from_pingpang_buf(void)
{
	if (((ptr_fpga_frame_first->FrameNumber)&(0x00000001))==0x00000001)//奇数先发
	{
		//拷贝4个传感器,4个传感器条数已确定
		copy_sensor_data(uio_sensor_mem_0.mem_ptr,uio_sensor_mem_1.mem_ptr,uio_sensor_mem_2.mem_ptr,uio_sensor_mem_3.mem_ptr,uio_sensor_mem_4.mem_ptr);
	}
	if (((ptr_fpga_frame_first->FrameNumber)&(0x00000001))==0) //偶数后发
	{
		//拷贝4个传感器 ,4个传感器条数已确定
		copy_sensor_data(uio_sensor_mem_8.mem_ptr,uio_sensor_mem_9.mem_ptr,uio_sensor_mem_A.mem_ptr,uio_sensor_mem_B.mem_ptr,uio_sensor_mem_C.mem_ptr);
	}
	ptr_send_to_upper_sensor->Senor_total_len=SIZE_OF_SENSOR_WITHOUT_SENSOR_DATA + SIZE_OF_ONE_PING_SENSOR * (sensor1_num+sensor2_num+sensor3_num+sensor4_num+sensor5_num);
	ptr_send_to_upper_sensor->SynchState = 1;
	ptr_send_to_upper_sensor->GGA_ZDA_NUM = sensor1_num; 
	memcpy(ddr_sonar_data+ptr_send_to_upper_package_first->SonarDataLength+SONAR_DATA_OFFSET,ptr_send_to_upper_sensor, SIZE_OF_SENSOR_FIRST); //先拷贝传感器总字节数，同步状态，预留字节，GGA个数这16字节的信息
	copy_letf_three_sensor_num_to_ddr(); 
}

/********************************************************************************
 * 名称：                    copy_tail_to_sonar_data
 * 功能：                    拷贝报尾ED>>四个字节到ddr_sonar_data数组的末尾
 * 入口参数：            	 若干
 * 出口参数：            	 无
 *********************************************************************************/
void copy_tail_to_sonar_data(void)
{
	memcpy(ddr_sonar_data + SONAR_DATA_OFFSET + ptr_send_to_upper_package_first->SonarDataLength + SIZE_OF_LEN_OF_SENSOR_DATA + SIZE_OF_SENSOR_WITHOUT_SENSOR_DATA + (sensor1_num+sensor2_num+sensor3_num+sensor4_num+sensor5_num) * SIZE_OF_ONE_PING_SENSOR + SIZE_OF_USHORT, DataTail_S, SIZE_OF_LONG); 
}
/**************************************************************************************
 * 名称：                    copy_sensor_data_streaming
 * 功能：                    将传感器信息拷贝到ddr_sonar_data数组的指定位置中
 * 出口参数：             无
 * 出口参数：             无
 * **********************************************************************************/
void copy_sensor_data_streaming(const int *ptr_sensor1,const int *ptr_sensor2,const int *ptr_sensor3,const int *ptr_sensor4,const int *ptr_sensor5)
{
	lookfor_five_sensor_num(ptr_sensor1,ptr_sensor2,ptr_sensor3,ptr_sensor4,ptr_sensor5);
	my_copy((unsigned char *)(ddr_sonar_data + SIZE_OF_SENSOR_FIRST), (unsigned char *)ptr_sensor1, sensor1_num*SIZE_OF_ONE_PING_SENSOR); 
	my_copy((unsigned char *)(ddr_sonar_data + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG + sensor1_num * SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor2, sensor2_num * SIZE_OF_ONE_PING_SENSOR); 
	my_copy((unsigned char *)(ddr_sonar_data + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG *2 + sensor1_num * SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor3, sensor3_num * SIZE_OF_ONE_PING_SENSOR); 
	my_copy((unsigned char *)(ddr_sonar_data + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG *3+(sensor1_num+sensor3_num)*SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor4, sensor4_num*SIZE_OF_ONE_PING_SENSOR);
	my_copy((unsigned char *)(ddr_sonar_data + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG *4+(sensor1_num+sensor3_num+sensor4_num)*SIZE_OF_ONE_PING_SENSOR), (unsigned char *)ptr_sensor5,sensor5_num*SIZE_OF_ONE_PING_SENSOR);
}

/**************************************************************************************
 * 名称：                    copy_letf_three_sensor_num_to_ddr_streaming
 * 功能：                    将除了GGA_ZDA的另外3个传感器信息的条数信息拷贝到ddr_sonar_data数组的指定位置中
 * 出口参数：             无
 * 出口参数：             无
 * **********************************************************************************/
void copy_letf_three_sensor_num_to_ddr_streaming(void)
{
	*((int *)(ddr_sonar_data + SIZE_OF_SENSOR_FIRST + sensor1_num*SIZE_OF_ONE_PING_SENSOR))=sensor2_num;
	*((int *)(ddr_sonar_data + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG + (sensor1_num+sensor2_num)*SIZE_OF_ONE_PING_SENSOR))=sensor3_num;
	*((int *)(ddr_sonar_data + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG*2 + (sensor1_num+sensor2_num+sensor3_num) * SIZE_OF_ONE_PING_SENSOR)) = sensor4_num;
	*((int *)(ddr_sonar_data + SIZE_OF_SENSOR_FIRST + SIZE_OF_LONG*3+(sensor1_num+sensor2_num+sensor3_num+sensor4_num)*SIZE_OF_ONE_PING_SENSOR)) =sensor5_num;
}

/**************************************************************************************
 * 名称：                    copy_sensor_data_from_pingpang_buf_streaming
 * 功能：                    
 * 出口参数：             无
 * 出口参数：             无
 * **********************************************************************************/
void copy_sensor_data_from_pingpang_buf_streaming(void)
{
	if (((ptr_fpga_frame_first->FrameNumber)&(0x00000001))==0x00000001)
	{
		//拷贝4个传感器,4个传感器条数已确定
		copy_sensor_data_streaming(uio_sensor_mem_0.mem_ptr,uio_sensor_mem_1.mem_ptr,uio_sensor_mem_2.mem_ptr,uio_sensor_mem_3.mem_ptr,uio_sensor_mem_4.mem_ptr);
	}
	if (((ptr_fpga_frame_first->FrameNumber)&(0x00000001))==0)
	{
		//拷贝4个传感器,4个传感器条数已确定
		copy_sensor_data_streaming(uio_sensor_mem_8.mem_ptr,uio_sensor_mem_9.mem_ptr,uio_sensor_mem_A.mem_ptr,uio_sensor_mem_B.mem_ptr,uio_sensor_mem_C.mem_ptr);
	}
	DBG("sensor1_num:%d,sensor2_num%d,sensor3_num%d,sensor4_num%d,sensor5_num%d\n",sensor1_num,sensor2_num,sensor3_num,sensor4_num,sensor5_num);
	ptr_send_to_upper_sensor->Senor_total_len=SIZE_OF_SENSOR_WITHOUT_SENSOR_DATA + SIZE_OF_ONE_PING_SENSOR * (sensor1_num+sensor2_num+sensor3_num+sensor4_num+sensor5_num);
	ptr_send_to_upper_sensor->SynchState = 1;
	ptr_send_to_upper_sensor->GGA_ZDA_NUM = sensor1_num; 
	memcpy(ddr_sonar_data,ptr_send_to_upper_sensor, SIZE_OF_SENSOR_FIRST);
	copy_letf_three_sensor_num_to_ddr_streaming(); 
}

/**************************************************************************************
 * 名称：                    send_original_data_streaming
 * 功能：                    
 * 出口参数：             无
 * 出口参数：             无
 * **********************************************************************************/
void send_original_data_streaming(void)
{
	if (Fpga_start_mod != 1)
	{
		return;
	}

	//计算传感器数据长度，并将传感器数据暂存拷贝到缓冲区中
	copy_sensor_data_from_pingpang_buf_streaming();
	//计算传感器数据长度
	unsigned int sensor_section_size = SIZE_OF_LEN_OF_SENSOR_DATA + ptr_send_to_upper_sensor->Senor_total_len;
	total_len = SIZE_OF_SEND_UPPER_PACKAGE_FIRST + ptr_send_to_upper_package_first->SonarDataLength + SIZE_OF_LEN_OF_SENSOR_DATA + ptr_send_to_upper_sensor->Senor_total_len + SIZE_OF_USHORT + SIZE_OF_LONG;

	pthread_mutex_lock(&mut);
		//发送帧头 <<ST
		if (send_all_bytes(Connect_fd, DataHead_S, SIZE_OF_LONG) < 0) 
		{       
			error_process();
			pthread_mutex_unlock(&mut);
			return; 
		}
		//发送长度
		if (send_all_bytes(Connect_fd, &total_len, SIZE_OF_LONG) < 0) 
		{       
			error_process();
			pthread_mutex_unlock(&mut);
			return; 
		}
		//发送128字节参数头
		if (send_all_bytes(Connect_fd, ptr_send_to_upper_package_first, SIZE_OF_SEND_UPPER_PACKAGE_FIRST) < 0)
		{
			error_process();
			pthread_mutex_unlock(&mut);
			return;
		}
		//分块发送 /4M
		unsigned int remaining = ptr_send_to_upper_package_first->SonarDataLength;
		unsigned char *src = (unsigned char *)uio_share_mem_original.mem_ptr;
		unsigned int chunk_size = 4 * 1024 * 1024;
		while (remaining > 0)
		{
			unsigned int to_send = (remaining > chunk_size) ? chunk_size : remaining;
			if (send_all_bytes(Connect_fd, src, to_send) < 0)
			{
				error_process();
				pthread_mutex_unlock(&mut);
				return;
			}
			src += to_send;
			remaining -= to_send;
		}

		//发送传感器数据
		if (send_all_bytes(Connect_fd, ddr_sonar_data, sensor_section_size) < 0)
		{
			error_process();
			pthread_mutex_unlock(&mut);
			return;
		}
		//发送
		unsigned short padding = 0;
		if (send_all_bytes(Connect_fd, &padding, SIZE_OF_USHORT) < 0)
		{
			error_process();
			pthread_mutex_unlock(&mut);
			return;
		}
		//发送帧尾
		if (send_all_bytes(Connect_fd, DataTail_S, SIZE_OF_LONG) < 0)
		{
			error_process();
			pthread_mutex_unlock(&mut);
			return;
		}

		pthread_mutex_unlock(&mut);
		sensor1_num=0;
		sensor2_num=0;
		sensor3_num=0;
		sensor4_num=0;
		sensor5_num=0;
}

/**************************************************************************************
 * 名称：                    send_original_data_streaming_8001
 * 功能：                    8001端口数据上传功能
 * 出口参数：             无
 * 出口参数：             无
 * **********************************************************************************/
void send_original_data_streaming_8001(void)
{
	if (Fpga_start_mod != 1)
	{
		return;
	}

	//计算传感器数据长度，并将传感器数据暂存拷贝到缓冲区中
	if (netStatus_8001 != 1 || Connect_fd_8001 <= 0)
	{
		send_to_8001_flag = 0;
		return;
	}

	copy_sensor_data_from_pingpang_buf_streaming();
	//计算传感器数据长度
	unsigned int sensor_section_size = SIZE_OF_LEN_OF_SENSOR_DATA + ptr_send_to_upper_sensor->Senor_total_len;
	total_len = SIZE_OF_SEND_UPPER_PACKAGE_FIRST + ptr_send_to_upper_package_first->SonarDataLength + SIZE_OF_LEN_OF_SENSOR_DATA + ptr_send_to_upper_sensor->Senor_total_len + SIZE_OF_USHORT + SIZE_OF_LONG;

	pthread_mutex_lock(&mut_8001);
	//发送帧头 <<ST
	if (send_all_bytes(Connect_fd_8001, DataHead_S, SIZE_OF_LONG) < 0) 
	{       
		error_process_8001();
		pthread_mutex_unlock(&mut_8001);
		return; 
	}
	//发送长度
	if (send_all_bytes(Connect_fd_8001, &total_len, SIZE_OF_LONG) < 0) 
	{       
		error_process_8001();
		pthread_mutex_unlock(&mut_8001);
		return; 
	}
	//发送128字节参数头
	if (send_all_bytes(Connect_fd_8001, ptr_send_to_upper_package_first, SIZE_OF_SEND_UPPER_PACKAGE_FIRST) < 0)
	{
		error_process_8001();
		pthread_mutex_unlock(&mut_8001);
		return;
	}
	//分块发送 /4M
	unsigned int remaining = ptr_send_to_upper_package_first->SonarDataLength;
	unsigned char *src = (unsigned char *)uio_share_mem_original.mem_ptr;
	unsigned int chunk_size = 4 * 1024 * 1024;
	while (remaining > 0)
	{
		unsigned int to_send = (remaining > chunk_size) ? chunk_size : remaining;
		if (send_all_bytes(Connect_fd_8001, src, to_send) < 0)
		{
			error_process_8001();
			pthread_mutex_unlock(&mut_8001);
			return;
		}
		src += to_send;
		remaining -= to_send;
	}

	//发送传感器数据
	if (send_all_bytes(Connect_fd_8001, ddr_sonar_data, sensor_section_size) < 0)
	{
		error_process_8001();
		pthread_mutex_unlock(&mut_8001);
		return;
	}
	//发送
	unsigned short padding = 0;
	if (send_all_bytes(Connect_fd_8001, &padding, SIZE_OF_USHORT) < 0)
	{
		error_process_8001();
		pthread_mutex_unlock(&mut_8001);
		return;
	}
	//发送帧尾
	if (send_all_bytes(Connect_fd_8001, DataTail_S, SIZE_OF_LONG) < 0)
	{
		error_process_8001();
		pthread_mutex_unlock(&mut_8001);
		return;
	}

	pthread_mutex_unlock(&mut_8001);
	sensor1_num=0;
	sensor2_num=0;
	sensor3_num=0;
	sensor4_num=0;
	sensor5_num=0;
}

/********************************************************************************
 * 名称：                    send_all_package_to_upper_and_clear_sensor_num
 * 功能：                    ARM向上位机发送整个数据包，其中包括声呐数据和传感器数据等所有，然后清传感器条数
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void send_all_package_to_upper_and_clear_sensor_num(void)
{
	total_len = SIZE_OF_SEND_UPPER_PACKAGE_FIRST + ptr_send_to_upper_package_first->SonarDataLength + SIZE_OF_LEN_OF_SENSOR_DATA + ptr_send_to_upper_sensor->Senor_total_len + SIZE_OF_USHORT + SIZE_OF_LONG;
	//Debug("total_len: %d\n",  total_len); 
	//Debug("sonar_len: %d\n",   ptr_send_to_upper_package_first->SonarDataLength);
	//Debug("range: %d\n",  ptr_send_to_upper_package_first->Range);  

	// Debug("Senor_total_len: %d\n",  ptr_send_to_upper_sensor->Senor_total_len); 
	// Debug("GGA_ZDA_NUM: %d\n",  ptr_send_to_upper_sensor->GGA_ZDA_NUM); 

	/* while (Send_ss_status == 1)
	   {
	   usleep(10);
	   }*/
	if (Fpga_start_mod == 1) //开始
	{
		pthread_mutex_lock(&mut);
		// Send_data_status=1; 
		send_all_package_to_upper();
		pthread_mutex_unlock(&mut);  
		sensor1_num=0;
		sensor2_num=0;
		sensor3_num=0;
		sensor4_num=0;
		sensor5_num=0;    
	}
}

static void send_all_package_to_8001_and_clear_sensor_num(void)
{
	total_len = SIZE_OF_SEND_UPPER_PACKAGE_FIRST + ptr_send_to_upper_package_first->SonarDataLength + SIZE_OF_LEN_OF_SENSOR_DATA + ptr_send_to_upper_sensor->Senor_total_len + SIZE_OF_USHORT + SIZE_OF_LONG;

	if (Fpga_start_mod == 1) //开始
	{
		pthread_mutex_lock(&mut_8001);
		send_all_package_to_8001();
		sensor1_num=0;
		sensor2_num=0;
		sensor3_num=0;
		sensor4_num=0;
		sensor5_num=0;
		pthread_mutex_unlock(&mut_8001);
	}
}

/********************************************************************************
 * 名称：                    prepare_first_128_byte_data
 * 功能：                    准备ARM向上位机发送整个数据包中的前128个参数信息
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void prepare_first_128_byte_data(void)
{
	memcpy(ptr_fpga_frame_first, uio_share_mem_IQ.mem_ptr, SIZE_OF_DATA_FIRST); //先读取报头、采样点数、帧号、声学数据时间戳等信息
	//打印功率系数
	//Debug("power_factor_fpga: %d\n", ptr_fpga_frame_first->POWER_FACTOR); 
	ptr_send_to_upper_package_first->power_factor = (float)(ptr_fpga_frame_first->POWER_FACTOR>>16)/10.0f;//new add
	ptr_send_to_upper_package_first->TimeStamp  = (unsigned int)((float)ptr_fpga_frame_first->PPS_10ns/(FPGA_CLK_FREQUENCY/1000)); //时间戳单位是毫秒
	ptr_send_to_upper_package_first->PPS_number = ptr_fpga_frame_first->PPS_s;  //PPS 计数
	ptr_send_to_upper_package_first->PingCount = ptr_fpga_frame_first->FrameNumber; //帧号
	ptr_send_to_upper_package_first->DataType = (unsigned short)ptr_fpga_frame_first->Data_Type;
	ptr_send_to_upper_package_first->WorkMode = (unsigned short)ptr_fpga_frame_first->Work_Mode;
	ptr_send_to_upper_package_first->LFMMode = (unsigned short)ptr_fpga_frame_first->LFM_Mode;
	ptr_send_to_upper_package_first->PWMFreq = ptr_fpga_frame_first->PWM_FREQUENCY;
	ptr_send_to_upper_package_first->PWMBandWidth = ptr_fpga_frame_first->PWM_BAND_WIDTH;
	ptr_send_to_upper_package_first->Range = (unsigned short)ptr_fpga_frame_first->RANGE;
	ptr_send_to_upper_package_first->Sampling = (unsigned short)ptr_fpga_frame_first->SAMPLING_RATE;
	ptr_send_to_upper_package_first->PWMPulseWidth = (float)((float)ptr_fpga_frame_first->PWM_PULSE_WIDTH)/100.0f;
	ptr_send_to_upper_package_first->pwm_start = (unsigned short)ptr_fpga_frame_first->PWM_START;

	ptr_send_to_upper_package_first->FPGAVersion = ptr_fpga_register_data->fpga_sta.date;
	ptr_send_to_upper_package_first->LinuxDriverVersion = LINUX_VERSION;
	ptr_send_to_upper_package_first->ad_num = SIZE_OF_CHANNEL_NUM;
//	DBG("FPGAVersion = %d\n",ptr_send_to_upper_package_first->FPGAVersion);
//	DBG("linuxVersion = %d\n",ptr_send_to_upper_package_first->LinuxDriverVersion);
#if 0
	int i;
	for (i=0;i<16;i++) 
	{
		//ptr_send_to_upper_package_first->INIT_PHASE[i]=(float)((float)360*(ptr_fpga_frame_first->INIT_PHASE[i])* (float)ptr_fpga_frame_first->PWM_FREQUENCY*1000.00f/(float)FPGA_CLK_FREQUENCY);
		ptr_send_to_upper_package_first->INIT_PHASE[i]=(float)((float)(ptr_fpga_frame_first->INIT_PHASE[i])/4294967296* 360);
		//printf("init_phase[%d]=%f",i,(float)ptr_fpga_frame_first->INIT_PHASE[i]);
		//tmp[i] = (float)FPGA_CLK_FREQUENCY / (float)360 * ptr_recv_upper_package->pwm_ip[i]/ (float)ptr_recv_upper_package->PWMFreq /1000.00f;
	}
#endif
	//Debug("INIT_PHASE[2]: %f\n", ptr_send_to_upper_package_first->INIT_PHASE[2]);
	ptr_send_to_upper_package_first->AD_NUM = ptr_recv_upper_package->AD_NUM;
	ptr_send_to_upper_package_first->TransGear = (char)ptr_fpga_frame_first->TransGear;
	//ptr_send_to_upper_package_first->EPLD_VERSIONS=(unsigned int)ptr_fpga_frame_first->EPLD_VERSIONS;
	//printf("EPLD_VERSIONS=%d\n",(unsigned int)ptr_fpga_frame_first->EPLD_VERSIONS);
	// Debug("TRANSGear: %d\n", ptr_send_to_upper_package_first->TransGear);
	// memcpy(ptr_send_to_upper_package_first->INIT_PHASE, ptr_fpga_frame_first->INIT_PHASE, NUM_OF_IP * SIZE_OF_LONG);
	//ptr_send_to_upper_package_first->SonarDataLength = ((ptr_fpga_frame_first->AD_sn + 1)/ SAMPLE_FACTOR *SIZE_OF_CHANNEL_NUM * SIZE_OF_LONG); //声呐数据总数
	//ptr_send_to_upper_package_first->SonarDataLength = ((ptr_fpga_frame_first->AD_sn + 1) / *SIZE_OF_CHANNEL_NUM_ORIGINAL * SIZE_OF_USHORT); //声呐数据总数
	// memcpy(ddr_sonar_data+ SONAR_DATA_OFFSET -SIZE_OF_SEND_UPPER_PACKAGE_FIRST,ptr_send_to_upper_package_first,SIZE_OF_SEND_UPPER_PACKAGE_FIRST);//128字节的参数信息
}

/********************************************************************************
 * 名称：                    prepare_IQ_data_len
 * 功能：                    准备IQ数据的声呐数据长度，并拷贝IQ数据的前128字节
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void prepare_IQ_data_len(void)
{
	ptr_send_to_upper_package_first->SonarDataLength = ((ptr_fpga_frame_first->AD_sn + 1) / SAMPLE_FACTOR * SIZE_OF_CHANNEL_NUM * SIZE_OF_LONG); //声呐数据总数
//	ptr_send_to_upper_package_first->SonarDataLength = (((ptr_fpga_frame_first->AD_sn + 1) / SAMPLE_FACTOR * SIZE_OF_CHANNEL_NUM * SIZE_OF_LONG))/2; //声呐数据总数
	memcpy(ddr_sonar_data+ SONAR_DATA_OFFSET -SIZE_OF_SEND_UPPER_PACKAGE_FIRST,ptr_send_to_upper_package_first,SIZE_OF_SEND_UPPER_PACKAGE_FIRST);//128字节的参数信息
}

/********************************************************************************
 * 名称：                    prepare_original_data_len
 * 功能：                    准备原始数据的声呐数据长度，并拷贝IQ数据的前128字节
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void prepare_original_data_len(void)
{
	ptr_send_to_upper_package_first->SonarDataLength = ((ptr_fpga_frame_first->AD_sn + 1) *SIZE_OF_CHANNEL_NUM_ORIGINAL * SIZE_OF_USHORT); //声呐数据总数
	DBG("ptr_fpga_frame_first->AD_sn + 1:%dByte\n",ptr_fpga_frame_first->AD_sn + 1);
	DBG("SonarDataLength:%dByte\n",ptr_send_to_upper_package_first->SonarDataLength);
	//memcpy(ddr_sonar_data+ SONAR_DATA_OFFSET - SIZE_OF_SEND_UPPER_PACKAGE_FIRST,ptr_send_to_upper_package_first,SIZE_OF_SEND_UPPER_PACKAGE_FIRST);//128字节的参数信息
}

/********************************************************************************
 * 名称：                    read_fpga_send_to_upper
 * 功能：                    FPGA--->ARM--->上位机主线程
 * 入口参数：            	 无
 * 出口参数：            	 无
 *********************************************************************************/
void read_fpga_send_to_upper(void)
{
	unsigned icount;
	int err;
	double timeuse;
	while (1)
	{
		err = read(fd_uio6, &icount, 4); //进中断
		DBG("IRQ");
		write(fd_uio6, &irq_on, sizeof(irq_on));//清中断
		if (err != 4) 
		{
			perror("uio read err\n");
		}
		else
		{
			flag++;
			prepare_first_128_byte_data();
			if ((ptr_fpga_frame_first->FrameHead == 0xAAAAAAAA) && (ptr_fpga_frame_first->SmallFrameHead == 0xCCCCCCCC))//IQ数据
			{
				prepare_IQ_data_len();
				my_copy((unsigned char *)(ddr_sonar_data + SONAR_DATA_OFFSET), (unsigned char *)(uio_share_mem_IQ.mem_ptr + SIZE_OF_DATA_FIRST / 4), ptr_send_to_upper_package_first->SonarDataLength); 
				copy_sensor_data_from_pingpang_buf();
				copy_tail_to_sonar_data();
				if (is_upper_8001_connected())
				{
					DBG("To 8001 IQ FrameNumber:%d\n", ptr_fpga_frame_first->FrameNumber);
					send_all_package_to_8001_and_clear_sensor_num();
					DBG("send IQ data to 8001 over\n");
				}
				else
				{
					DBG("To 8000 IQ FrameNumber:%d\n", ptr_fpga_frame_first->FrameNumber);
					send_all_package_to_upper_and_clear_sensor_num();
					DBG("send IQ data to 8000 over\n");
				}
			} 
			else if ((ptr_fpga_frame_first->FrameHead == 0xAAAAAAAA) && (ptr_fpga_frame_first->SmallFrameHead == 0xBBBBBBBB))
			{
				//判断当前是否有8001端口连接，如果有8001端口连接，就将数据发送给8001端口 : 8001 调试端口
				if ((!is_upper_8001_connected()) && (cmd_package.DataType == 1))
				{
					DBG("Drop original frame after IQ config:%d\n", ptr_fpga_frame_first->FrameNumber);
					continue;
				}
				if(is_upper_8001_connected()){
					DBG("To 8001 :0x300000000_FrameNumber:%d\n", ptr_fpga_frame_first->FrameNumber);  
					//计算声学数据长度
					prepare_original_data_len();
					send_original_data_streaming_8001();
					DBG("send data to 8001 over\n");
				}else{
					DBG("To 8000 0x300000000_FrameNumber:%d\n", ptr_fpga_frame_first->FrameNumber);  
					//计算声学数据长度
					prepare_original_data_len();
					send_original_data_streaming();
					DBG("send data to 8000 over\n");
				}
				
			} 
			else
			{
				DBG("%d FrameHead error:%x\n", ptr_fpga_frame_first->FrameNumber, ptr_fpga_frame_first->FrameHead); 
			}
		} 
	}
}


