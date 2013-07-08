/**
 * hash表操作
 **/  
#ifndef HASH_H
#define HASH_H
/*当hash表的节点数目是hash表容量的2/3时触发重新申请表*/
#define TRIGGER(capacity, number) ((number) * 3 > (capacity << 1))

#define DUMMY ((void*)1)

/*hash表的节点*/
struct HashNode
{
	//key
	char *key;
	//value
	void *value;
};

/*hash表*/
struct HashTable
{
	//计算两个key是否相等的函数，返回0表示相等
	int (*key_equal)(void *k1, void *k2);
	//拷贝value值，为空直接赋值
	void* (*copy_value)(void *src);
	//为value申请空间的函数
	//释放value值的函数
	void (*free_value)(void *value);
	//hash表的节点容量
	int capacity;
	//hash表的节点数目
	int number;
	//hash表头
	struct HashNode **head;
};

typedef struct HashIter
{
	struct HashTable *table;
	int index;
}HashIter;

/*创建一个hash表*/
struct HashTable* hash_new(int pre_max/*预估的最大节点数量的1.5倍*/,
	int (*key_equal)(void *k1, void *k2), 
	void* (*copy_value)(void *src),
	void (*free_value)(void *value));

/*向hash表中插入一组数据*/
void hash_insert(struct HashTable *table, char *key, void *value);

/*获得指定key值的value*/
void* hash_get(struct HashTable *table, char *key);

/*删除制定key值的节点*/
void hash_del(struct HashTable *table, char *key);

struct HashTable* hash_new_str(int pre_max/*预估的最大节点数量的1.5倍*/);

/*向hash表中插入一组数据*/
void insert_str(struct HashTable *table, char *key, char *value);

/*获得指定key值的value*/
char* get_str(struct HashTable *table, char *key);

/*删除制定key值的节点*/
void del_str(struct HashTable *table, char *key);

/**
 *  创建一个hash表的迭代器
 **/
HashIter* hash_create_iter(struct HashTable *table);

/**
 * 
 **/
struct HashNode* hash_next(HashIter *iter);

#endif