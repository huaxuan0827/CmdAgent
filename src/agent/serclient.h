#ifndef __SERCLT_H__
#define __SERCLT_H__
#include "SimuList.h"

#define SERCLT_RDBUFFER_SIZE		   204800

#define SERCLT_FLAGS_DROPADDATA			0
#define SERCLT_FLAGS_RECEIVEALL			1
#define SERCLT_FLAGS_CONNECTED			2

#define IS_SERCLT_DROPADDATA(devnet)		((devnet)->flags & DEVNET_FLAGS_DROPADDATA)
#define IS_SERCLT_CONNECTED(devnet)			((devnet)->flags & DEVNET_FLAGS_CONNECTED)

struct serclt_op{
	//callback functions
	int (*timeout)(void*);	//void *op_param
	int (*verifycmdpacket)(void*, uint8_t*, uint32_t, uint32_t*);//void *op_param, uint8_t *packet, uint32_t len, uint32_t *seqno
	int (*verifystream)(void*, uint8_t*, uint32_t);	//void *op_param, uint8_t *packet, uint32_t len
	int (*isvalidpacketheader)(void*, uint8_t*, uint32_t);//void *op_param, uint8_t *packet, uint32_t len
	int (*getpacketheaderlength)(void*);
	int (*dealpacket)(void*, uint8_t*, uint32_t, uint32_t);//void *op_param, uint8_t *packet, uint32_t len, uint32_t seqno
};

struct serclt_msg{
	int msgcmd;
	int seqno;
	time_t sendtime;
};

struct serclt_info{	
	char ipaddr[32];
	int ndevid;
	
	int nsocket;
	uint32_t flags;		
	struct bufferevent *evbuffer;

	uint8_t *data_blob;
	uint32_t rd_off;
	uint32_t wr_off;

	uint32_t error_count;
	//callback functions
	void *op_param;
	struct devnet_op net_op;
};

int serclt_initialize(struct serclt_info *serclt, void* param, struct serclt_op *op);
void serclt_release(struct serclt_info *serclt);

int serclt_connect(struct devnet_info *serclt);
int serclt_isconnected(struct serclt_info *serclt);
int serclt_recvdata(struct serclt_info *serclt, void *data, int len);
int serclt_writedata(struct serclt_info *serclt, void *data, int len);
void serclt_disconnect(struct serclt_info *serclt);

#endif


