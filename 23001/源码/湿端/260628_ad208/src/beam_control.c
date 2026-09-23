#include "beam.h"

/********************************************************************************
 * 模块：8000主端口命令接收与配置下发
 * 说明：由原 beam.c 按功能拆分，函数体保持原有逻辑。
 ********************************************************************************/

/********************************************************************************
 * 名称：                    receive_process_sendto_fpga
 * 功能：                    上位机--->ARM--->FPGA主线程
* 入口参数：            	 无
* 出口参数：            	 无
*********************************************************************************/
void receive_process_sendto_fpga(void)
{
	int       len_recv;
	unsigned short      recv_crc_calculate,recv_crc;
	int len_recv_head1 ,len_recv_head2,len_recv_crc;
	char UPDATE[5];
	char DataHead1[2];
	char DataHead2[2];
	char  DataTail[4];//数据尾标识“ED>>”
	int update_flag,send_flag;
	char   RT[4]={'#','#','S','U'};
	char DEV_SYNC[3];
	char SyncPlus_Type;
	float SyncPlus_Delaytime;
	unsigned int sync_delay;
	unsigned int sync_state;
	while(1)
	{
		if (netStatus == 1)
		{
			DataHead1[0]=0;
			DataHead1[1]=0;
			while ((DataHead1[0]!='<')||(DataHead1[1]!='<'))
			{   
				len_recv_head1 = recv(Connect_fd, DataHead1, 2, 0);
				if (len_recv_head1 <= 0)
				{
					error_process();
					break;  
				}
			}  
			// printf("find << \n");
			if (netStatus == 0) 
			{
				continue;
			}
			len_recv_head2 = recv(Connect_fd, DataHead2, 2, 0);
			if (len_recv_head2 <= 0)
			{
				error_process();
				continue;  
			}

			printf("DataHead2: %c %c\n", DataHead2[0],DataHead2[1]);
			if (netStatus == 0) 
			{
				continue;
			}

			if ((DataHead2[0]=='F')&&(DataHead2[1]=='R'))//1.frame reset，重置当前波束数据的帧号
			{
				DBG("<<FR\n");
				// ptr_send_to_upper_package_first->PingCount = 0; 
				//这条协议暂时不考虑
			}
			if ((DataHead2[0]=='S')&&(DataHead2[1]=='R'))//2.Sonar Reset，清空声呐缓存，声呐恢复成初始状态
			{
				DBG("<<SR\n");
				memset(uio_share_mem_IQ.mem_ptr+SIZE_OF_DATA_FIRST , 0,  SIZE_OF_AD_DATA_MAX_BYTE ); 
			}
			if ((DataHead2[0]=='R')&&(DataHead2[1]=='S'))//3.request status，请求硬件工作状态信息指令(回复)
			{
				printf("RECV:<<RS\n");    
				pthread_mutex_lock(&mut);
				send_status_to_upper();
				pthread_mutex_unlock(&mut);
			}
			if ((DataHead2[0]=='U')&&(DataHead2[1]=='P'))//更新BOOT.BIN和image.ub
			{
				printf("<<UP\n");
				ptr_fpga_register_data->wsm_con=0;
				Fpga_start_mod = 0;
				clear_sensor_data();
				update_flag=0;
				
				//调用脚本执行固件升级，返回-1：更新失败
				pthread_mutex_lock(&mut);
				if(system("./run/updata_sys.sh") == 0)
				{    
					//发送协议头            
					if (send_all_bytes(Connect_fd, RT, 4) < 0) //4byte
					{       
						error_process();
						pthread_mutex_unlock(&mut);
						continue; 
					} 
					//发送标志位
					send_flag=1;	
					printf("ZYNQ UPdate success!\n");
					len_recv_head1 = send_all_bytes(Connect_fd, &((unsigned int){htonl((unsigned int)send_flag)}), 4);
					printf("len_recv_head1=%d,send_flag=%d\n",len_recv_head1,send_flag);	
					if (len_recv_head1 < 0) //4byte
					{       
						error_process();
						pthread_mutex_unlock(&mut);
						continue; 
					}
				}
				else 
				{
					//发送协议头            
					if (send_all_bytes(Connect_fd, RT, 4) < 0) //4byte
					{       
						error_process();
						pthread_mutex_unlock(&mut);
						continue; 
					} 
					send_flag=0;     
					if (send_all_bytes(Connect_fd, &send_flag, 4) < 0) //4byte
					{       
						error_process();
						pthread_mutex_unlock(&mut);
						continue; 
					} 
					printf("ZYNQ UPdate failed!\n");
				}
				pthread_mutex_unlock(&mut);
				ptr_fpga_register_data->wsm_con=1;
				Fpga_start_mod = 1;
			}
			if ((DataHead2[0]=='B')&&(DataHead2[1]=='R'))//4.Begin Request Beam Data，开始请求数据 (回复)
			{
				DBG("<<BR\n");
				ptr_fpga_register_data->wsm_con=1;
				Fpga_start_mod = 1;
			}
			if ((DataHead2[0]=='E')&&(DataHead2[1]=='R'))//5.End RequestBeam Data停止请求波形数据
			{
				DBG("<<ER\n");
				ptr_fpga_register_data->wsm_con=0;
				Fpga_start_mod = 0;
				clear_sensor_data();
			}
			if ((DataHead2[0]=='S')&&(DataHead2[1]=='P'))//6.set parameter，设置声呐的各项参数  (接收)
			{
				DBG("<<SP\n");
				len_recv = recv_socket(Connect_fd, (char *)(ptr_recv_upper_package), SIZE_OF_CONFIG_PARA);
				DBG("len_recv: %d\n",len_recv );
				if (len_recv < 0)
				{
					error_process();
					continue;  
				}
				len_recv_crc=recv_socket(Connect_fd,(char *)&recv_crc,SIZE_OF_LONG);
				// printf("len_recv_crc: %d\n",len_recv_crc);
				if (len_recv_crc < 0)
				{
					error_process();
					continue;
				}
				len_recv_head1 = recv_socket(Connect_fd, DataTail, SIZE_OF_LONG); 
				if (len_recv_head1 < 0)
				{
					error_process();
					continue;
				}
				DBG("DataTail:%c %c %c %c\n",DataTail[0],DataTail[1],DataTail[2],DataTail[3]);
				recv_crc_calculate= crc16((char *)(ptr_recv_upper_package),SIZE_OF_CONFIG_PARA,0xFFFF);
				if (recv_crc_calculate!=recv_crc)
				{
					printf("recv_crc_calculate:%x \n",recv_crc_calculate);
					printf("recv_crc:%x \n",recv_crc);
					printf("crc error! \n");
					continue;
				}
				copy_recv_cmd(&cmd_package, ptr_recv_upper_package);//拷贝到本地
				Debug_pritf_receive_para(&cmd_package);//打印
				if(parsing_instructions_200k(&cmd_package, &(fpga_register_data.fpga_registers_200k))) //解析fpga寄存器
				{
					continue;
				}
				Debug_pritf_convert_fpga_parameter(&(fpga_register_data.fpga_registers_200k));//打印
				int restart_after_config = (Fpga_start_mod == 1);
				if (restart_after_config)
				{
					ptr_fpga_register_data->wsm_con = 0;
					Fpga_start_mod = 0;
					usleep(1000);
					clear_sensor_data();
				}
				ptr_fpga_register_data->fpga_registers_200k= fpga_register_data.fpga_registers_200k;//下发FPGA参数配置命令 


				config_sensor_baud_and_framehead(&cmd_package);
				Debug_pritf_sensor_baud_and_framehead(&(fpga_register_data.fpga_sensor)); //打印
				ptr_fpga_register_data->fpga_sensor = fpga_register_data.fpga_sensor; //下发传感器参数配置命令 
	
				//下发TVG数据
				memset(uio_tvg_register.mem_ptr, 0, SIZE_OF_TVG_MAX); //剩余tvg置0 
				memcpy((char *)uio_tvg_register.mem_ptr, (char *)&cmd_package.tvgGain, fpga_register_data.fpga_registers_200k.dac_registers_value.dac_sn*4);//下发tvg数据            
				// memset(uio_tvg_register.mem_ptr + fpga_register_data.fpga_registers_200k.dac_registers_value.dac_sn, 0, (1000/ cmd_package.PingRate - fpga_register_data.fpga_registers_200k.dac_registers_value.dac_sn)*4);//剩余tvg置0 
				ptr_fpga_register_data->set_pr=1;
				usleep(1000);//1ms 
				ptr_fpga_register_data->set_pr=0;
				if (restart_after_config)
				{
					usleep(1000);
					ptr_fpga_register_data->wsm_con = 1;
					Fpga_start_mod = 1;
				}
			}
			if ((DataHead2[0] == 'S') && (DataHead2[1] == 'Y'))//设备同步输入输出命令
			{
				len_recv_head1 = recv(Connect_fd, DEV_SYNC, 3, 0);
				if (len_recv_head1 < 0)
				{
					error_process();
					continue;
				}
				if ((DEV_SYNC[0] == '_') && (DEV_SYNC[1] == 'I') && (DEV_SYNC[2] == 'N'))
				{
					DBG("<<SY_IN\n");
					syncstatus = 1;
					if(ptr_fpga_register_data->fpga_sta.wstatus == 0)
					{
						DBG("=======同步输入=======\n");
							if (send_locked_bytes(Connect_fd, "<<YS", 4) < 0) //4byte
							{   
								error_process();
								return;
							} 
					}
					len_recv_head1 = recv(Connect_fd, (char*)&SyncPlus_Type, 1, 0);
					if (len_recv_head1 < 0)
					{
						error_process();
						continue;
					}
					len_recv_head1 = recv(Connect_fd, (float*)&SyncPlus_Delaytime, 4, 0);
					if (len_recv_head1 < 0)
					{
						error_process();
						continue;
					}
					sync_state = 9; // sync_state & 0x00000000 | 0x00000005;//1001
					sync_delay = (int )(SyncPlus_Delaytime * 100);

					if((ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state>>4) == 0)
					{
						ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state = sync_state;
					}
					else
					{
						ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state = (sync_state | 16);
					}
					ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_selec = SyncPlus_Type;
					ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_delaytime = sync_delay;

					ptr_send_upper_status_information->sync_state = 1;
					ptr_send_upper_status_information->sync_selec = SyncPlus_Type;
					ptr_send_upper_status_information->sync_delaytime = SyncPlus_Delaytime;

					printf("syn_in SyncPlus_Type = %hhu\n", SyncPlus_Type);
					printf("syn_in SyncPlus_Delaytime = %f\n", SyncPlus_Delaytime);
					printf("syn_in sync_delay = %d\n", sync_delay);
					printf("syn_in sync_state = %d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state);

					ptr_fpga_register_data->set_pr = 1;
					usleep(1000);//1ms            
					ptr_fpga_register_data->set_pr = 0;

				}
				if ((DEV_SYNC[0] == '_') && (DEV_SYNC[1] == 'O') && (DEV_SYNC[2] == 'T'))
				{
					DBG("<<SY_OT\n");
					syncstatus = 1;
					if(ptr_fpga_register_data->fpga_sta.wstatus == 0)
					{
						DBG("=======同步输出=======\n");
							if (send_locked_bytes(Connect_fd, "<<YS", 4) < 0) //4byte
							{   
								error_process();
								return;
							} 
					}
					len_recv_head1 = recv(Connect_fd, (char*)&SyncPlus_Type, 1, 0);
					if (len_recv_head1 < 0)
					{
						error_process();
						continue;
					}
					len_recv_head1 = recv(Connect_fd, (float*)&SyncPlus_Delaytime, 4, 0);
					if (len_recv_head1 < 0)
					{
						error_process();
						continue;
					}
					sync_state = 10;// sync_state & 0x00000000 | 0x00000006;//1010
					sync_delay = (int )(SyncPlus_Delaytime * 100);

					if((ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state>>4) == 0)
					{
						ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state = sync_state;
					}
					else
					{
						ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state = (sync_state | 16);
					}
					ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_selec = SyncPlus_Type;
					ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_delaytime = sync_delay;

					ptr_send_upper_status_information->sync_state = 2;
					ptr_send_upper_status_information->sync_selec = SyncPlus_Type;
					ptr_send_upper_status_information->sync_delaytime = SyncPlus_Delaytime;

					printf("syn_out SyncPlus_Type = %hhu\n", SyncPlus_Type);
					printf("syn_out SyncPlus_Delaytime = %f\n", SyncPlus_Delaytime);
					printf("syn_out sync_delay = %d\n", sync_delay);
					printf("syn_out sync_state = %d\n", ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state);
					ptr_fpga_register_data->set_pr = 1;
					usleep(1000);//1ms            
					ptr_fpga_register_data->set_pr = 0;
				}
				if ((DEV_SYNC[0] == '_') && (DEV_SYNC[1] == 'N') && (DEV_SYNC[2] == 'O'))
				{
					DBG("<<SY_NO\n");
					syncstatus = 0;
					if(ptr_fpga_register_data->fpga_sta.wstatus == 0)
					{
						DBG("=======非同步=======\n");
							if (send_locked_bytes(Connect_fd, "<<YS", 4) < 0) //4byte
							{   
								error_process();
								return;
							} 
					}
					len_recv_head1 = recv(Connect_fd, (char*)&SyncPlus_Type, 1, 0);
					if (len_recv_head1 < 0)
					{
						error_process();
						continue;
					}
					len_recv_head1 = recv(Connect_fd, (float*)&SyncPlus_Delaytime, 4, 0);
					if (len_recv_head1 < 0)
					{
						error_process();
						continue;
					}
					sync_state = 0;//sync_state & 0x00000000;
					sync_delay = (int )(SyncPlus_Delaytime * 100);
					if((ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state>>4) == 0)
					{
						ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state = sync_state;
					}
					else
					{
						ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state = (sync_state | 16);
					}
					ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_selec = SyncPlus_Type;
					ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_delaytime = sync_delay;

					ptr_send_upper_status_information->sync_state = 0;

					printf("syn_no SyncPlus_Type = %x\n", SyncPlus_Type);
					printf("syn_no SyncPlus_Delaytime = %f\n", SyncPlus_Delaytime);
					printf("syn_no sync_delay = %d\n", sync_delay);
					printf("syn_no sync_state = %d\n",  ptr_fpga_register_data->fpga_registers_200k.pwm_registers_value.sync_state);
					ptr_fpga_register_data->set_pr = 1;
					usleep(1000);//1ms            
					ptr_fpga_register_data->set_pr = 0;
				}
				printf("======================================================\n");
				fpga_register_data.fpga_registers_200k = ptr_fpga_register_data->fpga_registers_200k;
				Debug_pritf_convert_fpga_parameter(&(fpga_register_data.fpga_registers_200k));//打印
			}
		}
		else
		{
			DBG("WAIT PC Link... ...\r\n");
			if ((Connect_fd= accept(Socket_fd_server, (struct sockaddr *)NULL, NULL)) == -1)
			{
				printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
				continue;
			}
			netStatus = 1;
			setkeepalive(Connect_fd,5,1,5);
			set_send_timeout(Connect_fd,5);
			DBG("PC Link DRY Successfull:Connect_fd_upper%d \r\n",Connect_fd);
		}
	}
}

