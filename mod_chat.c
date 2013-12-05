/**
*@desc 聊天模块
*@auth JasonChen@boyaa.com
*@date 2013-11-18
*/
#include "mod_chat.h"


int mod_chat_send_message( long_t send_uid , char *data ){
	//	获取recv_uid客户端fd
	long_t recv_uid;
	spider_client *recv_client;
	json_value *raw;
	char *send_data;
	json_value *chat_data = json_parse( data , strlen(data) );
	json_value *raw_data  = chat_data;
	if( chat_data && chat_data->type == json_object  ){
		char *key	=	chat_data->u.object.values->name;
		if( (0 == strcmp( "recv_id" ,key )) && chat_data->u.object.values->value->type == json_integer ){
			raw		=	chat_data->u.object.values->value;
			recv_uid=	raw->u.integer;
			raw		=	raw->_reserved.next_alloc;
		}else{
			json_value_free(raw_data);
			return _ERROR_MOD_CHAT_PARSE_DATA_ERROR;
		}
		key +=8;
		if( (0==strcmp("msg" , key )) && raw->type == json_string && raw->u.string.length < 1024 ){
			send_data	=	raw->u.string.ptr;
		}else{
			json_value_free(raw_data);
			return _ERROR_MOD_CHAT_PARSE_DATA_ERROR;
		}
		recv_client = findClientByUid( recv_uid );
		if( recv_client == NULL ){	// offline
			log_write( LOG_DEBUG , "send message:recv_uid offline recv_uid:%ld\n" , recv_uid );
			//#TODO 写入数据库中
			char sql[2048];
			char table[30]={'\0'};
			getTableNameByInt2mod( send_uid , table , OFFLINE_MSG_TABLE_LEVEL , 30 );
			snprintf( sql , sizeof(sql) , "insert into %s ( uid , send_uid , msg ) values ( %d , %d , '%s' );" , table , recv_uid , send_uid , send_data );
			DBConn *conn = getConnect();
			if( conn == NULL ){
				log_write( LOG_ERR , "write offline msg fail , cannot get msg connect , send_uid:%d , recv_uid:%d , msg:%s \n " , send_uid , recv_uid , send_data );
				json_value_free(raw_data);
				return _ERROR_MOD_CHAT_SEND_MSG_SQL_FAIL;
			}
			if(!querySql( conn->conn , sql )){
				log_write( LOG_ERR , " write sql fail , fail reason:%s\n" , mysql_error(conn->conn));
			}
			destoryConnect(conn);
			json_value_free( raw_data );
			return _HANDLE_SUCCESSFUL;
		}else{
			char ret[1048];
			snprintf(ret , sizeof(ret) , "{\"send_uid\":%d , \"msg\":\"%s\" ,\"time\":%ld}" , send_uid , send_data , time(NULL) );
			handle_send(recv_client->fd ,HANDLE_SEND_MESSAGE , recv_client->gameid ,  ret);
			json_value_free( raw_data );
			return _HANDLE_SUCCESSFUL;
		}
	}else{
		if(chat_data) json_value_free(raw_data);
		return _ERROR_MOD_CHAT_PARSE_DATA_ERROR;
	}
}
/**
* 发送离线消息
*/
void mod_chat_send_offline_msg( socket_id fd , long_t uid  , int gameid ){
	char sql[256];
	char table[30];
	getTableNameByInt2mod( uid ,table , OFFLINE_MSG_TABLE_LEVEL , 30 );
	snprintf( sql , sizeof(sql) , "select id , send_uid , msg , time from %s where uid=%ld " , table , uid );
	DBConn *conn = getConnect();
	if( conn != NULL){
		MYSQL_ROW row;
		MYSQL_RES *res	=	getRows(conn->conn,sql);
		if( res != NULL ){
			while( row = mysql_fetch_row( res ) ){
				char msg[1048];
				memset( msg , 0 , 1048 );
				snprintf( msg , sizeof(msg) , "{\"send_uid\":%s , \"msg\":\"%s\" , \"time\":%ld}" , row[1] , row[2] , time(NULL) );
				handle_send( fd  , HANDLE_SEND_MESSAGE , gameid ,msg);
				//	delete msg
				memset(sql , 0 , 256);
				snprintf( sql , sizeof(sql) , "delete from %s where id=%s" , table , row[0] );
				querySql(conn->conn,sql);
			}
			mysql_free_result( res );
		}
		destoryConnect( conn );
	} 
}

