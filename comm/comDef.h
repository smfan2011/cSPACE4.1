/*
 * 	File: global.h
 *
 *  Created on: 2020年12月24日
 *  Author: becking
 */

#ifndef COMDEF_H
#define COMDEF_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h> //socket,bind,listen
#include <netinet/in.h>
#include <netinet/tcp.h> //man 7 ip
#include <arpa/inet.h> //inet_addr
#include <sys/ipc.h>   //ftok
#include <errno.h>     //perror
#include <sys/socket.h> //recv
#include <strings.h>
#include <sys/ioctl.h>  //ioctl
#include <sys/stat.h>   //open 
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h> //sem_post
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sys/shm.h>
#include <string.h>
#include <libgen.h>
#include "ecrt.h"
/**********************************/

/*数据类型定义*/
#define int8				signed char
#define int16				short
#define int32				int
#define uint8				unsigned char
#define	uint16				unsigned short
#define uint32				unsigned int

/*socket端口号和地址*/
#define PORT 11000
#define SERVER_ADDR "192.168.1.3"
#define CLIENT_ADDR "192.168.1.2"
#define SA struct sockaddr

/*消息队列key值路径，标识*/
#define MSG_SND_PATH "/etc"
#define MSG_RCV_PATH "./"
#define MSG_FLAG 'f'

/*消息队列类型*/
#define TYPE_RECV 100 	//接收IGH_FROM_ALG
#define TYPE_SEND 200   //发送IGH_TO_ALG

#define MSGLEN (sizeof(msg) - sizeof(long))

/*共享内存Key值路径，标识*/
#define SHM_PATH_A "/etc"
#define SHM_PATH_B "/bin"
#define SHM_FLAG 'f'
#define Q_SIZE	(128)

#define PERIOD_NS (1000000L)   //同步周期ms

#define ALG_CTRL		0       //1 算法控制,0 command control
#define SENSOR_6D		1		//6维力传感器
#define MECH_GRIP		0		//机械夹爪

#if MECH_GRIP
#define JOINTS_NUM		7			//支持关节数量
#else
#define JOINTS_NUM		6			//支持关节数量
#endif

#define OFFSET_VALUE 	100     	//每次偏移值

#define JOINTS_PULSE (131072/360)	//关节脉冲系数数

/**********************************/
typedef enum log_level{	
	ERROR=0,	
	INFO,
	WARN,
	DEBUG,	
	ALWAYS,	
}Log_level;

#define COLOR_NONE 				"\033[0m"
#define COLOR_RED               "\033[0;31m"
#define COLOR_GREEN             "\033[0;32m"
#define COLOR_YELLOW            "\033[1;33m"
#define COLOR_LIGHT_CYAN        "\033[1;36m"
#define COLOR_WHITE             "\033[1;37m"

#define PRINT_LEVEL ALWAYS
#ifndef PRINT_LEVEL
#define PRINT_LEVEL WARN
#endif

#define print_level(level,fmt,arg...) do {\
	if(level <= PRINT_LEVEL)\
	{\
		const char* pszColor = NULL;\
		switch(level)\
		{\
			case ERROR:\
				pszColor = COLOR_RED;\
				break;\
			case INFO:\
				pszColor = COLOR_LIGHT_CYAN;\
				break;\
			case WARN:\
				pszColor = COLOR_YELLOW;\
				break;\
			case DEBUG:\
				pszColor = COLOR_GREEN;\
				break;\
			default:\
				pszColor = COLOR_WHITE;\
				break;\
		}\
		printf("%s %s>%s:#%d, "COLOR_NONE" "fmt, pszColor, __FILE__, __FUNCTION__, __LINE__, ##arg);\
	}\
	} while(0)

 

/**********************************/
#define MALLOC(n, type) \
　　((type *) malloc((n)* sizeof(type)))

#define PRINT_USAGE(prg) \
	(fprintf(stderr,	\
		"Usage: %s [Options] <mode-type>\n" \
		"Options:\n"	\
		" --m, --modelLoop=1	model Loop (default = 1) 1:Position loop,2:Speed loop,3:Torque loop\n" \
		" --j, --joint=0	Motor ctrl joint index\n" \
		" --a, --angle=0	Motor ctrl joint angle\n" \
		" --h, --help		this help\n", prg))


int msgid[2];
#if 0
static struct option long_options[] = {
	{ "help",	no_argument,		0, 'h' },
	{ "modelLoop",	required_argument,	0, 'm' },
	{ "joint",	required_argument,	0, 'j' },
	{ "angle",	required_argument,	0, 'a' },
	{ 0,		0,			0, 0 },
};
#endif

/***************************PDO配置参数***************************************/
typedef struct  {
	uint8_t model;					//模式
	uint8_t jo[JOINTS_NUM];			//6个关节
	int16_t angle[JOINTS_NUM];	    //6个关节不同角度
}Motor_ctrl;

#define Q_SIZE	(128)

typedef enum model_type{
	MODEL_PP = 0x1,		//位置曲线模式
	MODEL_PV = 0x3,		//速度曲线模式
	MODEL_PT = 0x4,		//力矩模式
	MODEL_BZ = 0x6,		//回零模式
	MODEL_INP = 0x7,	//插补位置模式
	MODEL_CSP = 0x8,	//CSP:同步周期位置模式
	MODEL_CSV = 0x9,	//CSV: 同步周期速度模式
	MODEL_CST = 0xa		//CST：同步周期力矩模式
}Model_type;

typedef struct {
	unsigned int ctrl_word;   		// Contorl word  		   slave_6040_00
	unsigned int status_word;  		// status word   		   slave_6041_00
	unsigned int actual_pos;  		// Actual motor position   slave_6064_00
    unsigned int target_pos;  		// Profile target position slave_607a_00
    unsigned int pos_error;  		// Position loop error     slave_60f4_00
	unsigned int actual_vel;  		// Actual motor velocity   slave_606c_00
	unsigned int target_vel;  		// Target motor velocity   slave_60ff_00
    unsigned int offset_vel;  		// Velocity offset         slave_60b1_00
    unsigned int actual_torq;  		// Torque actual value     slave_6077_00
	unsigned int target_torq;  		// Target torque value     slave_6071_00
    unsigned int offset_torq;  		// Torque offset           slave_60b2_00

}Pdo_info;

typedef struct {
	Model_type model_type;	//控制模式
	Pdo_info pdo_val;		//PDO配置参数
	unsigned char error;			//错误码
	unsigned char status;			//状态
}Shm_info;

typedef struct {
    long type;
	Shm_info JointInfo[JOINTS_NUM];			//6个关节
}Shm;

typedef struct shm_queue {
	int index,count;
	Shm shmBuf[Q_SIZE];
}Shm_Queue_t;

Shm shm_send; //共享内存结构体
Shm shm_recv; //共享内存结构体

int set_timer(long tv_sec,long tv_usec);
void* init_shm(int *shmid, const char *pathname, int size);
int del_shm(void *shm);
int destory_shm(int shmid);
int shm_write(Shm shm_send,Shm_Queue_t *shared,sem_t *sem);
int shm_read(Shm *shm_recv,Shm_Queue_t *shared,sem_t *sem);

#endif
