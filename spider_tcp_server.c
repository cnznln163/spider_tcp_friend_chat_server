/**
*@auth JasonChen@boyaa.com
*@date 2013-11-10
*/

#include "spider_tcp_server.h"
#include "spider_function.h"
pthread_t t_epoll;
pthread_t t_serv_heart;
void serv_heartbeat(void * args);
void serv_epoll(void * args);
fd2client *fd2clients;

int main(){
	server s;
	struct rlimit rt;
	rt.rlim_max=rt.rlim_cur= 102400;
	if( setrlimit( RLIMIT_NOFILE , &rt ) == -1 ){
			perror( "set rlimit fail" );
			exit(-1);
	}

	
	/******************************/
	/*-------初始化线程锁---------*/
	/******************************/
	pthread_mutex_init( &t_mutex , NULL );
	pthread_cond_init( &t_cond , NULL );
	pthread_mutex_init( &t_mutex_log , NULL );

	/******************************/
	/*-------开启心跳程序---------*/
	/******************************/
	pthread_create( &t_serv_heart , NULL , (void *)serv_heartbeat , NULL );

	/******************************/
	/*-------初始化任务线程，开启两个线程来完成任务，线程之间会互斥地访问任务链表---------*/
	/******************************/
	t_num = 0;
    while (t_num < 2  && t_num < TASK_MAX ) {
        t_num++;
        pthread_create(&tid[t_num], NULL, (void *)readtask, NULL);
    }

	/******************************/
	/***********初始化Mysql********/
	/******************************/
	if(!initMysql()){
		log_write( LOG_ERR , "init mysql fail \n" );
	}
	log_write( LOG_ALERT , "init mysql successful\n" );
	/******************************/
	/*-------初始化socket---------*/
	/******************************/

	// 创建epoll
	epfd  = epoll_create( MAX_FDS );

	int optval = 1; // 关闭之后重用socket
    unsigned int optlen = sizeof (optval);
	//	创建socket，并绑定到固定端口
	s.listen_sockfd	=	socket( AF_INET , SOCK_STREAM , 0 );
	if( s.listen_sockfd < 0 ){
		log_write( LOG_ERR ," 创建socket失败 , %s , %d " , __FILE__ , __LINE__ );
		exit(-1);
	}
	setsockopt(s.listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optlen)); //端口重用，如意外没有释放端口，还可以绑定成功
	bzero(&s.listen_addr, sizeof (s.listen_addr));
	s.listen_addr.sin_family = AF_INET;
    s.listen_addr.sin_port = htons(SERVER_PORT);
    s.listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s.listen_sockfd, (struct sockaddr*) & s.listen_addr, sizeof (s.listen_addr)) < 0) {
        log_write(LOG_ERR, "bind error, %s, %d", __FILE__, __LINE__);
        exit(-1);
    }

	if (listen(s.listen_sockfd, 3) < 0) {
        log_write(LOG_ERR, "listen error, %s, %d", __FILE__, __LINE__);
        exit(-1);
    }
	log_write(LOG_DEBUG, "监听端口:%d......\n", SERVER_PORT);
    log_write(LOG_DEBUG, "线程数:%d......\n", t_num);

	if (pthread_create(&t_epoll, NULL, (void *)serv_epoll, NULL) != 0) {
        log_write(LOG_ERR, "serv_epoll pthread_create error, %s, %d", __FILE__, __LINE__);
//        destory();
        exit(-1);
    }
	
	int client_fd;
	struct sockaddr_in client_addr;
	socklen_t sock_len	=	sizeof( client_addr );
	struct epoll_event ev = {0};
	//	初始化fd2clients
	fd2clients=NULL;
	init_fd2client();
	//	accept
	while( (client_fd = accept( s.listen_sockfd , ( struct sockaddr* )&client_addr , &sock_len )) > 0 ){
		 set_nonblocking_socket(client_fd);
		 spider_client *node 	= 	(spider_client *)malloc( sizeof( spider_client ) );
		 node->fd				=	client_fd;
		 node->uid				=	0;
		 node->gmid				=	0;
		 node->gameid			=	0;
		 memset(node->app_version , 0  , sizeof(node->app_version));
		 //	获取当前时间
		 time_t now;
		 time(&now);
		 node->keepalivetime	=	now;
		 node->connect_status	=	CONNECT_STATUS_ACCPET;
		 //	初始化线程锁
		 pthread_mutex_init( &node->client_mutex , NULL );
		 add_fd2client( client_fd , node);
		 ev.events				=	EPOLLIN | EPOLLET;
		 ev.data.fd			=	client_fd;
		 epoll_ctl(epfd , EPOLL_CTL_ADD , client_fd , &ev);
		 log_write(LOG_DEBUG ,"新连接:fd-%d\n", client_fd );
	}
	close(s.listen_sockfd);
	destory();
	return -1;
}



