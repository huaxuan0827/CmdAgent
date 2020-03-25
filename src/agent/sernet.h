#ifndef __SERNET_H__
#define __SERNET_H__

#include "serclient.h"

#define SERNET_RDBUFFER_SIZE		   204800

#define SERNET_FLAGS_DROPADDATA				0
#define SERNET_FLAGS_RECEIVEALL				1
#define SERNET_FLAGS_CONNECTED				2

#define IS_SERNET_DROPADDATA(devnet)		((devnet)->flags & DEVNET_FLAGS_DROPADDATA)
#define IS_SERNET_CONNECTED(devnet)			((devnet)->flags & DEVNET_FLAGS_CONNECTED)

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

int sernet_initialize(struct devnet_info *devnet, const char *netpath, struct serclt_op *op);
void sernet_release(struct devnet_info *devnet);

int sernet_write(struct devnet_info *devnet, void *data, int len, int serid, int seqno);
void sernet_loop(struct devnet_info *devnet);
void sernet_breakloop(struct devnet_info *devnet);

#endif

