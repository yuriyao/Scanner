/**
 *  调试工具
 **/
/*打印包信息*/
#include <stdio.h>

#ifndef UTIL_H
#define UTIL_H

static void print_pack(char *pack, int len)
{
	int i = 0;
	int k = 0;
	printf("-------------packet info----------");
	for(i = 0; i < len; i ++, k++)
	{
		if(k % 16 == 0)
		{
			printf("\n%3d:\t", k);
		}
		printf("%02x ", (unsigned char)pack[i]);
	}
	printf("\n-------------------------------\n");
}

/**
  *  日志系统初始化
  *  @filename  日志名,为空时使用标准输出
  */
int scanner_log_init(char *filename);

/**
  *  打印日志
  *  参数同printf
  */
void scanner_log(char *fmt, ...);


void scanner_log_exit();
#endif