#ifndef SPIDER_DB
#define SPIDER_DB

#ifdef __cplusplus
extern "C" {
#endif
#define  DEBUG_MYSQL 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#if DEBUG_MYSQL
 #include <mysql.h>
#else
 #include <mysql.h>
#endif
#include "log.h"

typedef int BOOL;

#define TRUE 1
#define FALSE 0
#define MYSQL_HOST "192.168.183.32"
#define MYSQL_PORT 3306
#define MYSQL_USER "root"
#define MYSQL_PWD "123456"
#define MYSQL_DB "spider_chat"



#define MAX_MYSQL_CONNECT 1000
#define NORMARL_MYSQL_CONNECT 15 //默认15条mysql连接

typedef struct DBConn{
	MYSQL *conn;
	int is_use;
	int seq;
	BOOL normal_flag;
	struct DBConn *next;
	struct DBConn *prev;
}DBConn;

typedef struct db_pool{
	DBConn *db;
	int conn_sum;
}db_pool;

BOOL initMysql();

DBConn *getConnect();

DBConn *createNewConnect();

BOOL destoryConnect( DBConn *sql_connect );

BOOL querySql( MYSQL *conn , char *sql );

MYSQL_RES *getRows( MYSQL *conn , char *sql );
//MYSQL_RES *getOne( MYSQL *conn , char *sql );
//BOOL closeDB( MYSQL *conn );

#ifdef __cplusplus
}
#endif

#endif
