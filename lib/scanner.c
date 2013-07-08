#include "scanner.h"
#include "hash.h"
#include "register.h"
#include <netdb.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "util.h"
#include "thread_pool.h"
/**
 *
 **/
/*默认的扫描函数*/
static struct scan_func_info _default_scanner_info;
/*已废弃*/
struct scan_func_info *default_scanner_info = &_default_scanner_info;

/*用于全局扫描(扫描全部端口)*/
struct linear_node *global_scanner = NULL;
/*扫描的线程池*/
static struct thread_pool *scan_thread_pool;
/*选用的扫描方式*/
static int scan_method = SCAN_GENERAL;
/*注册的扫描函数表*/
struct HashTable *scanner_table = NULL;
/*扫描结果*/
struct linear_node *scan_reslut = NULL;
/**/
struct sockaddr_in global_sockaddr;
/*over函数的链表*/
struct linear_node *over_lists = NULL;
static char nic[256];

struct sockaddr_in* get_scan_addr()
{
	return &global_sockaddr;
}

/**
 *  添加一个全局扫描器
 **/
int add_global_scanner(int (*func)(int), int type)
{
	struct scan_func_info *scan_func_info;
	struct linear_node *node;
	if(!func)
	{
		errno = EINVAL;
		return -1;
	}
	node = (struct linear_node*)mm_malloc(sizeof(struct linear_node));
	if(!node)
	{
		errno = ENOMEM;
		return -1;
	}
	scan_func_info = (struct scan_func_info*)mm_malloc(sizeof(struct scan_func_info));
	if(!scan_func_info)
	{
		mm_free(node);
		errno = ENOMEM;
		return -1;
	}
	scan_func_info->scan_func = func;
	scan_func_info->type = type;
	node->next = NULL;
	node->data = (void*)scan_func_info;
	if(!global_scanner)
		global_scanner = node;
	else
	{
		node->next = global_scanner->next;
		global_scanner->next = node;
	}
	return 0;
}

int add_over_func(void (*func)(), int type)
{
	struct over_func_info *over_func_info;
	struct linear_node *node;

	node = (struct linear_node*)mm_malloc(sizeof(struct linear_node));
	if(!node)
		goto error1;
	over_func_info = (struct over_func_info*)mm_malloc(sizeof(struct over_func_info));
	if(!over_func_info)
		goto error2;
	over_func_info->over = func;
	over_func_info->type = type;
	node->data = (void*)over_func_info;
	node->next = NULL;
	if(!over_lists)
		over_lists = node;
	else
	{
		node->next = over_lists->next;
		over_lists->next = node;
	}
	return 0;
error2:
	mm_free(node);
error1:
	errno = ENOMEM;
	return -1;
}

/*获得本机的ip,已转为网络字节序*/
u_int32_t get_local_ip(int fd)
{
	struct ifreq if_data;
	u_int32_t addr_p;

	strcpy (if_data.ifr_name, nic);
	if (ioctl (fd, SIOCGIFADDR, &if_data) < 0)
	{
		/*
		*取名为eth0的的IP地址
		*这是个interface的地址
		*/
		perror("ioctl");
		exit(1);
	}
	memcpy ((void *) &addr_p, (void *) &if_data.ifr_addr.sa_data + 2, 4);
	return addr_p;
}
/*所有的开放端口*/
static int open_ports[] = {21, 25, 80, 53, 110};  

