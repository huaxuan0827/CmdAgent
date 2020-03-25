#ifndef __SERCLT_H__
#define __SERCLT_H__
#include "SimuList.h"

#define SERCLT_RDBUFFER_SIZE		   204800

struct serclt_op{
	//callback functions
	void *param;
	//int (*timeout)(void*);	//void *op_param
	//int (*verifycmdpacket)(void*, uint8_t*, uint32_t, uint32_t*);//void *op_param, uint8_t *packet, uint32_t len, uint32_t *seqno
	//int (*verifystream)(void*, uint8_t*, uint32_t);	//void *op_param, uint8_t *packet, uint32_t len
	//int (*isvalidpacketheader)(void*, uint8_t*, uint32_t);//void *op_param, uint8_t *packet, uint32_t len
	//int (*getpacketheaderlength)(void*);
	//void *op_param,uint8_t *packet,uint32_t len,int seqno,int serid,const char *devip
	//int (*dealpacket)(void*, uint8_t*, uint32_t, int, int ,const char *);
	// const char *szdevip,void *data, int len, int serid, int seqno
	int (*transmitpacket)(const char *,uint8_t *, int, int, int);
};

struct serclt_msg{
	int msgcmd;
	int seqno;
	time_t sendtime;
};

struct serclt_info{		
	int nsocket;
	struct bufferevent *evbuffer;

	uint8_t *data_blob;
	uint32_t rd_off;
	uint32_t wr_off;

	uint32_t error_count;
	//callback functions
	struct serclt_op net_op;
};

int serclt_initialize(struct serclt_info *serclt, struct serclt_op *op);
void serclt_release(struct serclt_info *serclt);
int serclt_recvata(struct serclt_info *serclt);
int serclt_writedata(struct serclt_info *serclt, void *data, int len);

#endif


