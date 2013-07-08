/**
 *  gcc -o thread_pool thread_pool.c -I ../include -lpthread
 **/
#include "thread_pool.h"
#include <errno.h>
#include <assert.h>
 #include <stdio.h>
#include "mm.h"
/**
 *  创建线程池
 *  @thread_max_number  线程的最大数量
 *  如果创建成功,返回线程池指针,否则返回NULL,并置errno
 **/
struct thread_pool* thread_pool_create(int thread_max_number)
{
	struct thread_pool *thread_pool;
	int i = 0;
	struct thread_node *nodes;

	if(thread_max_number <= 0)
	{
		errno = EINVAL;
		return (struct thread_pool*)0;
	}
	/**/
	thread_pool = (struct thread_pool*)mm_malloc(sizeof(struct thread_pool));
	if(!thread_pool)
		goto error1;
	/*申请线程数组*/
	thread_pool->threads = (struct thread_node*)mm_malloc(sizeof(struct thread_node) * thread_max_number);
	if(!thread_pool->threads)
		goto error2;
	/*先初始化线程池控制块*/
	thread_pool->number = thread_max_number;
	thread_pool->head = 0;
	pthread_mutex_init(&thread_pool->mutex, NULL);
	sem_init(&thread_pool->sem, 0, thread_max_number);
	/*初始化线程数组*/
	nodes = thread_pool->threads;
	//memset(nodes, 0, sizeof(struct thread_node) * thread_max_number);
	for(i = 0; i < thread_max_number; i ++)
	{
		nodes[i].is_init = 0;
		nodes[i].is_alive = 1;
		nodes[i].next = i + 1;
		nodes[i].is_free = 1;
		nodes[i].args.self = &nodes[i];
		nodes[i].args.thread_pool = thread_pool;
		nodes[i].self_index = i;
		pthread_mutex_init(&nodes[i].mutex, NULL);
		pthread_cond_init(&nodes[i].cond, NULL);
	}
	nodes[thread_max_number - 1].next = -1;
	return thread_pool;
error2:
	mm_free(thread_pool);
error1:
	errno = ENOMEM;
	return (struct thread_pool*)0;
}

static void* _worker(void *arg)
{
	struct args *args = (struct args*)arg;
	volatile void (*func)(void*);
	void *v;

	
	while(args->self->is_alive)
	{
		pthread_mutex_lock(&args->self->mutex);
		func = args->self->func;
		v = args->arg;
		args->self->is_free = 0;
		args->self->state = THREAD_RUNNING;
		pthread_mutex_unlock(&args->self->mutex);
		/*开始执行任务*/
		func(v);
		/**/
		pthread_mutex_lock(&args->self->mutex);
		/*修改线程的控制信息,使其可以再次被分发调度*/
		args->self->is_free = 1;
		args->self->state = THREAD_WAITING;
		pthread_mutex_lock(&args->thread_pool->mutex);
		args->self->next = args->thread_pool->head;
		args->thread_pool->head = args->self->self_index;
		//printf("head %d\n", args->thread_pool->head);
		sem_post(&args->thread_pool->sem);
		pthread_mutex_unlock(&args->thread_pool->mutex);
		/*让线程睡眠,等待调度*/
		//printf("I am waiting for call\n");
		pthread_cond_wait(&args->self->cond, &args->self->mutex);
		pthread_mutex_unlock(&args->self->mutex);
	}
	pthread_mutex_lock(&args->self->mutex);
	args->self->state = THREAD_EXITING;
	args->self->is_init = 0;
	pthread_mutex_unlock(&args->self->mutex);
	return NULL;
}

/**
 *  分发一个任务
 *  @thread_pool
 *  @func
 *  @arg
 *  如果分发成功,返回0,否则返回负数,并置errno
 *  注意,这个函数只负责任务的分发,函数执行结束,不代表任务执行结束
 **/
int thread_distribute(struct thread_pool *thread_pool, void (*func)(void*), void *arg)
{
	int index;
	struct thread_node *node;
	int ret;
	int out = 0;

	if(!thread_pool || !func)
	{
		errno = EINVAL;
		return -errno;
	}
	/*等待有空闲的线程节点*/
	sem_wait(&thread_pool->sem);
	pthread_mutex_lock(&thread_pool->mutex);
	/*获取空闲的线程*/
	index = thread_pool->head;
	//printf("index:%d\n", index);
	assert(index >= 0);
	/*改变空闲节点的头*/
	thread_pool->head = thread_pool->threads[index].next;
	/*为分发线程进行尊卑*/
	node = thread_pool->threads + index;
	pthread_mutex_lock(&node->mutex);
	node->args.arg = arg;
	node->func = func;
	node->is_free = 0;
	//printf("hello\n");
	/*线程是在等待分发*/
	if(node->is_init)
	{
		/*唤醒线程*/
		pthread_cond_signal(&node->cond);
	}
	/*还没有初始化,需要创建新线程*/
	else
	{
		//printf("create thread\n");
		ret = pthread_create(&node->thread, NULL, _worker, (void*)&node->args);
		/*创建线程失败*/
		if(ret < 0)
		{
			perror("pthread_create");
			/*节点归还线程池*/
			node->is_free = 1;
			node->next = thread_pool->head;
			thread_pool->head = index;
			out = -1;
			sem_post(&thread_pool->sem);
		}
		else
		{
			node->is_init = 1;
			node->is_alive = 1;
		}
		
	}
	pthread_mutex_unlock(&node->mutex);

	pthread_mutex_unlock(&thread_pool->mutex);

	return out;
}


/**
 *  销毁线程池
 **/
void thread_pool_destroy(struct thread_pool *thread_pool)
{
	int i = 0;
	struct thread_node *nodes;

	if(!thread_pool)
		return;

	nodes = thread_pool->threads;
	/*挨个销毁线程*/
	for(i = 0; i < thread_pool->number; i ++)
	{
		pthread_mutex_lock(&nodes[i].mutex);
		if(nodes[i].is_init)
		{
			nodes[i].is_alive = 0;
			/*等待线程结束*/
			while(nodes[i].state == THREAD_RUNNING)
			{
				pthread_mutex_unlock(&nodes[i].mutex);
				usleep(500);
				pthread_mutex_lock(&nodes[i].mutex);
			}
			/*唤醒线程,让它自然结束*/
			pthread_cond_signal(&nodes[i].cond);
			pthread_mutex_unlock(&nodes[i].mutex);
			pthread_join(nodes[i].thread, NULL);
			pthread_mutex_lock(&nodes[i].mutex);
		}
		pthread_mutex_unlock(&nodes[i].mutex);
		/*销毁线程信息*/
		pthread_mutex_destroy(&nodes[i].mutex);
		pthread_cond_destroy(&nodes[i].cond);
	}
	/*销毁线程池控制块*/
	pthread_mutex_destroy(&thread_pool->mutex);
	sem_destroy(&thread_pool->sem);
	mm_free(thread_pool->threads);
	mm_free(thread_pool);
}

//#define THREAD_POOL_DEBUG
#ifdef THREAD_POOL_DEBUG


void print(void *arg)
{
	printf("%d\n", (int)arg);
	sleep(1);
}
int main(int argc, char **argv)
{
	struct thread_pool *pool = thread_pool_create(10);
	int i = 0;

	for(i = 0; i < 500; i ++)
	{
		thread_distribute(pool, print, (void*)i);
	}
	thread_pool_destroy(pool);
	return 0;
}
#endif