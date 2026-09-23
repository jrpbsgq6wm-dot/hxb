#include "beam.h"

/********************************************************************************
 * 名称：                    main
 * 功能：                    主函数
 * 入口参数：            	 无 
 * 出口参数：            	 无
 *********************************************************************************/
int main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN);
	fpga_interface_init();
	write(fd_uio6, &irq_on, sizeof(irq_on)); 
	//创建8000端口TCP服务器
	init_socket_server();
	printf("Create Tcp server : 8000");
	//创建8001端口TCP
	init_socket_server_8001();
	printf("Create Tcp server : 8001");
	printf("Init socker server suessfully\n");
	pthread_mutex_init(&mut,NULL); //初始化互斥锁
	pthread_mutex_init(&mut_8001,NULL);
	thread_create();
	printf("-------------------------------------\n");
	printf("-            S23001-WET             -\n");
	printf("-            2026-0628              -\n");
	printf("-------------------------------------\n");
	thread_wait();
	pthread_mutex_destroy(&mut);
	pthread_mutex_destroy(&mut_8001);
	return 0;
}
