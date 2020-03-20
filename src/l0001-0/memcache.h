#ifndef __MEMCACHE_H__
#define __MEMCACHE_H__

#include "include/list.h"

struct memcache_object {
	struct list_head node;
	char block;
};

struct memcache {
	pthread_rwlockattr_t rwlock_attr;
	pthread_rwlock_t rwlock;
	struct list_head used_list;
	struct list_head unused_list;
	uint32_t unused_count;
	uint32_t count;
	void *mo;
};
#endif
