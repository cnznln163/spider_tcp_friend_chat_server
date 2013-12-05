#ifndef SPIDER_GLOBAL
#define SPIDER_GLOBAL

#ifdef __cplusplus
extern "C" {
#endif

#define OFFLINE_MSG_TABLE_LEVEL 0
#define APPLY_MSG_TABLE_LEVEL 1
#define FRIENDS_TABLE_LEVEL 2

#define GET_TABLE( level , uid , table_num ) \
								switch( (level) ){ \
											case 0:(table_num) = (uid) % 10;break; \
											case 1: (table_num) = (uid) % 10;break;\
											case 2: (table_num) = (uid) % 100;break;\
											default:(table_num) = (uid) % 100;\
										}

static const char *mod_table[]  = { 
							"offline_msg"  , 
							"apply_msg" ,
							"friends"
						 };
//	操作成功码
#define _HANDLE_SUCCESSFUL 1
/**************错误码*************/
//	登录
// 数据解析错误
#define _ERROR_HANDLE_LOGIN_DATA_PARSE_ERROR -1000
//  聊天模块
#define _ERROR_MOD_CHAT_PARSE_DATA_ERROR -1010
#define _ERROR_MOD_CHAT_SEND_MSG_SQL_FAIL -1011
//	好友模块
#define _ERROR_MOD_FRIEND_MYSQL_ERROR -1020

#ifdef	__cplusplus
}
#endif
#endif
