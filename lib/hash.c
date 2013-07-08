/**
 *  使用开放定址法实现的hash表(使用二次探测的方式)
 **/
#include "hash.h"
#include <string.h>
#include <stdio.h>
#include "mm.h"


static int _get_empty(struct HashTable *table, char *key);

/*主hash函数*/
static int hash_func(char *key)
{
	int len, ret, i;

	if(!key)
		return 0;
	len = strlen(key);
	ret = key[0];
	for(i = 1; i < len; i ++)
		ret ^= key[i];
	ret ^= len;
	return (ret & 0x7FFFFFFF);
}

/*二次探测函数*/
static int second_hash(char *key)
{
	int len, ret, i;

	if(!key)
		return 0;
	len = strlen(key);
	ret = key[0] << 4 + key[0];
	for(i = 1; i < len; i ++)
		ret ^= key[i] << 4 + key[i];
	ret ^= len;
	return (ret & 0x7FFFFFFF);
}

static struct HashNode* _get_node()
{
	return (struct HashNode*)mm_malloc(sizeof(struct HashNode));
}

/*向hash表中插入节点，只能用于导入原有的hash表内容*/
static void _insert(struct HashTable *table, struct HashNode *node)
{
	int index = _get_empty(table, node->key);
	table->head[index] = node;
}

/*调整hash表的容量*/
static void _resize(struct HashTable *table)
{
	int size = table->capacity;
	int new_size = size << 1;
	int i;
	struct HashNode **head = table->head;

	/*重新分配hash表的存储空间*/
	table->head = (struct HashNode**)mm_malloc(new_size * sizeof(struct HashNode));
	memset(table->head, 0, new_size * sizeof(struct HashNode*));
	table->number = 0;
	table->capacity = new_size;
	/*导入原先的内容*/
	for(i = 0; i < size; i ++)
	{
		if(head[i] != NULL && head[i] != DUMMY)
		{
			_insert(table, head[i]);
			table->number ++;
		}
	}
	mm_free(head);
}

/**
 *  寻找制定key值对应的节点的索引
 **/
static int _find(struct HashTable *table, char* key)
{
	struct HashNode **head;
	int hash_value;
	int begin;
	if(!table || !key)
		return -1;
	head = table->head;
	hash_value = hash_func(key) % table->capacity;
	begin = hash_value;
	if(!head[hash_value])
		return -1;
	
	if(head[hash_value] == DUMMY || strcmp(key, head[hash_value]->key) != 0)
	{
		/*进行二次探测*/
		hash_value = second_hash(key) % table->capacity;
		if(!head[hash_value])
			return -1;
		//
		if(head[hash_value] == DUMMY || strcmp(key, head[hash_value]->key) != 0)
		{
			//hash_value = (hash_value + 1) % table->number;
			begin = hash_value;
			while(1)
			{
				hash_value = (hash_value + 1) % table->capacity;
				/*空节点或者找了一圈*/
				if(!head[hash_value] || begin == hash_value)
					return -1;
				if(head[hash_value] != DUMMY && strcmp(key, head[hash_value]->key) == 0)
					return hash_value;
			}

		}
		else
			return hash_value;
	}
	else
		return hash_value;
	
}

/*找到一个空闲的位置*/
static int _get_empty(struct HashTable *table, char *key)
{
	int hash_value ;
	struct HashNode **head;
	if(!table || !key)
		return -1;

	head = table->head;
	//一次探测
	hash_value = hash_func(key) % table->capacity;
	if(!head[hash_value] || head[hash_value] == DUMMY)
		return hash_value;
	//二次探测
	hash_value = second_hash(key) % table->capacity;
	if(!head[hash_value] || head[hash_value] == DUMMY)
		return hash_value;
	//递增hash_value探测
	while(1)
	{
		hash_value = (hash_value + 1) % table->capacity;
		if(!head[hash_value] || head[hash_value] == DUMMY)
			return hash_value;
	}
}

/*创建一个hash表*/
struct HashTable* hash_new(int pre_max/*预估的最大节点数量的1.5倍*/,
	int (*key_equal)(void *k1, void *k2), 
	void* (*copy_value)(void *src),
	void (*free_value)(void *value))
{
	struct HashTable* table = (struct HashTable*)mm_malloc(sizeof(struct HashTable));
	pre_max = (pre_max >= 3) ? pre_max : 3;
	table->head = (struct HashNode**)mm_malloc(pre_max * sizeof(struct HashNode*));
	memset(table->head, 0, pre_max * sizeof(struct HashNode*));
	table->key_equal = key_equal;
	table->copy_value = copy_value;
	table->free_value = free_value;
	table->capacity = pre_max;
	table->number = 0;
	return table;
}


