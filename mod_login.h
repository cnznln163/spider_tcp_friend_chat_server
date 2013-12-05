#ifndef SPIDER_MOD_LOGIN
#define SPIDER_MOD_LOGIN
#ifdef __cplusplus
extern "C" {
#endif

#include "spider_tcp_server.h"

int mod_login( spider_client *client ,  char *data  );
void mod_login_recv_offlien_msg( spider_client *client );

#ifdef __cplusplus
}
#endif
#endif
