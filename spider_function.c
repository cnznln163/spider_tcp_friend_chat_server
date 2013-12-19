#include "spider_function.h"



void sleep_thread(int sec) {
    struct timespec wake;

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    time(&mytimestamp);
    p = gmtime(&mytimestamp);

    wake.tv_sec = mytimestamp + sec;

    //����������sec���msec�����滻����������䣬����ʵ��΢�뼶��˯��
    //nsec = now.tv_usec * 1000 + (msec % 1000000) * 1000;
    //wake.tv_sec = now.tv_sec + msec / 1000000 + nsec / 1000000000;
    wake.tv_nsec = 0;

    pthread_mutex_lock(&mutex);
    pthread_cond_timedwait(&cond, &mutex, &wake);
    pthread_mutex_unlock(&mutex);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}


//int node_add(clients *p) {
//		log_write(LOG_DEBUG, "node_add, %s, %d", __FILE__, __LINE__);
//		if (p != NULL) {
//			hash_item *item = (hash_item *) malloc(sizeof (hash_item));
//			item->key = p->username;
//			item->data = (void *) p->fd;
//			item->next = NULL;
//			hash_add(item);
//		}
//		log_write(LOG_DEBUG, "node_add end, %s, %d", __FILE__, __LINE__);
//		return SUCCESSFUL;
//	}
/**
//* ɾ������,������б�
//*/	
//int node_del(spider_client *p, long i) {
//		log_write(LOG_DEBUG, "node_del, %s, %d", __FILE__, __LINE__);
//		if (p != NULL) {
//			fd_clients[i] = NULL;
//			close(p->fd);
//			free(p);
//			p = NULL;
//		} else {
//			log_write(LOG_DEBUG, "�����Ͽ��ͻ�������ʧ��, %s, %d", __FILE__, __LINE__);
//		}
//		return SUCCESSFUL;
//}

void set_nonblocking_socket(int fd){
	if( fcntl(fd , F_SETFL , fcntl(fd , F_GETFD , 0)|O_NONBLOCK) == -1 ){
		return NULL;
	}
	return NULL;
}

fd2client *init_fd2client(){
	if( fd2clients != NULL ) return fd2clients;
	fd2clients	=	(fd2client *)malloc( sizeof(fd2client) );
	if( fd2clients == NULL ){
		log_write(LOG_ERR,"��ʼ��fd2clients�б�ʧ��, file:%s , line:%d ", __FILE__ , __LINE__ );
		return NULL;
	}
	fd2clients->fd	=	0;
	fd2clients->client = NULL;
	fd2clients->next  = NULL;
	fd2clients->prev  = NULL;
	fd2clients->tail  = NULL;
	return fd2clients;
}

int add_fd2client(  socket_id fd , spider_client *client){
	if( fd2clients == NULL ) return FAIL;
	//	�ж�ͷָ��
	if( fd2clients->prev == NULL && fd2clients->fd == 0){	//ͷָ��
		fd2clients->fd	=	fd;
		fd2clients->client	=	client;
		fd2clients->next = NULL;
		fd2clients->prev	=	NULL;
		fd2clients->tail	=	fd2clients;
	}else{
		fd2client *tmp=fd2clients;
		while(tmp->next != NULL){
			tmp	=	tmp->next;
		}
		fd2client *new_fd2;
		new_fd2 = (fd2client *)malloc(sizeof(fd2client));
		if( new_fd2 == NULL ){
			log_write( LOG_ERR , "�����ڴ�ʧ�� , File:%s , Line:%d \n" , __FILE__ , __LINE__ );
			return FAIL;
		}
		new_fd2->fd	=	fd;
		new_fd2->client	=	client;
		new_fd2->next	=	NULL;
		new_fd2->prev	=	tmp;
		new_fd2->tail	=	tmp->tail;
		tmp->next		=	new_fd2;
	}
	return SUCCESSFUL;
}

