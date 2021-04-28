#include  "comDef.h"
#include  "eth_ctrl.h"
#include  "net_recv.h"
#include  <sys/types.h>
#include  <sys/wait.h>
#include  "com_interface.h"

sem_t *sem_1;   //定义信号量
sem_t *sem_2;

int shmid[2];     //共享内存id
void *shmaddr[2]; //共享内存映射地址。
Shm_Queue_t *shared[2]; 
unsigned int isRunningFlag = 0;

static struct option long_options[] = {
        { "help",       no_argument,            0, 'h' },
        { "modelLoop",  required_argument,      0, 'm' },
        { "joint",      required_argument,      0, 'j' },
        { "angle",      required_argument,      0, 'a' },
        { 0,            0,                      0, 0 },
};


/*******************信号处理函数*************************/

void signal_handler (int signo)
{
	char cmd[100] = {0};

	switch(signo)
	{
		case SIGALRM:
			shm_write(shm_send,shared[0],sem_2);
		break;
		case SIGINT:
#if ALG_CTRL
			if(shmaddr[0] != NULL){
				if(shmdt(shmaddr[0]) < 0)
				{
					perror("shmdt fail! \n");
				}
				if(shmctl(shmid[0],IPC_RMID,NULL) < 0)
				{
					perror("shmctl fail! \n");	
				}
			}
			
			if(shmaddr[1] != NULL){
				if(shmdt(shmaddr[1]) < 0)
				{
					perror("shmdt fail! \n");
				}
				if(shmctl(shmid[1],IPC_RMID,NULL) < 0)
				{
					perror("shmctl fail! \n");	
				}
			}
			
			if(sem_1 != NULL){
				sem_close(sem_1);
				sem_unlink("sem_1");
			}
			if(sem_2 != NULL){
				sem_close(sem_2);
				sem_unlink("sem_2");
			}

			isRunningFlag = 0;
#endif
			EC_ReleaseMaster();
		        munlockall();
			printf("sudo killall -9 %s\n", g_old_file_name);
			memset(cmd, 0, sizeof(cmd));
                        sprintf(cmd,"sudo killall -9 %s", g_old_file_name);
                        system(cmd);
                        system("sudo killall -9 cSPACEServer4.1");


			/*
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd,"kill -9 %d",getpid());
			system(cmd);
			*/
		break;
	}

}

void killchild_handler (int signo)
{
	char cmd[1024];
	switch(signo)
	{
		case SIGHUP:
			close(connfd);

			//杀死上层算法进程
			if(g_old_file_name != NULL){
				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd,"killall -9 %s",g_old_file_name);
				//sprintf(cmd,"ps -ef | grep %s | grep -v 'grep' | awk '{print $2}' | xargs kill -9",g_old_file_name);
				system(cmd);
			}
			//杀死当前执行的进程
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd,"kill -9 %d",getpid());
			system(cmd);
                        system("sudo killall -9 cSPACEServer4.1");
		break;
	}
}


 static void my_split(char *src,const char *separator,char **dest,int *num) {

     char *pNext;
     int count = 0;
     if (src == NULL || strlen(src) == 0)
        return;
     if (separator == NULL || strlen(separator) == 0)
        return;
     pNext = (char *)strtok(src,separator);
     while(pNext != NULL) {
          *dest++ = pNext;
          ++count;
         pNext = (char *)strtok(NULL,separator);
    }  
    *num = count;
} 

static int para_cmdline(int argc, char **argv,Motor_ctrl *m_ctrl)
{
	int i, j, opt, index, num;
	
	char *revbuf[JOINTS_NUM] = {0}; //存放分割后的子字符串 
	int jo_an[JOINTS_NUM] = {0};
	
	for(i = 0;i < JOINTS_NUM; i++)
	{
		revbuf[i]=(char *)malloc(sizeof(char)*JOINTS_NUM);
	}
	
	memset(&m_ctrl->jo, 0xFF, sizeof(m_ctrl->jo));
	memset(&m_ctrl->angle, 0x0, sizeof(m_ctrl->angle));
	
	while ((opt = getopt_long(argc, argv, "h:m:j:a", long_options, NULL)) != -1) 
	{
		memset(&revbuf[0],0,JOINTS_NUM*sizeof(char *));
		switch (opt) {
		case 'h':
			PRINT_USAGE(basename(argv[0]));
			exit(0);

		case 'm':
			m_ctrl->model = strtoul(optarg, NULL, 0);
			printf("Motor_ctrl model = %d\n", m_ctrl->model);
			break;
		case 'j':
			my_split(optarg, ",", revbuf, &num);
			for(i = 0; i < JOINTS_NUM; i++) {
				if(revbuf[i] != NULL){
					index = strtoul(revbuf[i], NULL, 0);
					//printf("index = %d\n", index);
					m_ctrl->jo[index] = index;
				}
				printf("jo[%d] = %d\n", i, m_ctrl->jo[i]);
			}
			break;
		case 'a':
			my_split(optarg, ",", revbuf, &num);
			for(i = 0; i < JOINTS_NUM; i++) {
				if(revbuf[i] != NULL){
					jo_an[i] = strtoul(revbuf[i], NULL, 0);
				}
			}
			
			for(i = 0,j=0; i < JOINTS_NUM; i++){
				if(m_ctrl->jo[i] == 0xFF){
					//i++;
				}else{
					m_ctrl->angle[i] = jo_an[j];
					//i++;
					j++;
					printf("jo_an[%d] = %d ==> angle[%d] = %d\n", j, jo_an[j], i, m_ctrl->angle[i]);
				}
			}
			break;
		default:
			m_ctrl->model = MODEL_PP;
			//m_ctrl.angle = {0};
			break;
		}
	}
	
	return 0;
}

