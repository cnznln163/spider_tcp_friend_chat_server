#include "db.h"

// .c中定义变量
pthread_mutex_t mysql_normal_mutex =  PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mysql_courrent_mutex =  PTHREAD_MUTEX_INITIALIZER;

// global mysql conn
db_pool *sql_conn = NULL;

// global mysql courrent connect sum
int mysql_courrent_connet_sum	=	NORMARL_MYSQL_CONNECT;


BOOL initMysql(){
	sql_conn = ( db_pool *)malloc( sizeof( db_pool ) );	
	sql_conn->conn_sum	=	NORMARL_MYSQL_CONNECT;
	DBConn *conn        =   ( DBConn *)malloc( sizeof( DBConn ) );
	assert( conn );
	conn->conn	=	mysql_init( NULL );
	if( NULL == mysql_real_connect( conn->conn , MYSQL_HOST , MYSQL_USER , MYSQL_PWD , MYSQL_DB ,  MYSQL_PORT , NULL , CLIENT_MULTI_STATEMENTS )  ){
		log_write( LOG_ERR,"connot connect mysql , File:%s Line:%d \n" , __FILE__ , __LINE__ );
		return FALSE;
	}
	conn->is_use		=	0;
	conn->seq			=	0;
	conn->normal_flag	=	TRUE;
	conn->next			=	NULL;
	conn->prev			=	NULL;
	sql_conn->db		=	conn;
	int connect_num		=	NORMARL_MYSQL_CONNECT-1;
	while( connect_num-- ){
		DBConn *temp	=	(DBConn *)malloc(sizeof(DBConn));
		assert(temp);
		temp->conn	=	mysql_init(NULL);
		if( NULL == mysql_real_connect( temp->conn , MYSQL_HOST , MYSQL_USER , MYSQL_PWD , MYSQL_DB ,  MYSQL_PORT , NULL , CLIENT_MULTI_STATEMENTS )  ){
			log_write( LOG_ERR,"connot connect mysql , File:%s Line:%d \n" , __FILE__ , __LINE__ );
			return FALSE;
		}
		temp->is_use	=	0;
		temp->seq			=	NORMARL_MYSQL_CONNECT - connect_num;
		temp->normal_flag	=	TRUE;
		temp->next		=	NULL;
		temp->prev		=	conn;
		conn->next		=	temp;
		conn			=	temp;
	}
	return TRUE;
} 


DBConn *getConnect(){
//	DBConn *new_conn;
	pthread_mutex_lock(&mysql_normal_mutex);
	if( sql_conn->conn_sum == 0 ){	//	连接池中无连接,创建新连接
		//	立即释放conn_sum锁
		pthread_mutex_unlock( &mysql_normal_mutex );
		//	同时锁住current_sum锁
		pthread_mutex_lock( &mysql_courrent_mutex );
		if( mysql_courrent_connet_sum < MAX_MYSQL_CONNECT ){
			DBConn *new_conn	=	(DBConn *)malloc( sizeof(DBConn) );
			if( new_conn != NULL ){
				new_conn->conn = mysql_init(NULL);
				if( NULL == mysql_real_connect( new_conn->conn , MYSQL_HOST , MYSQL_USER , MYSQL_PWD , MYSQL_DB ,  MYSQL_PORT , NULL , CLIENT_MULTI_STATEMENTS )  ){
					log_write( LOG_ERR,"connot connect mysql , File:%s Line:%d \n" , __FILE__ , __LINE__ );
					pthread_mutex_unlock(&mysql_courrent_mutex);
					return  NULL ;
				}
				new_conn->is_use	=	0;
				new_conn->seq		=	mysql_courrent_connet_sum;
				new_conn->normal_flag	=	FALSE;
				new_conn->prev		=	NULL;
				new_conn->next		=	NULL;
				mysql_courrent_connet_sum++;
			}else{
				log_write( LOG_ERR , "getConnect malloc fail File:%s Line:%d\n" , __FILE__ , __LINE__ );
			}			
			pthread_mutex_unlock(&mysql_courrent_mutex);
			return new_conn;
		}else{
			//	等待30ms
//			usleep(30);
			//#TODO 尝试创建新连接
			pthread_mutex_unlock(&mysql_courrent_mutex);
			return NULL;
		}
	}{
		DBConn *temp = sql_conn->db;
		while( temp != NULL && temp->is_use != 0 ){
			temp	=	temp->next;
		}
		temp->is_use	=	1;
		sql_conn->conn_sum--;
		pthread_mutex_unlock(&mysql_normal_mutex);
		return temp;
	}
}


BOOL destoryConnect(DBConn * sql_connect){
	if( sql_connect->normal_flag ){
		pthread_mutex_lock( &mysql_normal_mutex );
		sql_connect->is_use	=	0;
		sql_conn->conn_sum++;
		pthread_mutex_unlock( &mysql_normal_mutex );
	}else{
		mysql_close( sql_connect->conn );
		free( sql_connect );
		sql_connect = NULL;
		pthread_mutex_lock( &mysql_courrent_mutex );
		mysql_courrent_connet_sum--;
		pthread_mutex_unlock( &mysql_courrent_mutex );
	}
}
BOOL querySql( MYSQL *conn , char *sql ){
//	char *temp_sql	=	mysql_escape_string(sql);
	if(mysql_real_query( conn , sql , strlen(sql) )){
		return FALSE;
	}
	return TRUE;
}

MYSQL_RES* getRows( MYSQL *conn , char *sql ){
	if (mysql_ping(conn) != 0) {
        log_write(LOG_ERR, "mysql: connection to database lost, %s, %d", __FILE__, __LINE__);
        return NULL;
    }
	if( mysql_real_query( conn , sql , strlen(sql) ) ){
		return NULL;
	}
	MYSQL_RES *res = mysql_store_result(conn);
	return res;
}
