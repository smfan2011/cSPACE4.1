/*
 * 	File: net_recv.c
 *
 *  Created on: 2021年03月31日
 *  Author: becking
 *  Copyright (C) 2021 si Valley <www.si-valley.com>
 */
 
#include "net_recv.h" 
#include "com_interface.h"
#include <string.h>

//设置sock为non-blocking mode
void setSockNonBlock(int sock) {
	int flags;
	flags = fcntl(sock, F_GETFL, 0);
	if (flags < 0) {
		perror("fcntl(F_GETFL) failed");
		exit(EXIT_FAILURE);
	}
	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		perror("fcntl(F_SETFL) failed");
		exit(EXIT_FAILURE);
	}
}

/*************TCP服务器初始化*********************/
int tcp_serverInit(void)
{
	print_level(INFO,"服务器创建socket...\n");
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(-1 == sockfd)
	{
		print_level(ERROR,"creat socket fail\n");
		return -1;
	}
	//重用sockfd
    int on = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        print_level(ERROR,"setsockopt() failed ! error message:%s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
	
	//设置sock为non-blocking
    setSockNonBlock(sockfd);
	
	print_level(INFO,"准备填充地址...\n");
	memset(&serveraddr,0,sizeof(serveraddr));  //清空serveraddr结构体
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(PORT);
	//serveraddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	print_level(INFO,"绑定socket与地址...\n");
	if(-1 == bind(sockfd,(SA *)&serveraddr,sizeof(serveraddr)))
	{
		close(sockfd);
		print_level(ERROR,"bind fail\n");
		return -2;
	}
	print_level(INFO,"设置监听...\n");
	if(listen(sockfd,5))
	{
		close(sockfd);
		print_level(ERROR,"listen");
		return -1;
	}

	return 0;
}

//获取QT客户端文件名长度，4字节
uint32_t getFileNameLen(void *buf)
{
	uint32_t filename_len = 0;
	char *recv_buf = (char *)buf;
	for(int i=0; i < 4; i++){
		filename_len |= ((recv_buf[i+16]&0xFF) << (24-i*8));
	}

	print_level(INFO,"filename_len:%d\n", filename_len);
	
	return filename_len;
}

//获取文件名称,注意QT传过来的每个字符都是16位的
void getFileName(void *buf, char *filename, uint32_t filename_len)
{
	int i,j;
	char *recv_buf = (char *)buf;
	for(i = 0,j=0; i < filename_len;j++,i+=2){
		filename[j] = recv_buf[i+21];
	}
	filename[filename_len/2+1] = '\0';
}

//获取QT客户端文件内容长度，8字节，注意需要减去30
uint32_t getFileContentTotalByte(void *buf)
{
	uint32_t file_len =0;
	char *recv_buf = (char *)buf;
	for(int i = 0; i < 4; i++){
		file_len |= ((recv_buf[i+4]&0xFF) << (24-i*8));
	}
	printf("file_len:%d\n", file_len);

	return file_len-30;
}

//kill进程
void stop_process()
{
	FILE *p_fp = NULL;
	char cmd[1024]={0};
	if(g_old_file_name != NULL){
		sprintf(cmd, "ps -ef | grep -w %s | wc -l", g_old_file_name); 
		if((p_fp = popen(cmd,"r")) != NULL) 
		{ 
			print_level(INFO,"process %s exist!!\n",g_old_file_name);
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd,"killall -9 %s",g_old_file_name);
			//sprintf(cmd,"ps -ef | grep %s | grep -v 'grep' | awk '{print $2}' | xargs kill -9",g_old_file_name);
			//杀死当前执行的进程
			system(cmd);
			fclose(p_fp);
		} 
	}
}

void kill_process(char *file_name, char *old_file_name)
{
	FILE *p_fp = NULL;
	char cmd[1024] = {0};
	char suffix[20] = {0};
	char file_name_tmp[200] = {0};
	char old_file_name_tmp[200] = {0};
	unsigned int fileLen = 0;
	unsigned int oldFileLen = 0;

	if(old_file_name == NULL || file_name == NULL){
		return;
	}

	printf("%s:line:%d, filename:%s, old_file_name:%s\n", __FUNCTION__,__LINE__, file_name, old_file_name);

	fileLen = strlen(file_name);
	memcpy(file_name_tmp, file_name, fileLen);

	oldFileLen = strlen(old_file_name);
	if(oldFileLen > 0){
		memcpy(old_file_name_tmp, old_file_name, oldFileLen);
		getFileNameSuffix(old_file_name_tmp, suffix);
		if(strcmp(suffix , ".zip") == 0)
		{
			old_file_name_tmp[oldFileLen - 1] = 'f';
			old_file_name_tmp[oldFileLen - 2] = 'l';
			old_file_name_tmp[oldFileLen - 3] = 'e';
			old_file_name_tmp[oldFileLen - 4] = '.';
		}
	}

	memset(suffix, 0 , sizeof(suffix));
	getFileNameSuffix(file_name_tmp, suffix);
	if(strcmp(suffix , ".zip") == 0)
	{
		file_name_tmp[fileLen - 1] = 'f';
		file_name_tmp[fileLen - 2] = 'l';
		file_name_tmp[fileLen - 3] = 'e';
		file_name_tmp[fileLen - 4] = '.';
	}


	if(memcmp(file_name_tmp, old_file_name_tmp, FILENAME_SIZE)!= 0)
	{
		sprintf(cmd, "ps -ef | grep -w %s | wc -l", old_file_name_tmp); 
		if((p_fp = popen(cmd,"r")) != NULL) 
		{ 
			print_level(INFO,"1>process %s exist!!\n",old_file_name_tmp);
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd,"sudo killall -9 %s",old_file_name_tmp);
			//sprintf(cmd,"ps -ef | grep %s | grep -v 'grep' | awk '{print $2}' | xargs kill -9",old_file_name);
			//杀死当前执行的进程
			system(cmd);
			fclose(p_fp);
		} 
	}
	sprintf(cmd, "ps -ef | grep -w %s | wc -l", file_name_tmp); 
	if((p_fp = popen(cmd,"r")) != NULL)
	{ 
		print_level(INFO,"2>process %s exist!!\n",file_name_tmp);
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"sudo killall -9 %s",file_name_tmp);
		//sprintf(cmd,"ps -ef | grep %s | grep -v 'grep' | awk '{print $2}' | xargs kill -9",file_name);
		//杀死当前执行的进程
		system(cmd);
		fclose(p_fp);
	} 
}
//运行进程
void running_process(char *file_name)
{
	char cmd[64]={0}; 
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd,"chmod a+x %s",file_name);
	system(cmd);
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd,"./%s &",file_name);
	system(cmd);
}