void ethercat_ctrl_cmd(void *arg)
{
	printf(">>ethercat_ctrl_cmd running:\n");
	Motor_ctrl m_ctrl;
	m_ctrl = *((Motor_ctrl *)arg);
	
/**********************EtherCat参数配置和启动*********************************/
	struct timespec wakeupTime, time;
	memset(&wakeupTime,0,sizeof(struct timespec));
	memset(&time,0,sizeof(struct timespec));
	
	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		perror("mlockall failed");
		return ;
	}
	
	g_MS_WorkStatus = MS_POWER_ON;
    if(g_MS_WorkStatus == MS_POWER_ON){
		EC_ActivateMaster();			        //激活主站
        g_MS_WorkStatus = MS_SAFE_MODE;
    }

    // get current time
    clock_gettime(CLOCK_TO_USE, &wakeupTime);

/*****************************************************************************/	
    memset(&msg_recv, 0,sizeof(Msg));
	memset(&msg_send, 0,sizeof(Msg));
	
	while(1)
	{	
		//sem_wait(&sem_2);
		
		//sem_post(&sem_1);
#if 0
		if(msgrcv(msgid[0], &msg_recv, sizeof(Msg_info)*JOINTS_NUM, TYPE_RECV, IPC_NOWAIT) < 0){			// 以非阻塞的方式接收类型为 type 的消息
			if (errno != ENOMSG) {				// 如果消息接收完毕就退出，否则报错并退出
				//printf(">>>>>>>msg_recv fail!");
			}
		}
#endif
		
		EC_CyclicTask(&wakeupTime, &time, &m_ctrl);
#if 0
		
		msg_send.type = TYPE_SEND;
		if(msgsnd(msgid[1], &msg_send, sizeof(Msg_info)*JOINTS_NUM, IPC_NOWAIT)<0){
			//printf(">>>>>>>msg_send fail!");
		}
#endif
	}
	EC_ReleaseMaster();
	munlockall();
	//pthread_exit(NULL);
}

