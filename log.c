#include "log.h"
/**
* 判断目录是否存在,不存在则创建
*/
int dir_exists( time_t *t , const char *interface ){
	char interface_dir[100];
	char year_dir[120];
	char month_dir[120];
	struct tm *stm;
	stm = gmtime(t);
	snprintf(interface_dir , sizeof(interface_dir) , "%s/%s" , LOG_ROOT_DIR , interface );
	if(  0 != access( interface_dir , F_OK ) ){
		if(mkdir( interface_dir , 0755 ) == -1){
			perror( "mkdir error interface" );
			return LOG_FAIL;
		}
	}
	snprintf(year_dir , sizeof(year_dir) , "%s/%s/%d" , LOG_ROOT_DIR , interface , stm->tm_year + 1900 );
	if(  0 != access( year_dir , F_OK ) ){
		if(mkdir( year_dir , 0755 ) == -1){
			perror( "mkdir error year" );
			return LOG_FAIL;
		}
	}
	snprintf( month_dir , sizeof( month_dir ) , "%s/%d" , year_dir , 1+stm->tm_mon );
	if(  0 != access( month_dir , F_OK)){
		if(mkdir( month_dir , 0755 ) == -1){
			perror( "mkdir error month" );
			return LOG_FAIL;
		}
	}
	return LOG_SUCCESSFUL;
}

/**
* 通用写入
*/
int log_write(  int level , const char *fmt, ... ){
	time_t t;
	struct tm *stm;
	char *p[10] , *pos;
	int len;
	char *outchar;
	char log_contents[512];
	char dir[120];
	FILE *log_fd;
	time(&t);
	stm = gmtime(&t);
	if(!dir_exists( &t , log_level[level] )){
		return LOG_FAIL;
	}
	len	= snprintf(log_contents , sizeof(log_contents) , "%ld\t%s\t" , t , log_level[level] );
	snprintf(dir , sizeof(dir) , "%s/%s/%d/%d/%s_%d" , LOG_ROOT_DIR , log_level[level] , stm->tm_year + 1900 , 1+stm->tm_mon , log_level[level] , stm->tm_mday);
	log_fd	=	fopen(dir , "a+");
	log_contents[len] = '\0';
	pos	=	log_contents;
	pos +=  len;
	va_list arg_ptr; 
	va_start(arg_ptr, fmt);
	vsnprintf(pos, 512 - len, fmt, arg_ptr);
	va_end(arg_ptr);
	/********************debug***********************/
	#if LOG_SWITCH_DEBUG 
	fprintf(stdout , "%s\n" , log_contents);
	#endif
	/************************************************/
	fwrite( log_contents , strlen(log_contents) , 1 , log_fd );
	fclose(log_fd);
	return LOG_SUCCESSFUL;
}


