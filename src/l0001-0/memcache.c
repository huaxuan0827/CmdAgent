#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "memcache.h"

struct memcache* create_memcache(ssize_t sz, uint32_t count)
{
	struct memcache *mc;
	void *mem, *off;
	int i;

	mc = malloc(sizeof(struct memcache));
	if (mc == NULL) {
		goto err1;
	}
	memset(mc, 0, sizeof(struct memcache));
	
	if (pthread_rwlockattr_init(&(mc->rwlock_attr)) < 0) {
		goto err2;
	}
	if (pthread_rwlock_init(&(mc->rwlock),&(mc->rwlock_attr)) < 0) {
		goto err3;
	}

	mem = malloc((sz + sizeof(struct memcache_object)) * count);
	if (mem == NULL) {
		goto err4;
	}
	off = mem;
	memset(mem, 0, (sz + sizeof(struct memcache_object)) * count);
	mc->count = count;
	mc->unused_count = 0;

	INIT_LIST_HEAD(&mc->unused_list);
	INIT_LIST_HEAD(&mc->used_list);

	for (i = 0;i < count;i ++) {
		struct memcache_object *mo = off;
		INIT_LIST_HEAD(&mo->node);
		list_add(&mo->node, &mc->unused_list);
		off = (void*)((unsigned long)off + (unsigned long)(sz + sizeof(struct memcache_object)));
	}

	return mc;
err4:
	pthread_rwlock_destroy(&(mc->rwlock));
err3:
	pthread_rwlockattr_destroy(&(mc->rwlock_attr));
err2:
	free(mc);
err1:
	return NULL;
}

void* alloc_memcache(struct memcache *mc)
{
	void* retval = NULL;
	struct memcache_object *mo;
	//lock
	pthread_rwlock_wrlock(&mc->rwlock);
	if (mc->unused_count) {
		mo = list_entry(mc->unused_list.next, struct memcache_object, node);
		retval = (void*)&mo->block;
		list_del(&mo->node);
		mc->unused_count --;
	}
	pthread_rwlock_unlock(&mc->rwlock);
	return retval;
}

void free_memcache(struct memcache *mc, void *block)
{
	struct memcache_object *mo;
	//lock
	pthread_rwlock_wrlock(&mc->rwlock);
	mo = container_of(block, struct memcache_object, block);
	list_add(&mo->node, &mc->unused_list);
	mc->unused_count ++;
	pthread_rwlock_unlock(&mc->rwlock);
}

/* return unused memcache object */
uint32_t get_memcache_count(struct memcache *mc)
{
	return mc->unused_count;
}

void destroy_memcache(struct memcache *mc)
{
	INIT_LIST_HEAD(&mc->unused_list);
	INIT_LIST_HEAD(&mc->used_list);
	free(mc->mo);
	free(mc);
	pthread_rwlock_destroy(&(mc->rwlock));
	pthread_rwlockattr_destroy(&(mc->rwlock_attr));
}