//Motor_ctrl m_ctrl;
int main(int argc, char *argv[])
{
	pid_t pid;
	Motor_ctrl m_ctrl;

#if ALG_CTRL
	pid_t pid_child = getpid();
#endif
	
    pid = fork();
    if(pid<0)
    {
        perror("cannot fork");
        return -1;
    }
    else if(pid==0)//子进程
    {			
#if ALG_CTRL
		pid_child=getpid();
#endif
		signal(SIGHUP,killchild_handler);
		signal(SIGINT,signal_handler);
		FILE *fp;
		char file_name[FILENAME_SIZE]={0};
		uint32_t filename_len = 0;
		uint32_t file_len = 0;
		int recv_len = 0;
		uint32_t total_recv_len = 0;
		char *buffer;		//store file content
		int f_size = 0;
		char excutableFile[100] = {0};
			
		buffer = (char *)malloc(sizeof(char)*BUFFER_SIZE);
		bzero(buffer,BUFFER_SIZE); 
		
		//初始化TCP服务器
		tcp_serverInit();
		
		print_level(INFO,"等待客户端连接...\n");

		while(1)
		{			
			client_len = sizeof(struct sockaddr_in);
			client_sockfd = accept(sockfd,(SA *)&clientaddr,(socklen_t *)&client_len);
			if(-1 == client_sockfd)
			{
				//print_level(INFO,"no client accept!\n");
				continue;
			}

			// 打印客户端的ip和端口
			printf("client ip=%s, port=%d\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
			
			//读取第一包数据
			memset(file_name,'\0',sizeof(file_name));
			memset(buffer, 0, BUFFER_SIZE);
			f_size = recv(client_sockfd,buffer,BUFFER_SIZE,0);
			if(f_size < 0){
				print_level(ERROR,"read() filename failed ! error message:%s\n",strerror(errno));
			}else{
				//获取文件名长度
				if(!strncmp(buffer,"STOP",5)){
					print_level(INFO,"server recv:%s\n",buffer);
					close(client_sockfd);
					stop_process();
					break;
				}
				filename_len = getFileNameLen(buffer);
				if(filename_len > 255 || filename_len <= 0)
				{		
					goto __CLIENT_SOCKT;
				}
				//获取文件名
				getFileName(buffer, file_name, filename_len);
				print_level(INFO,"file_name:%s\n",file_name);
				//获取文件内容大小
				file_len = getFileContentTotalByte(buffer);
				//判断当前进程是否存在，存在则kill
				kill_process(file_name, g_old_file_name);
				//保存当前进程，确保下次接收到文件时当前进程被kill了
				memset(g_old_file_name, 0, FILENAME_SIZE);
				memcpy(g_old_file_name , file_name, strlen(file_name) - 4);
				strcat(g_old_file_name, ".elf");
				printf("%s:%d, g_old_file_name:%s\n", __FUNCTION__, __LINE__, g_old_file_name);
				total_recv_len = 0;
				//打开文件
				fp = fopen(file_name,"w");
				if(fp != NULL)
				{
					int times = 1;
					memset(buffer, 0, BUFFER_SIZE);
					while((recv_len = recv(client_sockfd,buffer,BUFFER_SIZE,0)) > 0 )
					{
						print_level(INFO,"[%d]recv_len = %d   ",times,recv_len);
						times++;
						if(recv_len < 0)
						{
							print_level(ERROR,"recv2 fail!\n");
							break;
						}
						total_recv_len += recv_len;
						fwrite(buffer,sizeof(char),recv_len,fp);

						if((recv_len < BUFFER_SIZE) && (total_recv_len >= (file_len-filename_len-20)))
						{
							print_level(INFO,"write successful!\n");
							break;
						}else{
							print_level(INFO,"file writing....\n");
						}
						bzero(buffer,BUFFER_SIZE); 
					}
					print_level(INFO,"recv finished!\n");
					fclose(fp);
				}else{
					continue;
				}
				
				//执行上传文件
				if(ELF_FILE == fileIsExcutable(file_name)){
					running_process(file_name);
				}else if(ZIP_FILE  == fileIsExcutable(file_name)){
					memset(excutableFile, 0, sizeof(excutableFile));
					unzip_compiler_process(file_name, excutableFile);
					running_process(excutableFile);

				}else{
					printf("Error:the upload file is not a zip file or a elf file!\n");
				}
			}
			
__CLIENT_SOCKT:	
			// 反馈结果
			//send(client_sockfd, recv_buf, len, 0);
			if ( close(client_sockfd) == -1 ) {
				perror("close failed");
				exit(EXIT_FAILURE);
			}
			printf("close client_sockfd=%d done\n", client_sockfd);
		}
		
		free(buffer);
		close(sockfd);         //关闭监听套接字
        exit(0);//子进程正常退出
    }
    else//父进程
    {
		signal(SIGALRM, signal_handler); 
		signal(SIGINT,signal_handler);
		//解析命令行
		para_cmdline(argc,argv,&m_ctrl);

#if ALG_CTRL
		/*********************************************************/
		shmaddr[0] = init_shm(&shmid[0],SHM_PATH_A,sizeof(Shm_Queue_t), SHM_FLAG_A);
		shmaddr[1] = init_shm(&shmid[1],SHM_PATH_B,sizeof(Shm_Queue_t), SHM_FLAG_B);
		
		shared[0]=(Shm_Queue_t *)shmaddr[0];
		shared[1]=(Shm_Queue_t *)shmaddr[1];

		sem_1=sem_open("sem_1",O_CREAT,0666,0);
		sem_2=sem_open("sem_2",O_CREAT,0666,0);	
		
		//1ms定时器
		set_timer(0,PERIOD_NS/1000);
	/**********************EtherCat参数配置和启动*********************************/
		struct timespec wakeupTime;
		struct timespec  time;
		memset(&wakeupTime,0,sizeof(struct timespec));
		memset(&time,0,sizeof(struct timespec));
		
		if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
			perror("mlockall failed");
			return -1;
		}
		
		g_MS_WorkStatus = MS_POWER_ON;
		if(g_MS_WorkStatus == MS_POWER_ON){
			EC_ActivateMaster();			        //激活主站
			g_MS_WorkStatus = MS_SAFE_MODE;
		}

		// get current time
		clock_gettime(CLOCK_TO_USE, &wakeupTime);

	/*****************************************************************************/	
		memset(&shm_recv, 0,sizeof(Shm));
		memset(&shm_send, 0,sizeof(Shm));
		isRunningFlag = 1;
		printf("%s:%d,isRunningFlag:%d\n",__FUNCTION__,__LINE__, isRunningFlag);
		while(isRunningFlag && !waitpid(pid, NULL, WNOHANG))
		{	
			sem_wait(sem_1);
			if(shared[1]->count > 0){
				shm_recv = shared[1]->shmBuf[shared[1]->index];
				shared[1]->index = (shared[1]->index + 1) % Q_SIZE;
				shared[1]->count -=1;	
			}
			clock_gettime(CLOCK_TO_USE, &wakeupTime);
			EC_CyclicTask(&wakeupTime, &time, &m_ctrl);
			
		}

		
		kill(pid_child,SIGHUP);
		EC_ReleaseMaster();
		munlockall();
#else

		ethercat_ctrl_cmd(&m_ctrl);
#endif

	}

    return 0;
}