int del_fd2client( fd2client *fd2 ){
	if( fd2 == NULL ) return FAIL;
	if( fd2->prev == NULL ){	//	ͷָ��
		fd2client *tmp	=	fd2clients;
		if( fd2clients->next != NULL ){
			fd2clients	=	fd2clients->next;
			fd2clients->tail	=	fd2clients;
			fd2clients->prev		=	fd2clients;
			spider_free_mem( tmp );
			return SUCCESSFUL;
		}else{
			fd2->fd	=	0;
			fd2->client = NULL;
			fd2->prev	=	NULL;
			fd2->next  = NULL;
			fd2->tail  = NULL;
		}
	}else{
		fd2client *tmp	=	fd2;
		if( fd2->next != NULL ){
			fd2->prev->next	=	fd2->next;
			fd2			=	fd2->next;
			fd2->prev	=	tmp->prev;
			spider_free_mem( tmp );
		}else{
			fd2->prev->next	=	NULL;
			spider_free_mem( tmp );
		}
	}
}
/**
* ����sock�Ƿ����
* @return ���ؿͻ��˽ṹ
*/
spider_client *is_exists_fd( socket_id fd){
	fd2client *tmp	=	fd2clients;
	while( tmp != NULL ){
		if( fd == tmp->fd ){
			return tmp->client;
		}
		tmp	=	tmp->next;
	}
	return NULL;
}
/**
* ����fd��Ӧ��fd2client�ڴ��ַ
* @return ����fd2client��ַ
*/
fd2client *find_fd2client( socket_id fd){
	fd2client *tmp	=	fd2clients;
	while( tmp != NULL ){
		if( fd == tmp->fd ){
			return tmp;
		}
		tmp	=	tmp->next;
	}
	return NULL;
}

/**
* ͨ��uid��ȡ�ͻ���client
* @return spider_client *
*/
spider_client *findClientByUid( long_t uid ){
	fd2client *tmp	=	fd2clients;
	while( tmp != NULL ){
		if( uid == tmp->client->uid ){
			return tmp->client;
		}
		tmp	=	tmp->next;
	}
	return NULL;
}


/**
* �رտͻ�������
* @params fd �ͻ���socket id
* @params uid ֩��id
* @params gmid ��Ϸmid
* @params gameid ��Ϸid
*/
void close_client( socket_id fd , long_t uid , long_t gmid , int gameid ){
	//	����������Ϣ������
	send_friend_offline_message(  uid ,  gmid ,  gameid );
	//	�޸�redis�����б�
	//  Continue
	//  ɾ��fd2client
	fd2client *temp;
	temp	=	find_fd2client( fd );
	if( temp == NULL ){
		log_write( LOG_ERR , "ɾ���ͻ���ʧ��-fd:%d uid:%ld gmid:%ld gameid:%d \n" , fd , uid , gmid , gameid );
		return ;
	}
	//	д���¼��ʷ
	login_history(  uid ,  gmid , gameid );
	//	�����û��߳���
	pthread_mutex_unlock( &temp->client->client_mutex );
	pthread_mutex_destroy( &temp->client->client_mutex );
	spider_free_mem( temp->client );
	del_fd2client( temp );
	log_write( LOG_DEBUG , "close client , fd:%d uid:%ld \n" , fd , uid );
	close( fd );
}


void close_client2( fd2client *fd2 ){
	socket_id cid =	fd2->fd;
	send_friend_offline_message( fd2->client->uid ,  fd2->client->gmid ,  fd2->client->gameid );
	//	д���¼��ʷ
	login_history(  fd2->client->uid ,  fd2->client->gmid ,  fd2->client->gameid );
	//	�����û��߳���
	pthread_mutex_destroy( &fd2->client->client_mutex );
	spider_free_mem( fd2->client );
	del_fd2client( fd2 );
	close( cid );
}

/**
* ���ͺ�������֪ͨ
*/
int send_friend_offline_message( long_t uid , long_t mid , int gameid ){
	// Continue
	return SUCCESSFUL;
}
/**
* д���¼��ʷ
*/
void login_history( long_t uid ,long_t gmid ,int gameid ){
	
}

