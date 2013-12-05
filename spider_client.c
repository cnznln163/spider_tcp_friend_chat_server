/**
*@auth JasonChen@boyaa.com
*@date 2013-11-14
*@desc 客户端
*/

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

#define SERVER_PORT 6000
#define SERVER_IP "127.0.0.1"

#define HANDLE_LOGIN 0x000
#define HANDLE_LOGINOUT 0x001
#define HANDLE_HEARTBEAT 0x002
typedef int socket_id;
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

packet_head *read_head( socket_id fd );

int main(){
	int fd;
	char head_str[16]={'\0'};
	struct sockaddr_in addr;
	if( (fd = socket( AF_INET , SOCK_STREAM , 0 )) < 0 ){
		fprintf( stdout , "create socket fail\n" );
		exit(-1);
	}

	bzero( &addr , sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_port		=	htons( SERVER_PORT );
	addr.sin_addr.s_addr = inet_addr( SERVER_IP );
	if( connect( fd , (struct sockaddr *)&addr , sizeof(addr)) < 0 ){
		fprintf(stdout , "connect fail\n");
		exit(-1);
	}
	long int uid=10001;
	long int gmid= 200001;
	int gameid	=	1004;
	char *app_version="1.0.1";
	packet_head head;
	char pack[100]={'\0'};
	char send_buf[1024];
	snprintf( pack , 100 , "{\"uid\":%ld,\"gmid\":%ld,\"gameid\":%d,\"app_version\":\"%s\"}" , uid , gmid , gameid , app_version );
	head.length	=	strlen(pack);
	strcpy( head.flag ,  "BY");
	head.cVersion=1.0;
	head.cmd=HANDLE_LOGIN;
	head.gameid=1004;
	head.code	=	'&';
	int buff_len =	sizeof(packet_head);
	memcpy( send_buf , &head , sizeof(packet_head));
	fprintf(stdout, "pack_len:%d\n" , strlen(pack));
	// snprintf(send_buf , sizeof(send_buf) , "%s%s" , head_str , pack );
	memcpy( send_buf+buff_len , &pack , strlen(pack));
	buff_len	+= strlen(pack);	
	// write( fd , &head , sizeof(head) );
	int buflen	=	strlen( send_buf );
	if(write( fd , send_buf , buff_len )<0){
		fprintf(stdout , "发送失败\n");
	}
	memset(head_str , 0 , 16);
	while( recv( fd , head_str , 16 , MSG_PEEK ) ){
		packet_head *recv_pack	=	read_head(fd);
		if( recv_pack == NULL ){
			perror( "recv error" );
			exit(-1);
		}
		char *ret;
		ret	=	(char *)malloc( recv_pack->length );
		read( fd , ret , recv_pack->length );
		fprintf( stdout , "recv:%s\n" , ret );
	}
	// 发送心跳
	//睡眠5分钟
	sleep(300);
	memset( pack , 0 , sizeof(pack));
	snprintf( pack , 100 , "<heartbeat_event>spider_tcp</heartbeat_event>" );
	head.length	=	strlen(pack);
	strcpy( head.flag ,  "BY");
	head.cVersion=1.0;
	head.cmd=HANDLE_HEARTBEAT;
	head.gameid=1004;
	head.code	=	'&';
	buff_len =	sizeof(packet_head);
	memcpy( send_buf , &head , sizeof(packet_head));
	fprintf(stdout, "pack_len:%d\n" , strlen(pack));
	memcpy( send_buf+buff_len , &pack , strlen(pack));
	if(write( fd , send_buf , buff_len )<0){
		fprintf(stdout , "发送失败\n");
	}
	while( recv( fd , head_str , 16 , MSG_PEEK ) ){
		packet_head *recv_pack	=	read_head(fd);
		if( recv_pack == NULL ){
			perror( "recv error" );
			exit(-1);
		}
		char *ret;
		ret	=	(char *)malloc( recv_pack->length );
		read( fd , ret , recv_pack->length );
		fprintf( stdout , "recv:%s\n" , ret );
	}
	// 发送消息给uid10002
	memset( pack , 0 , sizeof(pack));
	snprintf( pack , 100 , "{\"send_uid\":%d ,\"recv_uid\":%d,\"msg\":\"haha,nihao world\"}" , 10001 , 10002 );
	head.length	=	strlen(pack);
	strcpy( head.flag ,  "BY");
	head.cVersion=1.0;
	head.cmd=HANDLE_HEARTBEAT;
	head.gameid=1004;
	head.code	=	'&';
	buff_len =	sizeof(packet_head);
	
}

packet_head *read_head( socket_id fd ){
	struct packet_head *head;
	head	=	(struct packet_head *)malloc( sizeof( head ) );
	assert( head );
	head->length	=	0;
	memset(head->flag , 0 , 2 );
	head->cVersion	=	0.0;
	head->cmd		=	0;
	head->gameid	=	0;
	head->code		=	0;
	int len;
	len		=	read( fd , &head->length , 4 );
	if( len <= 0 || head->length > 1024 || head->length <= 0){
		free(head);	
		return NULL;
	}
	len		=	read( fd , head->flag , 2 );
	if( len != 2 || strncmp( head->flag , "BY" , 2) != 0 ){	//	标识码错误
		free(head);
		return NULL;
	}
	len		=	read( fd , &head->cVersion , 2 );
	if( len != 2 ){
		free(head);
		return NULL;
	}
	len		=	read( fd , &head->cmd , 4);
	if( len != 4  ){
		free(head);
		return NULL;
	}
	len		=	read( fd , &head->gameid , 2 );
	if( len != 2 || head->gameid < 1000 ){
		free( head );
		return NULL;
	}
	len		=	read( fd , &head->code , 1 );
	if( len  != 1 || ( head->code != '&') ){
		free( head );
		return NULL;
	}
	return head;
}





