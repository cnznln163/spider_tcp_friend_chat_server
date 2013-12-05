#ifndef SPIDER_FUNCTION
#define SPIDER_FUNCTION

#ifdef	__cplusplus
extern "C" {
#endif

#include "spider_tcp_server.h"

extern fd2client *fd2clients;

void sleep_thread(int sec);
void set_nonblocking_socket( int fd );
fd2client *init_fd2client();
int add_fd2client( socket_id fd , spider_client *client );
int del_fd2client( fd2client *fd2 );
spider_client *is_exists_fd( socket_id fd );
fd2client *find_fd2client( socket_id fd);
void close_client( socket_id fd , long_t uid , long_t gmid , int gameid  );
int send_friend_offline_message( long_t uid , long_t mid , int gameid );
void login_history( long_t uid ,long_t gmid ,int gameid );
packet_status *read_head( socket_id fd );
int strkv(char *src, char *key, char *value);
void config(char *configFilePath, char * key , char *value );
void spider_free_mem( void *mem);
void close_client2( fd2client *fd2);
spider_client *findClientByUid( long_t uid );
void getTableNameByInt2mod( long_t _int_var , char *dest , int table_level , int len );


#ifdef	__cplusplus
}
#endif

#endif
