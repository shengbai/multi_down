#ifndef __SERVER_H__
#define __SERVER_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <strings.h>
#include <time.h>
#include <grp.h>
#include <dirent.h>
#include <pwd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <shadow.h>
#include <sys/mman.h>

typedef void *(*pfunc)(void *);
typedef struct{
	int port;
	char ip[20];
	int poolsize;
}config, *pconfig;

typedef struct client{
    int cfd;
	int port;
    struct client* pnext;
}client, *pclient;

typedef struct{
    pclient phead, ptail;
    int tasksize;
    pthread_mutex_t lock;
}taskque, *ptaskque;

typedef struct{
    pthread_t *pid;
    taskque clientque;
    int poolsize;
    pfunc client_handle;
    int status;
    pthread_cond_t empty;
    pthread_cond_t notempty;
}threadpool, *pthreadpool;

#define LISTENNUM 20
#define LOG_NAME "worklog.txt"

pconfig conf;
int fd;
ssize_t sendn(int ,char *, int);
ssize_t recvn(int , char* ,int);

int tcp_init(const char *, int );
int tcp_accept(int , pclient);

void threadpool_init(pthreadpool* , int);
void threadpool_start(pthreadpool);
void config_init(pconfig *);

void* thread_server(void *);
int client_log(int );
int epoll_run(int , pthreadpool);
int cmd_val(char *cmd);
void get_salt(char *salt, char *passwd);
void file_size(int);
void rest_file(int);

void taskque_init(ptaskque);
void que_insert_client(pthreadpool , pclient);
void que_get_client(pthreadpool, pclient *);
#endif