/****************************************/
/*------------EPOLL线程----------*/
/****************************************/
void serv_epoll(void * args){
	int nfds;
	packet_status *head;
	spider_client *c;
	struct epoll_event events[MAX_EVENTS];
	struct epoll_event ev;
	int event_fd;
	char *data;
	int packet_len=0;
	int i;
	while(1){
		//等待epoll事件的发生
        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1); //-1 一直等到有数据到达
        for(i=0; i<nfds;i++){
			time(&mytimestamp);
			p = gmtime(&mytimestamp);
			log_write(LOG_DEBUG, "serv_epoll fd:%d, file:%s, line:%d\n", events[i].data.fd, __FILE__, __LINE__);
			event_fd = events[i].data.fd;
			if( events[i].events & EPOLLIN  ){	//	读操作-有客户端发消息过来
				if( event_fd < 0 ){
					continue;
				}
				c  = is_exists_fd( event_fd );
				if( c == NULL ){
					events[i].data.fd	=	-1; //#TODO
					epoll_ctl( epfd , EPOLL_CTL_DEL , event_fd , NULL );
					close(event_fd);
					continue;
				}

				//	读取数据
				head	=	read_head( event_fd );
				if( head->status == 0 && head->v.return_value <= -7 ){	//	client close shutdown this fd
					events[i].data.fd	=	-1;
					epoll_ctl( epfd , EPOLL_CTL_DEL , event_fd , NULL );
					close_client(event_fd ,c->uid ,c->gmid ,c->gameid );
					free( head );
					head = NULL;
					continue;
				}else if( head->status == 0 ){
					free(head);
					head = NULL;
					continue;
				}
				//	读取数据
				int body_len;
				// 申请内存存放
				data	=	(char *)malloc( head->v.packet->length+1);
				bzero( data , head->v.packet->length+1  );
				body_len	=	recv( event_fd , data , head->v.packet->length , 0 );
				if( body_len != head->v.packet->length ){
					spider_free_mem( data );
					free( head->v.packet );
					head->v.packet	=	NULL;
					spider_free_mem( head );
					continue;
				}
				data[body_len] = '\0';
				//开启线程读取数据，并修改心跳时间
				task *new_task;
				new_task	=	( task *)malloc( sizeof( task ) );
				if( new_task == NULL ){
					log_write( LOG_ERR , "malloc memory fail , system malloc full %s %d" , __FILE__ , __LINE__ );
					spider_free_mem( data );
					free( head->v.packet );
					head->v.packet	=	NULL;
					spider_free_mem( head );
					events[i].data.fd	=	-1;
					epoll_ctl( epfd , EPOLL_CTL_DEL , event_fd , NULL );
					close_client(event_fd , c->uid , c->gmid , c->gameid );
					continue;
				}
				new_task->cmd	=	head->v.packet->cmd;
				new_task->fd	=	event_fd;
				new_task->client=	c;
				new_task->data	=	data;
				new_task->next	=	NULL;
				free( head->v.packet );
				free( head );
				head = NULL;
				fprintf( stdout ,"read client data:%s , client cmd:%d , client fd:%d , client uid:%ld \n",  new_task->data , new_task->cmd , new_task->fd , new_task->client->uid );
				pthread_mutex_lock(&t_mutex);
				if( task_head == NULL ){
					task_head = new_task;
				}else{
					task_last->next = new_task;
				}
				task_last	=	new_task;
				pthread_cond_signal(&t_cond);
				pthread_mutex_unlock(&t_mutex);
				
			} else if (events[i].events & EPOLLOUT) {
				if (events[i].data.fd < 0) {
                    continue;
                }

                //设置用于读操作的文件描述符
                ev.data.fd = events[i].data.fd;
                //设置用于注测的读操作事件
                ev.events = EPOLLIN | EPOLLET;
                //修改sockfd上要处理的事件为EPOLIN
                epoll_ctl(epfd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
			}
		}
	}
	
	
}



/**********************************/
/*************内部socket***********/
/**********************************/
//#TODO


void serv_heartbeat(void * args){
	int i;
	while(1){
		sleep_thread(40);
        time(&mytimestamp);
        p = gmtime(&mytimestamp);
		fd2client *temp	=	fd2clients;
		while( temp != NULL && temp->fd != 0){
			if( (mytimestamp-temp->client->keepalivetime)>600 ){
				 log_write(LOG_DEBUG, "serv_heartbeat 600 seconds none event shadown client!,spider_id:%ld, mid:%d",  temp->client->uid , temp->client->gameid );
				 fd2client *del_temp	=	temp;//#debug
				 temp	=	temp->next;
				 close_client2( del_temp );
				 continue;
			}
			temp	=	temp->next;
		}
        pthread_testcancel();
	}
}
/**
* 销毁程序
*/
void destory(){
	int i=0;
	time(&mytimestamp);
	log_write( LOG_ERR , "shadown program %ld ..." , mytimestamp );
	if (t_epoll) pthread_cancel(t_epoll);
    if (t_serv_heart) pthread_cancel(t_serv_heart);
	for (t_num; t_num > 0; t_num--) {
        if (tid[t_num]) pthread_cancel(tid[t_num]);
    }
	//	删除链接
	while( fd2clients != NULL && fd2clients->fd != 0){
		pthread_mutex_destroy( &fd2clients->client->client_mutex );
		spider_free_mem( fd2clients->client );
		del_fd2client(fd2clients);
	}
	pthread_mutex_destroy(&t_mutex);
    pthread_cond_destroy(&t_cond);
	exit(-1);
}

