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

int serclt_recvdata(struct serclt_info *serclt, void *data, int len)
{
	size_t nread, remain_len;
	uint32_t sapce, space_to_end;
	uint8_t *packet;
	struct agent_packet *agent_head;
	struct cmd_packet *cmd_head;
	int errorflag = 0;
	char ipaddr[32];
	
	space_to_end = SERCLT_RDBUFFER_SIZE - serclt->wr_off;
	nread = bufferevent_read(serclt->evbuffer, serclt->data_blob + serclt->wr_off, space_to_end);
	if( nread > 0){
		serclt->wr_off += nread;
	}

	while( serclt->wr_off - serclt->rd_off > CMD_PACKET_HEAD_LEN + AGENT_PACKET_HEAD_LEN){
		packet = serclt->data_blob + serclt->rd_off;
		agent_head = (struct agent_packet *)packet;
		cmd_head = (struct cmd_packet*)(packet + AGENT_PACKET_HEAD_LEN);
		remain_len = serclt->wr_off - AGENT_PACKET_HEAD_LEN - serclt->rd_off;

		sprintf(ipaddr, "%u.%u.%u.%u", agent_head->devip[0], agent_head->devip[1], agent_head->devip[2], agent_head->devip[3]);
		if( cmd_head->f.length >= CMD_PACKET_MAX_LENGTH || cmd_head->f.length < CMD_PACKET_HEAD_LEN){
			errorflag = 1;
			ERRSYS_INFOPRINT("recv error packet length:%d , devip:%s\n",cmd_head->f.length, ipaddr);
			break;
		}
		if( cmd_head->f.length > remain_len){
			break;
		}

		if( serclt->net_op.transmitpacket != NULL){
			serclt->net_op.transmitpacket(serclt->op_param, (void *)cmd_head, cmd_head.f.length, cmd_head.seqno, serclt->nsocket, ipaddr);
		}
		serclt->rd_off += AGENT_PACKET_HEAD_LEN;
		serclt->rd_off += cmd_head->f.length;
	}

	if(errorflag){
		// reset buffer!
		ERRSYS_INFOPRINT("[%d] recv data error, now will reset, wr_off:%d, rd_off:%d !\n", serclt->nsocket,
			serclt->wr_off, serclt->rd_off);
		evbuffer_drain(bufferevent_get_input(serclt->evbuffer), -1);
		serclt->wr_off = 0;
		serclt->rd_off = 0;
		serclt->error_count++;	
		errorflag = 0;
	}

	space_to_end = DEVNET_RDBUFFER_SIZE - serclt->wr_off;
	if( space_to_end < CMD_PACKET_MAX_LENGTH){
		ERRSYS_INFOPRINT("[%d] wr_off:%d, rd_off:%d, sapce_to_end:%d\n", serclt->nsocket,
			serclt->wr_off, serclt->rd_off, space_to_end);
		memmove(serclt->data_blob, serclt->data_blob + serclt->rd_off, serclt->wr_off - serclt->rd_off);
		serclt->rd_off = 0;
		serclt->wr_off = serclt->wr_off - serclt->rd_off;
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

