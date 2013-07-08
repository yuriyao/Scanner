#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include "register.h"
#include "scanner.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int tcp_scanner(int port)
{
	int raw_socket;
	raw_socket = socket(AF_INET,SOCK_RAW,IPPROTO_TCP);
	struct sockaddr_in *_target;
	struct sockaddr_in target;

	if(raw_socket < 0)
	{
		printf("init MySocket failed\n");
		exit(0);
		return 0;
	} 

	const int on = 1;
	if(setsockopt(raw_socket,IPPROTO_IP,IP_HDRINCL,&on,sizeof(on)) < 0)
	{
		printf("Setting IP_HDRINCL Error!\n");
		exit(0);
		return 0;
	}

	
	_target = get_scan_addr();
	memcpy(&target, _target, sizeof(target));

	if(connect(raw_socket, (struct sockaddr*)&target,sizeof(target)) < 0)
	{
		printf("Connect Error: %s(errno:%d)\n", (char*)strerror(errno),errno);
		exit(0);
		return 0;
	}
	printf("Connect Successful!\n");
	close(raw_socket);
	exit(0);
	return 1;

}

void init_tcp()
{
	
	register_scanner(80, tcp_scanner, SCAN_TCP);
	
	printf("init\n");
}
