#include "register.h"
#include "scanner.h"
#include "mm.h"
#include <errno.h>
#include <stdio.h>



int register_scanner(int port, int (*scan_func)(int port), int type)
{
	char number_buffer[10];
	struct linear_node *linear_node, *v;
	struct scan_func_info *scan_func_info;

	/*参数检查*/
	if(scan_func == NULL || (port < DEFAULT_SCANNER) || port > PORT_MAX)
	{
		errno = EINVAL;
		return -errno;
	}
	if(port == DEFAULT_SCANNER)
	{
		return add_global_scanner(scan_func, type);
	}
	/**/
	sprintf(number_buffer, "%d", port);
	/*看看是否已经存在注册函数列表*/
	linear_node = (struct linear_node*)hash_get(scanner_table, number_buffer);

	/**/
	scan_func_info = (struct scan_func_info*)mm_malloc(sizeof(struct scan_func_info));
	if(!scan_func_info)
		goto error1;
	v = (struct linear_node*)mm_malloc(sizeof(struct linear_node));
	if(!v)
		goto error2;
	/*初始化一下*/
	scan_func_info->scan_func = scan_func;
	scan_func_info->type = type;
	v->next = NULL;
	v->data = (void*)scan_func_info;
	/*还没有注册函数*/
	if(!linear_node)
	{
		hash_insert(scanner_table, number_buffer, v);
	}
	else
	{
		/*采用头插法的变形,插入第二个*/
		v->next = linear_node->next;
		linear_node->next = v;
	}
	return 0;

error2:
	mm_free(scan_func_info);
error1:
	errno = ENOMEM;
	return -errno;
}

int register_over(void (*over)(void), int type)
{
	return add_over_func(over, type);
}