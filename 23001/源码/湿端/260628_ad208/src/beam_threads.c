#include "beam.h"

/********************************************************************************
 * 模块：线程创建和线程入口
 * 说明：由原 beam.c 按功能拆分，函数体保持原有逻辑。
 ********************************************************************************/

void set_fpga_send_to_upper_flag(){
	while(1){
		//判断状态标志位
		if(netStatus_8001 == 1){
			//printf("send_to_8001_flag %d\n",send_to_8001_flag);
			send_to_8001_flag = 1;
			usleep(1000);
		}else{
			send_to_8001_flag = 0;
			DBG("WAIT 8001 Client Link... ...\n");
			if ((Connect_fd_8001= accept(Socket_fd_server_8001, (struct sockaddr *)NULL, NULL)) == -1)
			{
				printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
				continue;
			}
			setkeepalive(Connect_fd_8001,5,1,5);
			set_send_timeout(Connect_fd_8001,2);
			netStatus_8001 = 1;
			send_to_8001_flag = 1;
			DBG("8001 Link DRY Successfull:Connect_fd_upper%d \r\n",Connect_fd_8001);
		}
	}
}

/********************************************************************************
 * 名称：                    thread_upper_to_fpga
 * 功能：                    线程：接收上位机，处理，发送给FPGA
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void *thread_upper_to_fpga(void *arg)
{
	(void)arg;
	receive_process_sendto_fpga();
	pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    thread_fpga_to_upper
 * 功能：                    线程：接收FPGA、传感器等外设，处理，发送给upper
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void *thread_fpga_to_upper(void *arg)
{
	(void)arg;
	read_fpga_send_to_upper();
	pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    *thread_sensor()
 * 功能：                    线程：温度传感器
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void *thread_sensor(void *arg)
{
	(void)arg;
	read_TMP451_sensor_and_save();
	pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    *thread_fpga_to_8001_upper()
 * 功能：                    线程：判断是否有未下放版显控接入网口，如果有未下放版显控连接，则原始数据会发送给未下放版本显控中
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void *thread_fpga_to_8001_upper(void *arg)
{
	(void)arg;
	set_fpga_send_to_upper_flag();
	pthread_exit(NULL);
}

/********************************************************************************
 * 名称：                    thread_create
 * 功能：                    创建两个线程
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void thread_create(void)
{
	int temp;
	memset(&thread, 0, sizeof(thread)); //comment1
	if((temp = pthread_create(&thread[0], NULL, thread_upper_to_fpga, NULL)) != 0){
		printf("upper to FPGA thread created fail!\n");
		
	}else{
		printf("upper to FPGA thread is created!\n");
	}
	if((temp = pthread_create(&thread[1], NULL, thread_fpga_to_upper, NULL)) != 0){
		printf("FPGA to UPPER thread created fail!\n");
	}else{
		printf("FPGA to UPPER thread is created!\n");
	}
	if ((temp = pthread_create(&thread[2], NULL, thread_sensor, NULL)) != 0){ //comment3
		printf("sensor thread created fail!\n");
	}else{
		printf("sensor thread is created!\n");
	}
	if ((temp = pthread_create(&thread[3], NULL, thread_fpga_to_8001_upper, NULL)) != 0){ //comment4
		printf("thread_fpga_to_8001_upper created fail!\n");
	}else{
		printf("thread_fpga_to_8001_upper is created!\n");
	}
}

/********************************************************************************
 * 名称：                    thread_wait
 * 功能：                    等待线程执行完
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
void thread_wait(void)
{
	if(thread[0] !=0)
	{
		pthread_join(thread[0],NULL);
		printf("upper to FPGA thread is over \n");
	}
	if(thread[1] !=0) 
	{  
		pthread_join(thread[1],NULL);
		printf("FPGA to upper thread is over \n");
	}
	if(thread[2] !=0) 
	{  
		pthread_join(thread[2],NULL);
		printf("sensor thread is over \n");
	}
	if(thread[3] !=0) 
	{  
		pthread_join(thread[3],NULL);
		printf("thread_fpga_to_8001_upper thread is over \n");
	}
}


