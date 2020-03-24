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

int serclt_initialize(struct serclt_info *serclt, void* param, struct serclt_op *op);
void serclt_release(struct serclt_info *serclt);

int serclt_recvdata(struct serclt_info *serclt, void *data, int len)
{
	uint8 *packet = NULL;
	struct agent_packet *agent_head;
	struct cmd_packet *cmd_head;
	int remain_len = 0;
	int errorflag = 0;
	int space_to_end = DEVNET_RDBUFFER_SIZE - serclt->wr_off;

	if( len > space_to_end){
		memmove(serclt->data_blob, serclt->data_blob + serclt->wr_off, serclt->rd_off - serclt->wr_off);
		serclt->wr_off = serclt->rd_off - serclt->wr_off;
		space_to_end = DEVNET_RDBUFFER_SIZE - serclt->wr_off;
	}
	
	if( len < space_to_end){
		memcpy(serclt->data_blob + serclt->wr_off, data, len);
	}else{
		return -1;
	}
	
	while( serclt->wr_off - serclt->rd_off > CMD_PACKET_HEAD_LEN + AGENT_PACKET_HEAD_LEN){
		packet = serclt->data_blob + serclt->rd_off;
		agent_head = (struct agent_packet *)packet;
		cmd_head = (struct cmd_packet*)(packet + AGENT_PACKET_HEAD_LEN);
		
		remain_len = serclt->wr_off - AGENT_PACKET_HEAD_LEN - serclt->rd_off;

		if( cmd_head->f.length >= CMD_PACKET_MAX_LENGTH || cmd_head->f.length < CMD_PACKET_HEAD_LEN + ){
			errorflag = 1;
			//ERRSYS_INFOPRINT("[%s-%d] recv error packet length:%d \n",devnet->ipaddr, devnet->usport,packet_head->f.length);
			break;
		}
		if( cmd_head->f.length > remain_len){
			break;
		}

		if( serclt->net_op.dealpacket != NULL){
			//serclt->net_op.dealpacket(serclt->op_param,packet_head);
		}

		serclt->rd_off += AGENT_PACKET_HEAD_LEN;
		serclt->rd_off += cmd_head->f.length;
	}

	if(errorflag){
		// reset buffer!
		ERRSYS_INFOPRINT("[%s-%d] recv data error, now will reset, wr_off:%d, rd_off:%d !\n", devnet->ipaddr, devnet->usport,
			devnet->wr_off, devnet->rd_off);
		evbuffer_drain(bufferevent_get_input(bev), -1);
		devnet->wr_off = 0;
		devnet->rd_off = 0;
		devnet->error_count++;	
		errorflag = 0;
	}

	space_to_end = DEVNET_RDBUFFER_SIZE - devnet->wr_off;
	if( space_to_end < CMD_PACKET_MAX_LENGTH){
		ERRSYS_INFOPRINT("[%s-%d] wr_off:%d, rd_off:%d, sapce_to_end:%d\n", devnet->ipaddr, devnet->usport,
			devnet->wr_off, devnet->rd_off, space_to_end);
		memmove(devnet->data_blob, devnet->data_blob + devnet->rd_off, devnet->wr_off - devnet->rd_off);
		devnet->rd_off = 0;
		devnet->wr_off = devnet->wr_off - devnet->rd_off;
	}


}
int serclt_writedata(struct serclt_info *serclt, void *data, int len);