/*半开扫描*/
static int half_scanner(int port)
{
	char buffer[8192];
	int raw_sock;
	struct sockaddr_in *_dest;
	int ret;
	static u_int32_t seq = 0x12345678;
	struct sockaddr_in dest;
	int sock_len;
	int i = 0;
	struct tcphdr *tcp;
	u_int32_t current_seq = seq;
	u_int32_t dst_ip;

	/*创建用于发送tcp包的原始套接字*/
	raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if(raw_sock < 0)
	{
		perror("socket");
		return -errno;
	}
	/*获取扫描地址*/
	_dest = get_scan_addr();
	memcpy(&dest, _dest, sizeof(dest));
	//dest = *dest;
	dest.sin_port = htons(port);
	/*构造syn包*/
	memcpy(&dst_ip, &dest.sin_addr, 4);
	ret = tcp_syn_pack((struct tcphdr*)buffer, 8192, get_local_ip(raw_sock), dst_ip, 5643, port, seq ++);
	if(ret < 0)
	{
		perror("tcp_syn_pack");
		return ret;
	}
	//print_pack(buffer, ret);
	//ip_pack((struct ip*)buffer, IPPROTO_TCP, 255)
	/*发送*/
	ret = sendto(raw_sock, buffer, ret, 0, (struct sockaddr*)&dest, sizeof(dest));
	if(ret < 0)
	{
		perror("sendto");
		return -errno;
	}
	/*接收对应的报文*/
	sock_len = sizeof(dest);
	/*尝试接收20次,每次间隔100ms,所以最长延迟2s*/
	for(i = 0; i < 40; i ++)
	{
		ret = recvfrom(raw_sock, buffer, 8192,  MSG_DONTWAIT,(struct sockaddr*)&dest, &sock_len);
		if(ret < 0 && (errno != EAGAIN) && (errno != EWOULDBLOCK))
		{
			perror("recvfrom");
			close(raw_sock);
			return 0;
		}
		else if(ret >= sizeof(struct ip) + sizeof(struct tcphdr))
		{
			/*定位到tcp报文*/
			tcp = (struct tcphdr*)(buffer + ((struct ip*)buffer)->ip_hl * 4);
			/*检测是否是合格的确认报文*/
			if(tcp->ack && tcp->seq && tcp->ack_seq == htonl(current_seq + 1))
			{
				scanner_log("主机打开了端口:%10d\n", port);
				//fflush(stdout);
				close(raw_sock);
				return 1;
			}
			//printf("recv one %x %x %x\n", tcp->ack, tcp->syn, ntohl(tcp->ack_seq));
		}
		usleep(10 * 1000);
	}
	//printf("The host didn't open %d\n", port);
	//fflush(stdout);
	close(raw_sock);
	return 0;
} 

/*用于扫描函数表的内部函数*/
static int key_equal(void *k1, void *k2)
{
	return strcmp((char*)k1, (char*)k2);
}

static void* copy_value(void *src)
{
	return src;
}

static void free_value(void *value)
{
	;
}

int scanner_init(char *addr, int _scan_method, char *_nic)
{
	unsigned long inaddr;
	struct hostent *host;

	/*设置默认的扫函数*/
	default_scanner_info->scan_func = half_scanner;
	default_scanner_info->type = SCAN_HALF;
	/*初始化扫描结果*/
	scan_reslut = NULL;

	/*初始化扫描函数表*/
	scanner_table = hash_new(10,  key_equal, copy_value, free_value);
	if(!scanner_table)
		return -1;
	/*解析地址*/
	memset((void*)&global_sockaddr, 0, sizeof(global_sockaddr));
	inaddr = inet_addr(addr);
	if(inaddr == INADDR_NONE)
	{
		host = gethostbyname(addr);
		if(!host)
		{
			printf("gethostbyname");
			return -1;
		}
		memcpy((char*)&global_sockaddr.sin_addr, host->h_addr, host->h_length);
	}
	else
	{
		memcpy((char*)&global_sockaddr.sin_addr, (char*)&inaddr, sizeof(inaddr));
	}
	global_sockaddr.sin_family = AF_INET;
	/*初始化线程池*/
	scan_thread_pool = thread_pool_create(200);
	if(!scan_thread_pool)
	{
		perror("thread_pool_create");
		return -1;
	}
	/*扫描方式*/
	scan_method = _scan_method;
	/*设置网卡*/
	strcpy(nic, _nic);
	/*将半开扫描设为全局扫描器*/
	return add_global_scanner(half_scanner, SCAN_HALF);
}

/**
  * 记录扫描成功的端口信息
  * 
  */
