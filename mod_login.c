#include "mod_login.h"

/**
*@desc 登录
*/
int mod_login( spider_client *client ,  char *data  ){
	json_value *login_data;
	login_data	=	json_parse( data , strlen( data ) );
	json_value *json_raw	= 	login_data;
	if( !login_data ){	//	登录异常
		return _ERROR_HANDLE_LOGIN_DATA_PARSE_ERROR ;
	}
	//	#登录验证逻辑:PHP侧生成token并返回给客户端并存储在redis中, 客户端登录长连接并传递token,长连接验证token
	if( login_data->type != json_object || login_data->u.object.length != 4 ){
		json_value_free( login_data );
		return _ERROR_HANDLE_LOGIN_DATA_PARSE_ERROR;
	}
	long_t uid;
	long_t mid;
	int	   gameid;
	char *key	=	login_data->u.object.values->name;
	login_data		=	login_data->u.object.values->value;
	if( (0 == strcmp( "uid" , key )) && login_data->type == json_integer  ){	//uid
		uid				=	login_data->u.integer;
		login_data		=	login_data->_reserved.next_alloc;
	}else{
		json_value_free( json_raw );
		return _ERROR_HANDLE_LOGIN_DATA_PARSE_ERROR;
	}
	key		+=4;
	if( (0 == strcmp( "mid" , key )) && login_data->type == json_integer ){		//	mid
		mid	=	login_data->u.integer;
		login_data		=	login_data->_reserved.next_alloc;
	}else{
		json_value_free( json_raw );
		return _ERROR_HANDLE_LOGIN_DATA_PARSE_ERROR;
	}
	key		+=4;
	if( (0 == strcmp( "gameid" , key )) && login_data->type == json_integer){	//	gameid
		gameid			=	login_data->u.integer;
		login_data		=	login_data->_reserved.next_alloc;
	}else{
		json_value_free( json_raw );
		return _ERROR_HANDLE_LOGIN_DATA_PARSE_ERROR;
	}
	key		+=7;
	if((0 == strcmp( "app_version" , key )) && login_data->type == json_string ){	//	app_version
		strncpy( client->app_version , login_data->u.string.ptr , 10 );
	}else{
		json_value_free( json_raw );
		return _ERROR_HANDLE_LOGIN_DATA_PARSE_ERROR;
	}
	client->uid		=	uid;
	client->gmid	=	mid;
	client->gameid	=	gameid;
	json_value_free( json_raw );
	return _HANDLE_SUCCESSFUL;
}

void mod_login_recv_offlien_msg( spider_client *client ){
	
}

 


