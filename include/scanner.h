/**
 *
 **/
#ifndef SCANNER_H
#define SCANNER_H
#include <sys/types.h>
/**
 *  初始化扫描器
 **/
int scanner_init(char *addr, int _scan_method, char *nic);

int scanner_work(int type, int arg1, int arg2);

void scan_exit();

/**
 *  添加一个全局扫描器
 **/
int add_global_scanner(int (*func)(int), int type);

/**
  *  添加收尾函数
  **/
int add_over_func(void (*func)(), int type);

/**
  *  获得本机的ip,已转为网络字节序
  *  @fd  套接口
  */
u_int32_t get_local_ip(int fd);

/*获取扫描的地址*/
struct sockaddr_in* get_scan_addr();

/*已废弃*/
extern struct scan_func_info *default_scanner_info;
/*注册的扫描函数表*/
extern struct HashTable *scanner_table;
/*全局扫描函数列表*/
struct linear_node *global_scanner;

/*扫描器的工作方法*/
/*仅开放服务端口扫描*/
#define SCAN_OPEN_PORT_ONLY 1
/*全部扫描*/
#define SCAN_ALL 2
/*低端口扫描(0-1024)*/
#define SCAN_LOW 3
/*高端口扫描*/
#define SCAN_HIGH 4
/*低端口+高端口随机扫描*/
#define SCAN_LOW_RAND_HIGH 5
/*部分端口(0-2000)扫描*/
#define SCAN_PART 6
/*指定端口扫描*/
#define SCAN_FIXED 7
/*指定范围扫描*/
#define SCAN_RANGE 8

/*扫描使用的方法*/
/*半开扫描*/
#define SCAN_HALF 1
/*FIN方式*/
#define SCAN_FIN 2
/*tcp*/
#define SCAN_TCP 3
/*udp*/
#define SCAN_UDP 4
/*混合*/
#define SCAN_MIXED 5
/*通用扫描*/
#define SCAN_GENERAL 6

struct linear_node
{
	void *data;
	//int (*scan_func)(int port);
	struct linear_node *next;
};


/*扫描结果*/
struct scan_result_node
{
	/*扫描成功的端口号*/
	int port;
	/*所采用的扫描方式*/
	int type;

};

struct over_func_info
{
	void (*over)();
	int type;
};

#endif