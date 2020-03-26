#ifndef __SERNET_H__
#define __SERNET_H__

#include "serclient.h"

struct sernet_info{
	char serpath[32];

	uint32_t flags;		
	struct event_base *evbase;
	struct evconnlistener* evlistener;

	pthread_rwlockattr_t rwlock_attr;
	pthread_rwlock_t rwlock;
	SimuList_t serclt_list;
	
	struct serclt_op ser_op;
};

int sernet_initialize(struct sernet_info *sernet, const char *netpath, struct serclt_op *op);
void sernet_release(struct sernet_info *sernet);

int sernet_write(struct sernet_info *sernet,int serid, int seqno, void *data, int len);
void sernet_loop(struct sernet_info *sernet);
void sernet_breakloop(struct sernet_info *sernet);

#endif

