#include "server.h"

ssize_t sendn(int fd, char *buf, int size)
{
	char *pbuf = buf;
	int total , nsend;
	for(total = 0; total < size;)
	{
		nsend = send(fd, pbuf, size-total, 0);
		if(nsend <= 0)
		{
			if(nsend == -1 && errno == EINTR)
					continue;
			else
					return -1;
		}
		total += nsend;
		pbuf += nsend;
	}
	return total;
} 

ssize_t recvn(int fd, char *buf, int size)
{
	char *pbuf = buf;
	int total , nrecv;
	for(total = 0; total < size;)
	{
		nrecv = recv(fd, pbuf, size-total, 0);
		if(nrecv == 0)
			return total;
		if(nrecv == -1)
		{
			if(errno == EINTR)
				continue;
			else
				return -1;
		}
		total += nrecv;
		pbuf += nrecv;
	}
	return total;
}
void taskque_init(ptaskque pq)
{	
	pq->phead = NULL;
	pq->ptail = NULL;
	pthread_mutex_init(&pq->lock, NULL);
	pq->tasksize = 0;
}
	
void que_insert_client(pthreadpool pool, pclient pnew)
{
	do
	{
	if(&pool->clientque == NULL)
	{
		printf("task is null\n");
		break;
	}
	ptaskque pq = &pool->clientque;
		pthread_mutex_lock(&pq->lock);
		if(pool->status == 0)
		{
			printf("线程池已经关闭");
			pthread_mutex_unlock(&pq->lock);
			break;
		}
		if(pq->ptail == NULL)
		{
			pq->phead = pnew;
			pq->ptail = pnew;
		}else{
			pq->ptail->pnext = pnew;
			pq->ptail = pnew;
		}					
		pq->tasksize ++;
		pthread_cond_signal(&pool->notempty);
		pthread_mutex_unlock(&pq->lock);
	}while(0);	
}

void que_get_client(pthreadpool pool, pclient *pnew)
{
	do
	{
	if(&pool->clientque == NULL)
	{
		printf("que paq is null\n");
		break;
	}
	ptaskque pq = &pool->clientque;
		pthread_mutex_lock(&pq->lock);
		while(pq->tasksize == 0)
		{
			pthread_cond_wait(&pool->notempty, &pq->lock);
		}
		if(pool->status == 0)
		{	
			pthread_mutex_unlock(&pq->lock);
			printf("线程池已经关闭\n");
			break;
		}
		*pnew = pq->phead;
		pq->phead = pq->phead->pnext;
		pq->tasksize--;
		if(pq->tasksize == 0)
		{
			pq->ptail = NULL;
		
			pthread_cond_signal(&pool->empty);
		}
		pthread_mutex_unlock(&pq->lock);
	}while(0);
}

void config_init(pconfig* conf)
{
	char buf[512];
	bzero(buf, sizeof(buf));
	
	int ret;
	*conf = (pconfig)malloc(sizeof(config));
	if(NULL == conf)
	{	
		printf("config malloc lost");
		exit(-1);		
	}
	
	int fd = open(LOG_NAME, O_RDONLY);
	if(fd == -1)
	{
		perror("config open lost");
		exit(-1);	
	}
	
//	(*conf)->logfd = fd;
	ret = read(fd, buf, sizeof(buf));
	if(ret == -1)
	{
		perror("config read lost");
		exit(-1);
	}
	char *cur, *pre = buf;
	char temp[4][128] = {0};
	int i = 0;
	while(cur != NULL)
	{
		cur = strchr(pre, '\t');
		if(cur != NULL)	
		{
			strncpy(temp[i], pre , cur-pre);
			i++;
			pre = cur+1;
		}
	}
	strcpy(temp[i], pre);
	(*conf)->port = atoi(temp[0]);
	strcpy((*conf)->ip,temp[1]);
	(*conf)->poolsize = atoi(temp[2]);
	printf("config init port :%d ,ip: %s\n", (*conf)->port, (*conf)->ip);
}

void threadpool_init(pthreadpool* pool, int poolsize)//初始化线程池
{
	*pool = (pthreadpool)malloc(sizeof(threadpool));
	if(*pool == NULL)
	{
		printf("pthreadpool malloc lost\n");
		exit(-1);
	}
	(*pool)->pid =(pthread_t*)malloc(poolsize*sizeof(pthread_t));
	if((*pool)->pid == NULL)
	{
		printf("pthread_t malloc lost\n");
		exit(-1);
	}	
	taskque_init(&(*pool)->clientque);//	
	(*pool)->poolsize = poolsize;
	(*pool)->client_handle = thread_server;
	if(pthread_cond_init(&(*pool)->empty, NULL))
	{
		printf("pthread_cond_init empty lost\n");
		exit(-1);	
	}
	if(pthread_cond_init(&(*pool)->notempty, NULL))
	{
		printf("pthread_cond_init notempty lost");
		exit(-1);
	}	
	(*pool)->status = 0;
}

