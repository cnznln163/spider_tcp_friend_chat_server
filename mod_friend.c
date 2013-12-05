#include "mod_friend.h"


int mod_friend_get_friend_list( long_t uid , char *friend ){
	char table[30];
	char sql[150];
	DBConn *conn;
	getTableNameByInt2mod( uid ,table ,FRIENDS_TABLE_LEVEL , 30 );
	snprintf( sql , sizeof(sql) , "select fuid from %s where uid=%d" , table , uid );
	conn	=	getConnect();
	if( conn == NULL ){
		return _ERROR_MOD_FRIEND_MYSQL_ERROR;
	}
	MYSQL_RES *res;
	MYSQL_ROW row;
	res	=	getRows( conn->conn , sql);
	if( !res ){
		return _ERROR_MOD_FRIEND_MYSQL_ERROR;
	}
	while( row = mysql_fetch_row( res ) ){
		char str_doule[21]={'\0'};
//		gcvt( row[0] , 0 , str_doule );
		snprintf(str_doule , sizeof(str_doule) , "%d" , row[0]);
		strncpy( friend+strlen(friend) , str_doule , 210);
		friend	+=strlen(friend);
		strncpy( friend , "," , 210 );
//		friend	=	',';
	}
	friend	+=strlen(friend);
	strncpy( friend , "}" , 210 );
	return _HANDLE_SUCCESSFUL;
}
