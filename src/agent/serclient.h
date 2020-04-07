#ifndef __SERCLT_H__
#define __SERCLT_H__

#include <stdint.h>
#include "SimuList.h"

#define SERCLT_RDBUFFER_SIZE		   204800

struct serclt_op{
	//callback functions
	void *param;

	//void *param, const char *szdevip, unsigned short usport
	int (*registerdevice)(void*,const char *, unsigned short);
	//void *param, const char *szdevip, unsigned short usport, int serid, void *data, int len
	int (*transmitpacket)(void *,const char *, unsigned short, int, void *,int);
};

struct serclt_msg{
	int msgcmd;
	int seqno;
	time_t sendtime;
};

struct serclt_info{		
	int nsocket;
	char netpath[32];
	struct bufferevent *evbuffer;

	char devip[32];
	unsigned short devport;
	uint8_t cltid;

	uint8_t *data_blob;
	uint32_t rd_off;
	uint32_t wr_off;

	uint32_t error_count;
	//callback functions
	struct serclt_op net_op;
};

int serclt_initialize(struct serclt_info *serclt, struct serclt_op *op);
void serclt_release(struct serclt_info *serclt);
int serclt_recvdata(struct serclt_info *serclt);
int serclt_writedata(struct serclt_info *serclt, void *data, int len);

#endif