void threadpool_start(pthreadpool pool)//开启线程池
{
	int i = 0;
	//int num = pool->poolsize;
	for(;i < pool->poolsize; i++)
	{
		if(pthread_create(pool->pid + i, NULL, pool->client_handle, pool) != 0)
		{
			printf("pthread_create lost\n");
			exit(-1);
		}
	}
	pool->status = 1;
	printf("--------------pool started now-------------\n");
}

int tcp_init(const char* ip, int port)
{
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sfd == -1)
	{
		perror("tcp_init socket lost");
		return -1;
	}
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(struct sockaddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(ip);
	if(bind(sfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr)) == -1)
	{
		perror("tcp_init socket bind lost");
		close(sfd);
		return -1;
	}
	printf("bind success \n");
	if(listen(sfd, LISTENNUM) == -1)
	{
		perror("tcp_init socket listen lsot");
		close(sfd);
		return -1;
	}
	return sfd;
} 

int tcp_accept(int sfd, pclient pnew)
{
	char buf[1024] = {0};
	struct sockaddr_in clientaddr;
	
	memset(&clientaddr, 0, sizeof(struct sockaddr));
	int addrlen = sizeof(struct sockaddr);
	int new_fd = accept(sfd, (struct sockaddr*)&clientaddr, &addrlen);
	if(new_fd == -1)
	{
		perror("tcp_accept accept lost");
		close(sfd);
		return -1;
	}
	printf("%s %d success connect ..\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	pnew->cfd = new_fd;

	return new_fd;
}
void* thread_server(void *args)
{	
	pthreadpool pool = (pthreadpool)args;
	pclient pget = NULL;
	int status = 1;
	int ret;

		char cmdline[300] = {0};	
		int len;
		char *cmd;
	while(1)
	{
		if(pool == NULL)
		{
			printf("thread_ssrver pool is null\n");
			break;
		}

		que_get_client(pool, &pget);
		if(pget == NULL)
		{		
				printf("线程获取client lost\n");
				break;
		}	
		printf("thread-server取得任务\n");
	//	recvn(pget->cfd, (char *)&status , sizeof(status));

//		if(status == 0)
//		{
///			int user_id = client_log(pget->cfd);
//			if(user_id == /-1)
//			{
///				free(pget);
//				pget = NULL;
///				break;
//			}
//		}
		

		printf("得到的 pget : %d\n", pget->cfd);
		while(1)
		{	
			printf("通信符 %d\n", pget->cfd);
			
			ret = recvn(pget->cfd, (char *)&len, sizeof(int));
			if(ret == -1)
				{
					printf("recv lost thread\n");
					break;
				}
			printf("命令 长度 ：%d\n", len);

			if(recvn(pget->cfd, cmdline, len) == -1)
			{
				perror("server 接受命令失败");
				pthread_exit(NULL);
			}
			printf("命令:%s\n",cmdline);

			cmd = strtok(cmdline,  "\t");	
			if(cmd == NULL)
			{
				printf("no cmd\n");
				break;

				
			}
			printf("cmd:%s\n", cmd);
			int val = 0;
			val = cmd_val(cmd);		
			switch(val)
			{
				case 1://ls
						break;
				case 2://cd
						break;
				case 3://pwd
						break;
				case 4://rm
						break;
				case 5://gets
						break;
				case 6://puts
						break;
				case 7:
						file_size(pget->cfd);
						break;
				case 8:
						rest_file(pget->cfd);
						break;
				case 9:
						break;
				default:
						printf("命令不知名----\n");
						break;
			}

		}
		free(pget);
		pget = NULL;
		
	}

}
	
void file_size(int sfd)
{
	int ret;
	char *filename;
	do{

	filename = strtok(NULL, "\t");
	if(filename == NULL)
	{
		printf("文件名为空\n");
		break;
	}
	struct stat st;
	ret =  stat(filename, &st);
	if(ret == -1)
	{
		perror("stat");
		break;
	}
	int filesize = st.st_size;
	sendn(sfd, (char *)&filesize, sizeof(int));
	}while(0);
}
void rest_file(int sfd)
{
	char *filename;
	char *filesize;
	char *fileoffset;

	int size;
	int offset;
	do
	{
		filename = strtok(NULL, "\t");
		if(filename == NULL)
		
		{
			printf("文件名为空\n");
			break;	
		}
		filesize = strtok(NULL, "\t");
		if(filesize == NULL)
		{
			printf("文件名为空\n");
			break;
		}
		fileoffset = strtok(NULL, "\t");
		if(fileoffset == NULL)
		{
			printf("fileoffset 为空\n");
			break;
		}

		size = atoi(filesize);
		offset = atoi(fileoffset);
		printf("down filename %s size :%d, offset %d\n",filename, size, offset);	
		int fd = open(filename, O_RDWR);
		if(-1 == fd)
		{
			perror("open");
			break;
		}
		char * addr = mmap(NULL,size, PROT_READ, MAP_SHARED, fd, offset);
		if(addr == NULL || addr == (void *)-1)
		{
			perror("mmap lost");
			break;
		}
		close(fd);
		int total = 0;
		int len;
		char buf[1000];
		while(total < size)
		{
			bzero(buf, 1000);
			memcpy(buf, addr, 1000);
			len = strlen(buf);
			sendn(sfd, (char *)&len, sizeof(int));
		
			sendn(sfd, buf, len);
			addr += len;
			total += len;		
		}
		munmap(addr, size);

	}while(0);
}
int cmd_val(char *cmd)
{
	if(strcmp("ls", cmd)==0)	
	{
			return 1;	
	}
	else
	if(strcmp("cd", cmd) == 0)
	{
			return 2;
	}
	else
	if(strcmp("pwd", cmd)==0)	
	{
			return 3;
	}
	else
	if(strcmp("gets", cmd)==0)	
	{
		return 5;
	}
	else	
	if(strcmp("puts", cmd)==0)	
	{
		return 6;
	}
	else
	if(strcmp("rm", cmd)==0)	
	{
		return 4;
	}
	if(strcmp("SIZE", cmd) == 0)
	{
	
		return 7;
	}
	if(strcmp("REST", cmd) == 0)
	{

		return 8;
	}
	else
		return 0;
}
void get_salt(char *salt, char *passwd)
{
	int i, j;
	
	for(i = 0,j = 0; passwd[i] && j != 3; ++i)
	{
		if(passwd[i] == '$')
			++j;
	}
	strncpy(salt, passwd, i-1);
}


int  client_log(int sfd)//客户登录验证
{
	char user[20] = {0};	
	char passwd[20] = {0};
	struct spwd *sp;
	char salt[512] = {0};
	int len;

	recvn(sfd, (char*)&len, sizeof(int));

	recvn(sfd, user, len);

	printf("username :%s\n", user);
	
	if((sp = getspnam(user)) == NULL)
	{
		printf("getspanam lost\n");
		return -1;
	}

	struct passwd* userwd;
	userwd =(struct passwd*) malloc(sizeof(passwd));
	userwd = getpwnam(sp->sp_namp);
	int user_id = userwd->pw_uid;
	free(userwd);	
	get_salt(salt, sp->sp_pwdp);
	recvn(sfd, (char *)&len, sizeof(int));
 
	recvn(sfd, passwd, len);
	printf("passwd: %s\n", passwd);

	int flag = 0;
	if(strcmp((sp->sp_pwdp),(crypt(passwd, salt))) == 0)
	{
		printf("验证通过\n");
		flag = 1;
		sendn(sfd, (char *)&flag, sizeof(int));
		return user_id;
	}
	else
	{	
		sendn(sfd, (char *)&flag, sizeof(int));
		printf("验证失败\n");
		return -1;
	}
}


int epoll_run(int sfd, pthreadpool pool)
{	 
	int epfd = epoll_create(1);
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = sfd;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &event) < 0)
	{
		perror("epoll_run epoll_ctl lost");
		return -1;
	}
	pclient pnew;
	int true = 1;
	do	
	{
				if(epoll_wait(epfd, &event ,1 , -1) > 0)
				{
					if(event.events == EPOLLIN && event.data.fd == sfd)
					{
						pnew = (pclient)malloc(sizeof(client));
						if(pnew == NULL)
						{
							perror("epoll_run pclient malloc lost");
							return -1;
						}
						if(tcp_accept(sfd, pnew) == -1)
						{
							return -1;
						}
						if(pool == NULL )
						{
							printf("pool is NUL\n");
							break;
						}
						que_insert_client(pool, pnew);
						if(0)//////有待于写 退出形况 有可能 accpet 被中断。 返回 EINTER
						{
							true = 0;
						}
						
					}
				}
	}while(true);
}
							
