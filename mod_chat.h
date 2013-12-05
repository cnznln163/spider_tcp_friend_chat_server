#ifndef SPIDER_MOD_CHAT
#define SPIDER_MOD_CHAT

#ifdef	__cplusplus
extern "C" {
#endif

#include "spider_tcp_server.h"


int mod_chat_send_message( long_t send_uid , char *msg );

void mod_chat_send_offline_msg(socket_id fd , long_t uid , int gameid );

#ifdef	__cplusplus
}
#endif

#endif