/*向hash表中插入一组数据*/
void hash_insert(struct HashTable *table, char *key, void *value)
{
	int index;
	struct HashNode *node;

	if(!table || !key)
		return;

	index = _find(table, key);
	if(index == -1)
	{
		if(TRIGGER(table->capacity, table->number + 1))
			_resize(table);
		index = _get_empty(table, key);
		node = (struct HashNode*)mm_malloc(sizeof(struct HashNode));
		node->key = (char*)mm_malloc(strlen(key) + 1);
		strcpy(node->key, key);
		if(table->copy_value)
			node->value = table->copy_value(value);
		else
			node->value = value;
		table->head[index] = node;
		table->number ++;
	}
	else
	{
		
		if(table->free_value)
			table->free_value(table->head[index]->value);
		if(table->copy_value)
			table->head[index]->value = table->copy_value(value);
		else
			table->head[index]->value = value;
	}
}


/*获得指定key值的value*/
void* hash_get(struct HashTable *table, char *key)
{
	int index;
	if(!table || !key)
		return NULL;
	index = _find(table, key);
	if(index == -1)
		return NULL;
	return table->head[index]->value;
}

void hash_del(struct HashTable *table, char *key)
{
	int index;
	struct HashNode *node;
	if(!table || !key)
		return;
	index = _find(table, key);
	if(index == -1)
		return;
	node = table->head[index];
	mm_free(node->key);
	if(table->free_value)
		table->free_value(node->value);
	mm_free(node);
}

/**
 *  创建一个hash表的迭代器
 **/
HashIter* hash_create_iter(struct HashTable *table)
{
	struct HashIter *iter = (struct HashIter*)mm_malloc(sizeof(struct HashIter));
	if(!iter || !table)
		return NULL;
	iter->table = table;
	iter->index = 0;
	return iter;
}

/**
 * 
 **/
struct HashNode* hash_next(HashIter *iter)
{
	int i = 0;

	if(!iter)
		return NULL;
	for(i = iter->index; i < iter->table->capacity; i ++)
	{
		if(iter->table->head[i] != NULL && iter->table->head != DUMMY)
		{
			iter->index = i + 1;
			return iter->table->head[i];
		}
	}
	return NULL;
}


static int key_equal_str(void *k1, void *k2)
{
	char *_k1 = (char*)k1;
	char *_k2 = (char*)k2;
	return strcmp(_k1, _k2);
}

static void* copy_value_str(void *value)
{
	char *_v = (char*)value;
	char *ret;
	if(!_v)
		return NULL;
	ret = (char*)mm_malloc(strlen(_v) + 1);
	strcpy(ret, _v);
	return ret;
}

static void free_value_str(void *value)
{
	mm_free(value);
}

struct HashTable* hash_new_str(int pre_max/*预估的最大节点数量的1.5倍*/)
{
	return hash_new(pre_max, key_equal_str, copy_value_str, free_value_str);
}

/*向hash表中插入一组数据*/
void insert_str(struct HashTable *table, char *key, char *value)
{
	hash_insert(table, key, value);struct HashTable *var_table;
}

/*获得指定key值的value*/
char* get_str(struct HashTable *table, char *key)
{
	(char*)hash_get(table, key);
}

/*删除制定key值的节点*/
void del_str(struct HashTable *table, char *key)
{
	hash_del(table, key);
}

#ifdef HASH_DEBUG

int main(int argc, char **argv)
{
	char *kws[] = {"hello", "demo", "Ok", "fuck", "ass", "hole", "god", "charge", "hello", "bitch"};
	struct HashTable *table = hash_new(3, key_equal, copy_value, free_value);
	int i;
	for(i = 0; i < sizeof(kws) / sizeof(char*); i ++)
	{
		hash_insert(table, kws[i], kws[i]);
	}
	for(i = 0; i < sizeof(kws) / sizeof(char*); i ++)
	{
		hash_del(table, kws[i]);
	}
	printf("%s %s\n", (char*)hash_get(table, "hello"), (char*)hash_get(table, "fuck"));
	hash_insert(table, "hello", "bitch");
	printf("%s\n", (char*)hash_get(table, "hello"));

	return 0;
}
#endif