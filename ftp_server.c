#include "server.h"


int
main()
{
//	pconfig conf;
	config_init(conf);
	
	fd = conf->logfd;
	
//	pthread_mutex_init(&writelock, NULL);	
		
	pthreadpool pool;
	threadpool_init(pool, conf->poolsize);
	threadpool_start(pool);	
	
/*	int workfd;
	workfd = worklog_init(conf->logpath);
	if(workfd == -1)
	{
		exit(-1);
	}
*/
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
	 
//	threadpool_destroy(pool);
}								
