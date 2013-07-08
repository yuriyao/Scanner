/**
 *  实现线程池
 *  author: jeffyao
 **/
#include <pthread.h>
#include <semaphore.h>
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

enum thread_state
{
	/*线程正在运行*/
	THREAD_RUNNING,
	/*线程正在等待分配任务*/
	THREAD_WAITING,
	/*线程正在关闭*/
	THREAD_EXITING,
};

struct args
{
	/*传递给工作函数的参数*/
	void *arg;
	/*指向线程节点*/
	struct thread_node *self;
	/*指向线程池*/
	struct thread_pool *thread_pool;
};

struct thread_node
{
 	/*线程号*/
 	pthread_t thread;
 	/*是否已经初始化*/
 	int is_init;
 	/*是否还活跃*/
 	volatile int is_alive;
 	/*工作函数*/
 	volatile void (*func)(void*);
 	/*参数*/
 	volatile struct args args;
 	/*指向自身*/
 	int self_index;
 	/*指向下一个空闲节点*/
 	int next;
 	/*是否空闲*/
 	int is_free;
 	/*互斥量*/
 	pthread_mutex_t mutex;
 	/*条件变量*/
 	pthread_cond_t cond;
 	/*线程状态*/
 	enum thread_state state;
};

struct thread_pool
{
	/*线程池的线程数量*/
	int number;
	/*线程数组*/
	struct thread_node *threads;
	/*指向第一个空闲的线程*/
	volatile int head;
	/*可用的空闲线程数量*/
	sem_t sem;
	/*结构的互斥量*/
	pthread_mutex_t mutex;
};

/**
 *  创建线程池
 *  @thread_max_number  线程的最大数量
 *  如果创建成功,返回线程池指针,否则返回NULL,并置errno
 **/
struct thread_pool* thread_pool_create(int thread_max_number);

/**
 *  分发一个任务
 *  @thread_pool
 *  @func
 *  @arg
 *  如果分发成功,返回0,否则返回负数,并置errno
 *  注意,这个函数只负责任务的分发,函数执行结束,不代表任务执行结束
 **/
int thread_distribute(struct thread_pool *thread_pool, void (*func)(void*), void *arg);


/**
 *  销毁线程池
 **/
void thread_pool_destroy(struct thread_pool *thread_pool);

#endif 