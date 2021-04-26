/*
 * 	File: net_recv.h
 *
 *  Created on: 2021年03月31日
 *  Author: becking
 *  Copyright (C) 2021 si Valley <www.si-valley.com>
 */

#ifndef NET_RECV_H
#define NET_RECV_H
#include "comDef.h"

#define BUFFER_SIZE		4*1024
#define FILENAME_SIZE	100


int connfd;		//udp接收文件描述符
FILE *fp; 		//保存时间

static char g_old_file_name[100]={0};

struct sockaddr_in serveraddr;
struct sockaddr_in clientaddr;
int sockfd,client_sockfd;
int client_len;

//tcp服务器初始化
int tcp_serverInit(void);
//获取QT客户端文件名长度，4字节
uint32_t getFileNameLen(void *buf);
//获取文件名称,注意QT传过来的每个字符都是16位的
void getFileName(void *buf, char *filename, uint32_t filename_len);
//获取QT客户端文件内容长度，8字节，注意需要减去30
uint32_t getFileContentTotalByte(void *buf);
//kill进程
void stop_process();
void kill_process(char *file_name, char *old_file_name);
//运行进程
void running_process(char *file_name);

#endif /* NET_RECV_H */