static int record_scan_ok(int port, int type)
{
	struct linear_node *linear_node;
	struct scan_result_node *result;

	result = (struct scan_result_node*)mm_malloc(sizeof(struct scan_result_node));
	if(!result)
		goto error1;
	linear_node = (struct linear_node*)mm_malloc(sizeof(struct linear_node));
	if(!linear_node)
		goto error2;
	result->port = port;
	result->type = type;
	linear_node->data = (void*)result;
	linear_node->next = NULL;
	if(!scan_reslut)
	{
		scan_reslut = linear_node;
	}
	else
	{
		linear_node->next = scan_reslut->next;
		scan_reslut->next = linear_node;
	}
	return 0;
error2:
	mm_free(result);
error1:
	errno = ENOMEM;
	return -errno;
}

/*用来扫描某个具体的端口*/
static void _scan_one(void *arg)
{
	int port = (int)arg;
	char number_buffer[10];
	struct linear_node *linear_node;
	int ret;
	int (*scan_func)(int port);

	sprintf(number_buffer, "%d", port);	

	/*调用全局的函数*/
	linear_node = global_scanner;
	while(linear_node)
	{
		if(scan_method == SCAN_GENERAL || ((struct scan_func_info*)linear_node->data)->type == scan_method)
		{
			scan_func = ((struct scan_func_info*)linear_node->data)->scan_func;
			ret = scan_func(port);
			if(ret == 1)
			{
			
			}
		}
		linear_node = linear_node->next;
	}

	/*调用注册函数*/
	linear_node = (struct linear_node*)hash_get(scanner_table, number_buffer);
	
	while(linear_node)
	{
		if(scan_method == SCAN_GENERAL || ((struct scan_func_info*)linear_node->data)->type == scan_method)
		{
			scan_func = ((struct scan_func_info*)linear_node->data)->scan_func;
			ret = scan_func(port);
			if(ret == 1)
			{
			
			}
		}
		linear_node = linear_node->next;
	}
}

static void scan_one(int port)
{
	thread_distribute(scan_thread_pool, _scan_one, (void*)port);
}

/*扫描结束，调用处理函数*/
static void _over_handle(void *arg)
{
	struct linear_node *node = over_lists;

	while(node)
	{
		if(scan_method == SCAN_GENERAL || scan_method == ((struct over_func_info*)node->data)->type)
		{
			((struct over_func_info*)node->data)->over();
		}
		node = node->next;
	}
}

static void over_handle()
{
	thread_distribute(scan_thread_pool, _over_handle, NULL);
}
/**
  *  扫描器开始工作
  *  @type  扫描方式
  *  @arg1, @arg2  由方式解释
  */
int scanner_work(int type, int arg1, int arg2)
{
	int i = 0;
	long k;

	switch(type)
	{
	case SCAN_OPEN_PORT_ONLY:
		for(i = 0; i < sizeof(open_ports) / sizeof(int); i ++)
		{
			scan_one(open_ports[i]);
		}
		break;
	case SCAN_LOW:
		for(i = 0; i < 1024; i ++)
		{
			scan_one(i);
		}
		break;
	case SCAN_HIGH:
		for(i = 1025; i <= PORT_MAX; i ++)
			scan_one(i);
		break;
	case SCAN_ALL:
		for(i = 0; i <= PORT_MAX; i ++)
			scan_one(i);
		break;
	case SCAN_PART:
		for(i = 0; i < 2000; i ++)
			scan_one(i);
		break;
	case SCAN_FIXED:
		scan_one(arg1);
		break;
	case SCAN_RANGE:
		for(i = arg1; i <= arg2; i ++)
			scan_one(i);
		break;
	case SCAN_LOW_RAND_HIGH:
		for(i = 0; i < 1024; i ++)
			scan_one(i);
		srandom(time(NULL));
		for(i = 1025; i <= PORT_MAX; i += 100)
		{
			k = random();
			k %= 100;
			scan_one(i + k);
		}
		break;
	default:
		errno = EINVAL;
		return -EINVAL;
	}
	/*收尾处理*/
	over_handle();
	return 0;
}

void scan_exit()
{
	thread_pool_destroy(scan_thread_pool);
}

