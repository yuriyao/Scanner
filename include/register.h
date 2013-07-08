/**
 *  注册处理函数
 **/
#ifndef REGISTER_H
#define REGISTER_H
#include "scanner.h"
 
/**
 *  注册扫描函数
 *  @port 扫描函数需要扫描的端口,如果是DEFALULT_SCANNER会使函数成为默认的扫描函数
 *  @scan_func  扫描函数,其参数port是正在扫描的端口号,如果端口是打开的返回1,否则返回0
 *  @type  扫描器使用的扫描方式:
			#define SCAN_HALF 1 //半开
			#define SCAN_FIN 2  //fin
			#define SCAN_TCP 3  //tcp
			#define SCAN_UDP 4  //udp
			#define SCAN_MIXED 5 //混合
 *  注册成功返回0,否则返回负的错误号,并置errno
 */
extern int register_scanner(int port, int (*scan_func)(int port), int type); 

/**
 *  添加处理的收尾函数
 **/
extern int register_over(void (*over)(void), int type);

#define DEFAULT_SCANNER -1

/*扫描函数的信息*/
struct scan_func_info
{
	/*扫描函数*/
	int (*scan_func)(int);
	/*扫描函数所采用的扫描方式*/
	int type;
};

#define PORT_MAX ((1 << 16) - 1)

#endif