packet_status *read_head( socket_id fd ){
	packet_status *head_status;
	head_status	=	(packet_status *)malloc( sizeof( head_status ) );
	assert( head_status );
	struct packet_head *head;
	head		=	(struct packet_head *)malloc( sizeof( packet_head ) );
	assert( head );
	head->length	=	0;
	memset(head->flag , 0 , 2 );
	head->cVersion	=	0.0;
	head->cmd		=	0;
	head->gameid	=	0;
	head->code		=	'\0';
	int len;
	len		=	recv( fd , &head->length , 4 , 0 );
	if( len == -1 ){
		fprintf( stdout , "error no: %d error str:%s\n" , errno , strerror( errno ) );
		free(head);
		head = NULL;
		if( errno == EAGAIN || errno == ECONNRESET ){
			head_status->status		=	0;
			head_status->v.return_value	=	-7;
			return head_status;
		}
		// client close
		head_status->status			=	0;
		head_status->v.return_value		=	-8;
		return head_status;
	}else if( len == 0 ){ // peer close connect , shutdown client
		head_status->status			=	0;
		head_status->v.return_value		=	-9;
		return head_status;
	}
	if( len <= 0 || head->length > PACKET_MAX_LEN || head->length <= 0){
		log_write( LOG_ERR ,"packet length Error:%d\n ", head->length );
		free(head);
		head=NULL;
		head_status->status		=	0;
		head_status->v.return_value	=	-1;
		return head_status;
	}
	len		=	recv( fd , head->flag , 2 , 0 );
	if( len != 2 || strncmp( head->flag , "BY" , 2) != 0 ){	//	��ʶ�����
		log_write( LOG_ERR ,"packet flag Error:%s\n ", head->flag );
		free(head);
		head=NULL;
		head_status->status		=	0;
		head_status->v.return_value	=	-2;
		return head_status;
	}
	len		=	recv( fd , &head->cVersion , 2 , 0 );
	if( len != 2 ){
		log_write( LOG_ERR ,"packet flag Error:%s\n ", head->flag );
		free(head);
		head=NULL;
		head_status->status		=	0;
		head_status->v.return_value	=	-3;
		return head_status;
	}
	len		=	recv( fd , &head->cmd , 4 , 0 );
	if( len != 4  ){
		log_write( LOG_ERR ,"packet cmd Error:%d\n ", head->cmd );
		free(head);
		head_status->status		=	0;
		head_status->v.return_value	=	-4;
		return head_status;
	}
	len		=	recv( fd , &head->gameid , 2 , 0 );
	if( len != 2 || head->gameid < 1000 ){
		log_write( LOG_ERR ,"packet gameid Error:%f\n ", head->gameid );
		free(head);
		head=NULL;
		head_status->status		=	0;
		head_status->v.return_value	=	-5;
		return head_status;
	}
	len		=	recv( fd , &head->code , 1 , 0 );
	if( len  != 1 || ( head->code != PACKET_HEAD_CODE) ){
		log_write( LOG_ERR ,"packet code Error:%c\n ", head->code );
		free(head);
		head=NULL;
		head_status->status		=	0;
		head_status->v.return_value	=	-6;
		return head_status;
	}
	head_status->status		=	1;
	head_status->v.packet	=	head;
	return head_status;
}

void spider_free_mem( void *mem ){
	free( mem );
	mem = NULL;
}


/*
* �ַ�����Ѱ���ԵȺŷֿ��ļ�ֵ��
* @param src Դ�ַ��� [�������]
* @param key �� [�������]
* @param value ֵ [�������]
*/
int strkv(char *src, char *key, char *value)
{
	char *p,*q;
	// int len;
	if(*src == '#') return 0; // # �ſ�ͷΪע�ӣ�ֱ�Ӻ���

	p	= strchr(src,'=');		// p�ҵ��Ⱥ�
	q	= strchr(src, '\r');	// ����лس���
	if( q == NULL ){	//	û�лس������һ��з�
		q	= strchr(src,'\n');		// q�ҵ�����
	}
	
	// ����еȺ��л���
	if (p != NULL && q != NULL)
		{
			*q = '\0';	// ����������Ϊ�ַ�����β
			strncpy(key, src, p - src); // ���Ⱥ�ǰ�����ݿ����� key ��
			strcpy(value, p+1); // ���Ⱥź�����ݿ����� value ��
			return 1;
		}else{
			return 0;
		}
}

/*
* ���ú���
* @param configFilePath �����ļ�·�� [�������]
* @param configVar �洢���õ�ֵ [�������]
* @param configNum �����õĸ��� [�������]
*/
void config(char *configFilePath, char * key , char *value )
{
	int i;
	FILE *fd;
	char buf[120]	=	"";// �����ַ���
	char k[50]		= 	"";
	char v[60]		=	""; // ���ñ���ֵ 
	
	// �������ļ�
	fd = fopen(configFilePath, "r");

	if (fd == NULL){
		printf("�����ļ���ʧ�ܣ�\n");
		exit(-1);
	}
	// ���ζ�ȡ�ļ���ÿһ��
	while(fgets(buf, 120, fd)){
		// ��ȡ��ֵ��
		if(strkv(buf, k , v) ){
			if (strcmp(k, key) == 0){
				i = 1;
				strcpy(value, v );
				break;
			}
			// ��� ��ȡ������ key
			memset(key, 0, strlen(key));
		}
	}
	fclose(fd);
	if( !i ){
		printf("��ȡ�����ļ�ʧ��  \n " );
		exit(-1);
	}
}

void getTableNameByInt2mod( long_t _int_var ,  char *dest , int table_level , int len ){
	int table_num;
	GET_TABLE(table_level , _int_var , table_num );
	snprintf( dest , len , "%s_%d" , mod_table[table_level] , table_num );
	fprintf(stdout , "table:%s , table_num:%d\n" , dest , table_num );
}



