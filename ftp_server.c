#include "server.h"


int
main()
{
	config_init(&conf);
//	fd = conf->logfd;
	pthreadpool pool;
	threadpool_init(&pool, conf->poolsize);
	threadpool_start(pool);	
	
	int listenfd;
	listenfd = tcp_init(conf->ip, conf->port);
	if(listenfd == -1)
	{
		exit(-1);	
	} 

	int ret ;
	ret = epoll_run(listenfd , pool);
	if(ret == -1)
	{
		exit(-1);
	}
	return 0; 
}								
