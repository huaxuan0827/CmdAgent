#ifndef __DEVNET_H__
#define __DEVNET_H__

#define DEVNET_RDBUFFER_SIZE		   204800

#define DEVNET_FLAGS_DROPADDATA				0
#define DEVNET_FLAGS_RECEIVEALL				1
#define DEVNET_FLAGS_CONNECTED				2

#define IS_DEVNET_DROPADDATA(devnet)		((devnet)->flags & DEVNET_FLAGS_DROPADDATA)
#define IS_DEVNET_CONNECTED(devnet)			((devnet)->flags & DEVNET_FLAGS_CONNECTED)

struct devnet_op{
	//callback functions
	int (*timeout)(void*);	//void *op_param
	int (*verifycmdpacket)(void*, uint8_t*, uint32_t, uint32_t*);//void *op_param, uint8_t *packet, uint32_t len, uint32_t *seqno
	int (*verifystream)(void*, uint8_t*, uint32_t);	//void *op_param, uint8_t *packet, uint32_t len
	int (*isvalidpacketheader)(void*, uint8_t*, uint32_t);//void *op_param, uint8_t *packet, uint32_t len
	int (*getpacketheaderlength)(void*);
	int (*dealpacket)(void*, uint8_t*, uint32_t, uint32_t);//void *op_param, uint8_t *packet, uint32_t len, uint32_t seqno
};

struct devnet_info{
	char ipaddr[24];
	unsigned short usport;

	uint32_t flags;		
	struct event_base *base;
	struct bufferevent *bev;
	struct event *evtimeout;

	pthread_rwlockattr_t rwlock_attr;
	pthread_rwlock_t rwlock;
	
	uint8_t *data_blob;
	uint32_t rd_off;
	uint32_t wr_off;

	uint32_t error_count;
	//callback functions
	void *op_param;
	struct devnet_op net_op;
};

int devnet_initialize(struct devnet_info *devnet, const char *ipaddr, int port, void* param, struct devnet_op *op);
void devnet_release(struct devnet_info *devnet);

int devnet_connect(struct devnet_info *devnet);
int devnet_isconnected(struct devnet_info *devnet);
int devnet_write(struct devnet_info *devnet, void *data, int len);
void devnet_disconnect(struct devnet_info *devnet);
void devnet_loop(struct devnet_info *devnet);
void devnet_breakloop(struct devnet_info *devnet);

#endif
