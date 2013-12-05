/**
*@auth JasonChen@boyaa.com
*@date 2013-11-10
*/
#ifndef SPIDER_TCP_SERVER
#define SPIDER_TCP_SERVER

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h> //相关函数atoi，atol，strtod，strtol，strtoul

#include <unistd.h>
#include <crypt.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>
#include <openssl/md5.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "global.h"
#include "json.h"
#include "log.h"
#include "db.h"

#define SUCCESSFUL 1
#define FAIL  0

#define DEBUG_SWITCH 1

#define SERVER_PORT 6000
#define MAX_CLIENTS 50000
#define MAX_FDS  20480
#define MAX_BUFFER_LENGTH 2046
#define TASK_MAX 10

#define BUFF_ONE_BYTE 0x0af
#define BUFF_SECOND_BYTE 0x2af
#define BUFF_LEN  4

#define MAX_EVENTS 10240

//定义三种链接状态
#define CONNECT_STATUS_ACCPET 0; //	刚刚accept
#define CONNECT_STATUS_ALIVE  1; // 存活状态
#define CONNECT_STATUS_ZOMBIE 2; // 僵尸状态

#define PACKET_HEAD_CODE '&'
#define PACKET_MAX_LEN 1024

#define HEARTBEAT_POLICY "<heartbeat_event>spider_tcp</heartbeat_event>\0"





typedef long int long_t;
typedef int socket_id;


typedef struct server {
		socket_id listen_sockfd;
		struct sockaddr_in listen_addr;
} server;


int epfd; 	//	epoll句柄

pthread_mutex_t t_mutex_fd[MAX_FDS];
pthread_mutex_t t_mutex;
pthread_mutex_t t_mutex_log;
pthread_cond_t t_cond;


/****************************************************************/
/*实现通过蜘蛛id查找用户信息uid2client->spider_client
/*实现通过socket id查找用户信息fd2client->spider_client
/***************************************************************/

typedef struct spider_client{
	socket_id fd;
	long_t uid;	//	蜘蛛ID 
	long_t gmid; // 游戏用户id
	int gameid; //	游戏id
	char app_version[10];//客户端版本
	int keepalivetime; // 上一次心跳时间
	int connect_status;// 链接状态
	pthread_mutex_t client_mutex; //用户线程锁
} spider_client;

#pragma pack(1)
typedef struct packet_head{
	int length;   		//长度  4
    char  flag[2];   	//BY 2
    short  cVersion;  	//版本 2
    int cmd;          	//命令类型 4
    short gameid;       //gameid 2
    unsigned char  code;  //checkcode 1
}packet_head;
#pragma pack()

typedef struct {
	int status;
	union {
		packet_head *packet;
		int return_value;
	}v;
}packet_status;


//所有的客户端列表
spider_client *fd_clients[MAX_FDS];


typedef struct uid2client{
	long_t uid;
	spider_client *client;
	struct uid2client *next;
} uid2client;

typedef struct fd2client{
	socket_id fd;
	spider_client *client;
	struct fd2client *next;
	struct fd2client *prev;  
	struct fd2client *tail; // 头指针
}fd2client;

fd2client *fd2clients ;

typedef struct task{
	socket_id fd;
	spider_client *client;
	int cmd;
	char *data;
	struct task *next;
} task;
task *task_head;
task *task_last;

int t_num;
pthread_t tid[TASK_MAX];

//时间
time_t mytimestamp;
struct tm *p;


void destory();





#define HANDLE_LOGIN 0x100
#define HANDLE_LOGINOUT 0x101
#define HANDLE_HEARTBEAT 0x102
#define HANDLE_SEND_MESSAGE 0x103
#define HANDLE_GET_FRIEND_LIST 0x104

//static const char *handle[]	=	{
//									"login",
//									"login_out",
//									"handle_heartbeat"
//								};

//#include "spider_function.h"
#include "handle.h"


#ifdef	__cplusplus
}
#endif


#endif


