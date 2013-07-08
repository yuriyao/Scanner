/**/
#include "util.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static FILE *log_file;
static pthread_mutex_t log_mutex;
static char log_buffer[1024 * 4];

int scanner_log_init(char *filename)
{
	if(filename)
	{
		log_file = fopen(filename, "w");
		if(!log_file)
			return -1;
	}
	else
		log_file = stdout;
	pthread_mutex_init(&log_mutex, NULL);
	return 0;
}


void scanner_log(char *fmt, ...)
{
	char *time_str;
	va_list ap;
	va_start(ap, fmt);
	/*加锁,避免冲突*/
	pthread_mutex_lock(&log_mutex);
	/*打印时间*/
	//fprintf(log_file, "%s:\t", ctime(NULL));
	/*打印log*/
	vfprintf(log_file, fmt, ap);
	fflush(log_file);
	pthread_mutex_unlock(&log_mutex);
	va_end(ap);
}

void scanner_log_exit()
{
	fclose(log_file);
}