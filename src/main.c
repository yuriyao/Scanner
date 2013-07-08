#include "scanner.h"
#include "register.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <getopt.h>
#include <dlfcn.h>


/*判断是否为插件*/
static int is_plugin(char *name)
{
	int len = strlen(name);
	return strcmp(name + len - 3, ".so") == 0;
}

/**
 *  加载plugins文件夹下的插件
 **/
static int load_plugins()
{
	char buffer[512];
	/*插件的目录*/
	DIR *plugin_dir;
	struct dirent *plugin;
	/*动态加载的句柄*/
	void *handler;
	/**/
	void (*plugin_init)();
	/*初始化函数名称前缀*/
	char *plugin_name_prefix = "init_";

	/*打开插件所在的文件夹*/
	plugin_dir = opendir("plugins");
	if(!plugin_dir)
	{
		perror("opendir");
		return -1;
	}
	/*加载每一个可能的插件*/
	while((plugin = readdir(plugin_dir)) != NULL)
	{
		if(is_plugin(plugin->d_name))
		{
			/*获得插件的名字*/
			sprintf(buffer, "plugins/%s", plugin->d_name);
			/*加载插件*/
			handler = (void*)dlopen(buffer, RTLD_NOW);
			if(!handler)
			{
				perror("dlopen");
				continue;
			}
			/*获得插件的初始化函数*/
			sprintf(buffer, "%s", plugin_name_prefix);
			memcpy(buffer + strlen(buffer), plugin->d_name, strlen(plugin->d_name) - 3);
			buffer[strlen(plugin_name_prefix) + strlen(plugin->d_name) - 3] = '\0';
			printf("Calling init function %s\n", buffer);
			plugin_init = (void(*)())dlsym(handler, buffer);
			if(plugin_init)
			{
				plugin_init();
			}
			else
			{
				dlclose(handler);
			}
		}
	}
}
/*扫描方式*/
static int scan_method = SCAN_GENERAL;
/*参数1*/
static int arg1 = 0;
static int arg2 = PORT_MAX;
/*主机名*/
static char host_name[256];
static int host_gived = 0;
/**/
static char log_file[1024] = "scanner.log";
static int log_file_gived = 0;
/*网卡*/
static char nic[256] = "eth1";

static struct option opts[] = {
	{"host",  required_argument, NULL, 'h'},
	{"port", optional_argument, NULL, 'p'},
	{"type", optional_argument, NULL, 't'},
	{"logfile", optional_argument, NULL, 'f'},
	{"nic", optional_argument, NULL, 'n'}
};

/*解析参数*/
static int parse_args(int argc, char **argv)
{
	int opt;
	char type[20];
	int i = 0;

	/*使用的是scanner hostname*/
	if(argc == 2)
	{
		
		if(strlen(argv[1]) > 255)
		{
			printf("Host-name %s 太长了\n", argv[1]);
			return -1;
		}
		strcpy(host_name, argv[1]);
		host_gived = 1;
		return 0;
	}

	while((opt = getopt_long(argc, argv, "h:p:t:f:n:", opts, NULL)) != -1)
	{
		switch(opt)
		{
		case 'h':
			if(strlen(optarg) > 255)
			{
				printf("Host-name %s 太长了\n", optarg);
				return -1;
			}
			strcpy(host_name, optarg);
			host_gived = 1;
			break;
		case 'p':
			//printf("port %s\n", optarg);
			sscanf(optarg, "%d-%d", &arg1, &arg2);
			if(arg1 < 0 || arg2 < 0 || arg1 > arg2 || arg2 > PORT_MAX)
			{
				printf("端口范围不合法\n");
				return -1;
			}
			break;
		case 'f':
			if(strlen(optarg) > 1023)
			{
				printf("日志文件名太长\n");
				return -1;
			}
			strcpy(log_file, optarg);
			log_file_gived = 1;
			break;
		case 'n':
			if(strlen(optarg) > 255)
			{
				printf("网卡名称太长\n");
				return -1;
			}
			strcpy(nic, optarg);
			break;

		case 't':
			if(strlen(optarg) > 19)
			{
				printf("未定义的扫描类型:%s\n", optarg);
				return -1;
			}
			strcpy(type, optarg);
			for(i = 0; i < strlen(type); i ++)
				type[i] = toupper(type[i]);
			/*通用扫描*/
			if(strcmp(type, "GEN") == 0)
				scan_method = SCAN_GENERAL;
			/**/
			else if(strcmp(type, "FIN") == 0)
				scan_method = SCAN_FIN;
			else if(strcmp(type, "TCP") == 0)
				scan_method = SCAN_TCP;
			else if(strcmp(type, "UDP") == 0)
				scan_method = SCAN_UDP;
			else if(strcmp(type, "MIX") == 0)
				scan_method = SCAN_MIXED;
			else
				scan_method = SCAN_HALF;
			break;
		default:
			break;
		}
	}
	if(!host_gived)
	{
		printf("主机名是必须的\n");
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret;

	
	/*解析参数*/
	ret = parse_args(argc, argv);
	if(ret < 0)
		exit(-1);

	printf("Scanning %s from %d to %d with method %d\n", host_name, arg1, arg2, scan_method);
	
	/*初始化日志*/
	ret = scanner_log_init(log_file_gived ? log_file : NULL);
	if(ret < 0)
	{
		perror("scanner_log_init");
		exit(-1);
	}
	/*初始化扫描器*/
	ret = scanner_init(host_name, scan_method, nic);
	if(ret < 0)
	{
		perror("scanner_init");
		exit(-1);
	}
	/*加载插件*/
	load_plugins();

	/*开始执行*/
	scanner_work(SCAN_RANGE, arg1, arg2);

	/*执行结束,清理内存*/
	scan_exit();
	/*关闭日志*/
	scanner_log_exit();
	return 0;

}