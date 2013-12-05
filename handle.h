#ifndef SPIDER_HANDLE
#ifdef	__cplusplus
extern "C" {
#endif

#include "spider_tcp_server.h"
#include "mod_chat.h"
#include "mod_login.h"
#include "mod_friend.h"

void readtask(void *args);
int handle_data( task *task_t );
int handle_login(task *task_t );
int handle_loginout(task *task_t );
int handle_heartbeat( task *task_t );
void handle_catch_error( task *task_t );
void handle_send( socket_id fd , int cmd  , int gameid , char *ret  );
void handle_get_friend_list( task *task );



#ifdef	__cplusplus
}
#endif

#endif
