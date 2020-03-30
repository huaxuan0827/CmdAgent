#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <string.h>

#include "l0001-0/l0001-0.h"
#include "l0002-0/l0002-0.h"
#include "l0003-0/l0003-0.h"

#include "cmddef.h"
#include "serclient.h"

#define MODNAME	 "[A15SC]"

int serclt_initialize(struct serclt_info *serclt, struct serclt_op *op)
{
	serclt->data_blob = zmalloc(SERCLT_RDBUFFER_SIZE);
	if(serclt->data_blob == NULL){
		return -1;
	}
	serclt->wr_off = 0;
	serclt->rd_off = 0;
	serclt->error_count = 0;

	serclt->devport = 0;
	
	serclt->net_op.param = op->param;
	serclt->net_op.transmitpacket = op->transmitpacket;
	return 0;
}

void serclt_release(struct serclt_info *serclt)
{
	if( serclt->evbuffer != NULL){
		bufferevent_free(serclt->evbuffer);
	}
}

int serclt_recvdata(struct serclt_info *serclt)
{
	size_t nread, remain_len;
	uint32_t space_to_end;
	uint8_t *packet;
	struct agent_packet *agent_head;
	struct cmd_packet *cmd_head;
	int errorflag = 0;
	
	space_to_end = SERCLT_RDBUFFER_SIZE - serclt->wr_off;
	nread = bufferevent_read(serclt->evbuffer, serclt->data_blob + serclt->wr_off, space_to_end);
	if( nread > 0){
		serclt->wr_off += nread;
	}

	while( serclt->wr_off - serclt->rd_off >= CMD_PACKET_HEAD_LEN){
		packet = serclt->data_blob + serclt->rd_off;
		cmd_head = (struct cmd_packet*)(packet);

		if(serclt->devport <= 0 && cmd_head->magic == AGENT_INIT_PACKET_HEAD_MAGIC){
			agent_head = (struct agent_packet *)(packet);
			sprintf(serclt->devip, "%u.%u.%u.%u", agent_head->devip[0], agent_head->devip[1], agent_head->devip[2], agent_head->devip[3]);
			serclt->devport = agent_head->port;
			serclt->rd_off += AGENT_PACKET_HEAD_LEN;
			ERRSYS_INFOPRINT("serclt:%d, register devip:%s, port:%d \n", serclt->nsocket, serclt->devip, serclt->devport);
			continue;
		}
		
		if( serclt->devport <= 0){
			ERRSYS_INFOPRINT("serclt:%d, not register devip port, but recv cmd:%d, seqno:%d, len:%d, refuse agent!!\n", 
				serclt->nsocket, cmd_head->cmd, cmd_head->seqno, serclt->wr_off- serclt->rd_off);
			serclt->error_count++;

			serclt->wr_off = 0;
			serclt->rd_off = 0;	
			break;
		}

		remain_len = serclt->wr_off- serclt->rd_off;
		packet = serclt->data_blob + serclt->rd_off;
		if( serclt->net_op.transmitpacket != NULL){
			serclt->net_op.transmitpacket(serclt->net_op.param,serclt->devip,serclt->devport,serclt->nsocket, packet, remain_len);
		}
		serclt->wr_off = 0;
		serclt->rd_off = 0;	
	}
}

int serclt_writedata(struct serclt_info *serclt, void *data, int len)
{
	struct evbuffer *output = bufferevent_get_output(serclt->evbuffer);
	int retval = evbuffer_add(output, data, len);
	if (retval == -1) {
		ERRSYS_ERRPRINT("fail to send data length %d\n", len);
	}
	return retval;
}

