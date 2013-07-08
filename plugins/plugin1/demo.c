#include <stdio.h>
#include "register.h"
#include "scanner.h"

static int tcp_scanner(int port)
{
	printf("I am scan port %d\n", port);
}

void init_demo()
{
	//register_scanner(DEFAULT_SCANNER, tcp_scanner, SCAN_TCP);
	printf("init\n");
}
