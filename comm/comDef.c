/*
 * 	File: comDef.c
 *
 *  Created on: 2021年03月20日
 *  Author: becking
 *  Copyright (C) 2021 si Valley <www.si-valley.com>
 */
#include "comDef.h"

Shm shm_send ={0}; //共享内存结构体
Shm shm_recv ={0}; //共享内存结构体

//1ms定时器
int set_timer(long tv_sec,long tv_usec)
{
	struct itimerval tv, oldtv;
    printf("Starting timer...\n");

    tv.it_interval.tv_sec = tv_sec;//0;
    tv.it_interval.tv_usec = tv_usec;//1000; //100ms
    tv.it_value.tv_sec = tv_sec;//0;
    tv.it_value.tv_usec = tv_usec;//1000;
    if (setitimer(ITIMER_REAL, &tv, &oldtv)) {
        fprintf(stderr, "Failed to start timer: %s\n", strerror(errno));
        return 1;
    }
	
	return 0;
}

/****************************共享内存********************************/

/*创建共享内存*/
void* init_shm(int *shmid, const char *pathname, int size)  //pathname=SHM_PATH_A
{
	void *shmaddr=NULL;
	
	key_t key = ftok(pathname,SHM_FLAG);
	if(-1 == key)
	{
		perror("shm_key ftok fail\n");
		return NULL;
	}
	
	*shmid = shmget((key_t)key,size,0666 | IPC_CREAT);
	if(0 > *shmid){
		//printf("%s:%d\n",__func__,__LINE__);
		return NULL;
	}

	*shmid = shmget((key_t)key,size,IPC_CREAT | IPC_EXCL | 0664);
	if(-1 == *shmid)
	{
		if(errno == EEXIST)
		{
			*shmid = shmget((key_t)key,size,0664);
		}
		else
		{
			perror("shmget fail \n");
			return NULL;
		}
	}

	shmaddr = shmat(*shmid,NULL,0);
	if(NULL == shmaddr)
	{
		perror("shmat fail \n");
		return NULL;
	}
    return shmaddr;
}

/*删除共享内存*/
int del_shm(void *shm)
{
    int rv;
	//删除本进程与共享内存的映射
    rv = shmdt(shm);
    if (rv == -1) {
        return rv;
    }
    return 0;
}

int destory_shm(int shmid)
{
	if(shmctl(shmid,IPC_RMID,NULL) < 0)
	{
		perror("shmctl fail! \n");	
		return -1;
	}
	return 0;
}

int shm_write(Shm shm_send,Shm_Queue_t *shared,sem_t *sem)  //shm_index=0 sem=sem_2
{
	if(shared->count < Q_SIZE-1){
		shared->shmBuf[(shared->index + shared->count + 1) % Q_SIZE] = shm_send;
		shared->count +=1;
	}
	sem_post(sem);
	return 0;
}

int shm_read(Shm *shm_recv,Shm_Queue_t *shared,sem_t *sem) //shm_index=1 sem=sem_1
{
	sem_wait(sem);
	if(shared->count > 0){
		*shm_recv = shared->shmBuf[shared->index];
		shared->index = (shared->index + 1) % Q_SIZE;
		shared->count -=1;
	}
	return 0;
}
