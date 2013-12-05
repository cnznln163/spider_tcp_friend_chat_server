/***
*@desc 操作功能
*@auth JasonChen
*@date 2013-11-11
***/
#include "handle.h"

void readtask(void *args){
	while( 1 ){
		//互斥访问任务队列
        pthread_mutex_lock(&t_mutex);
		//等待到任务队列不为空 
		while( task_head == NULL ){
			//线程阻塞，释放互斥锁，当等待的条件等到满足时，它会再次获得互斥锁
            pthread_cond_wait(&t_cond, &t_mutex);
		}
		//从任务队列取出一个读任务
		struct task *temp = task_head;
		task_head = task_head->next;
		pthread_mutex_unlock(&t_mutex);
		log_write(LOG_DEBUG, "readtask, cmd: %d, socket_id: %d", temp->cmd , temp->fd );
		//#TODO
		pthread_mutex_lock( &temp->client->client_mutex );
		if(handle_data( temp )){
			pthread_mutex_unlock( &temp->client->client_mutex );
		}
		if( temp->data ) spider_free_mem( temp->data );
		spider_free_mem( temp );
		pthread_testcancel();
	}
	pthread_exit(NULL);
}


int handle_data( task *task ){
	int cmd;
	cmd = task->cmd;
	switch( cmd ){
		case HANDLE_LOGIN:
			handle_login( task );	//	登录处理
			break;
		case HANDLE_LOGINOUT:
			handle_loginout( task ); // 登出
			return 0;
			break;
		case HANDLE_HEARTBEAT:
			handle_heartbeat( task );	//	心跳
			break;
		case HANDLE_SEND_MESSAGE:		//	与单一用户聊天
			handle_send_message( task );
			break;
		case HANDLE_GET_FRIEND_LIST:
			handle_get_friend_list( task );
			break;
		default:
			handle_catch_error( task ); // 异常处理
	}
	return 1;
}
/**
* @desc 登录
*/
int handle_login( task *task ){
	int retcode;
	char ret[100]={'\0'};
	retcode		=	mod_login(task->client , task->data );
	snprintf( ret , sizeof(ret) , "{\"ret\":%d}" , retcode );
	handle_send( task->fd , task->cmd , task->client->gameid , ret );
	/////上线后操作////////
	//发送离线消息
	if( retcode == _HANDLE_SUCCESSFUL ){
		mod_chat_send_offline_msg( task->fd , task->client->uid , task->client->gameid );
	}
	
	//////////////////
	return SUCCESSFUL;
}

/**
* 心跳接口
*/
int handle_heartbeat( task *task ){
	if( strcmp( HEARTBEAT_POLICY  , task->data ) == 0 ){
		time(&mytimestamp);
		task->client->keepalivetime	= mytimestamp;
		char ret[100]={'\0'};
		strncpy( ret , "{\"ret\":1}" , 100 );
		handle_send( task->fd , task->cmd , task->client->gameid , ret );
	}else{
		close_client( task->fd , task->client->uid , task->client->gmid , task->client->gameid );
	}
}

/**
* 统一返回
*/
void handle_send( socket_id fd , int cmd  , int gameid , char *ret ){
	int len;
	char maxBuff[MAX_BUFFER_LENGTH]={'\0'};
	packet_head head;
	head.length	=	strlen(ret);
	strcpy( head.flag , "BY");
	head.cVersion	=	1.0;//#TODO
	head.cmd	=	cmd;
	head.gameid	=	gameid;
	head.code	=	PACKET_HEAD_CODE;
	memcpy( maxBuff , &head , sizeof(packet_head));
	len	=	sizeof(packet_head);
	memcpy( maxBuff+len , ret , strlen(ret) );
	len += strlen(ret);
	int sendcode = send( fd , maxBuff , len , 0 );
	if( sendcode == -1 ){
		log_write( LOG_ERR ,"send data fail , cmd:%d socket:%d , gameid:%d , ret:%s\n ", cmd , fd , gameid , ret );
	}else{
#if DEBUG_SWITCH
		log_write( LOG_DEBUG , "send data successful , cmd:%d socket:%d , gameid:%d , ret:%s \n " , cmd , fd , gameid , ret );
#endif
		;;
	}
}
/**
* 异常处理
*/
void handle_catch_error( task *task ){
	
}

int handle_loginout( task *task ){
	close_client( task->fd ,  task->client->uid , task->client->gmid , task->client->gameid );
}

int handle_send_message( task *task ){
	char ret[100]={'\0'};
	int retcode	=	mod_chat_send_message( task->client->uid , task->data );
	strncpy( ret , "{\"ret\":%d}" , retcode );
	handle_send( task->fd , task->cmd , task->client->gameid , ret);
	return SUCCESSFUL;
}

/**
*@desc 获取好友列表
*/
void handle_get_friend_list( task *task ){
	char friend[512];
	char ret[600];
	int retcode	=	mod_friend_get_friend_list( task->client->uid , friend );
	if( retcode != _HANDLE_SUCCESSFUL ){
		snprintf( ret ,  sizeof(ret)  , "{\"ret\":%d}" , retcode );
		handle_send( task->fd , task->cmd , task->client->gameid ,ret);
	}else{
		snprintf( ret , sizeof(ret) , "{\"ret\":%d,\"list\":%s}" , retcode , friend );
		handle_send( task->fd , task->cmd , task->client->gameid ,ret);
	}
}