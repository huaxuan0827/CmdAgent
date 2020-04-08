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

void dump_file(uint8_t *proto, uint32_t size, struct serclt_info *serclt, int flag, int bPrint)
{
	int i;
	static FILE* fp = NULL;
	static int nCount = 0;
	struct tm *tmp_ptr = NULL;
	time_t tnow;
	char time_str[256];
	char tmp_str[256] = {0};
	
 	time(&tnow);
 	tmp_ptr = gmtime(&tnow);
	sprintf(time_str, "%02d%02d_%02d%02d%02d",tmp_ptr->tm_mon, 
			tmp_ptr->tm_mday, 
			tmp_ptr->tm_hour, 
			tmp_ptr->tm_min,
			tmp_ptr->tm_sec);
  
	if( fp == NULL || nCount > 10*1024*1024){
		if( fp != NULL){
			fclose(fp);
			fp = NULL;
		}
		char szFileName[256] = {0};

		mkdir("/tmp/log_dump",0755);
		sprintf(szFileName, "/tmp/log_dump/agentdata_dump_%s",time_str);
		if(bPrint)
			printf("create log_dump filename %s \n", szFileName);
		fp = fopen(szFileName, "w");
		nCount = 0;
	}
	if( fp == NULL){
		printf("fp == NULL \n");
		return;
	}

	if( flag == 1){
		sprintf(tmp_str, "[%s] size=%d, clt:%d-socket:%d => dev:%s-port:%d\n", time_str, size, serclt->cltid, serclt->nsocket, serclt->devip, serclt->devport);
	}else{
		sprintf(tmp_str, "[%s] size=%d, dev:%s-port:%d => clt:%d-socket:%d\n", time_str, size, serclt->devip, serclt->devport, serclt->cltid, serclt->nsocket);
	}
	if(bPrint)
		printf("%s", tmp_str);
	fprintf(fp, "%s", tmp_str);
	
	nCount += size;
	for (i = 0;i < size;i++){
		fprintf(fp, "%02x ", proto[i]);
		if(bPrint)printf("%02x ", proto[i]);
		if ((i & 0xF) == 0xF) {
			fprintf(fp,"\n");
			if(bPrint)printf("\n");
		}
	}
	fprintf(fp, "\n");
	if(bPrint)printf("\n");
	
	fflush(fp);
}



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
	serclt->net_op.registerdevice = op->registerdevice;
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
	int nret = -1;
	
	space_to_end = SERCLT_RDBUFFER_SIZE - serclt->wr_off;
	nread = bufferevent_read(serclt->evbuffer, serclt->data_blob + serclt->wr_off, space_to_end);
	if( nread > 0){
		serclt->wr_off += nread;
	}

	while( serclt->wr_off - serclt->rd_off >= CMD_PACKET_HEAD_LEN){
		packet = serclt->data_blob + serclt->rd_off;
		cmd_head = (struct cmd_packet*)(packet);

		if(/*serclt->devport <= 0 && */cmd_head->magic == AGENT_INIT_PACKET_HEAD_MAGIC){
			agent_head = (struct agent_packet *)(packet);
			sprintf(serclt->devip, "%u.%u.%u.%u", agent_head->devip[0], agent_head->devip[1], agent_head->devip[2], agent_head->devip[3]);
			serclt->devport = agent_head->port;
			serclt->cltid = agent_head->cltid;
			serclt->rd_off += AGENT_PACKET_HEAD_LEN;

			ERRSYS_INFOPRINT("SERCLT=>clt:%d-socket:%d, new devip:%s, port:%d\n",serclt->cltid, serclt->nsocket, serclt->devip, serclt->devport);		
			if( serclt->net_op.registerdevice != NULL){
				nret = serclt->net_op.registerdevice(serclt->net_op.param, serclt->devip, serclt->devport);
				ERRSYS_INFOPRINT("SERCLT=>clt:%d-socket:%d, register devip:%s, port:%d, nret:%d\n",serclt->cltid, serclt->nsocket, serclt->devip, 
						serclt->devport, nret);
			}
			continue;
		}
		
		if( serclt->devport <= 0){
			ERRSYS_INFOPRINT("SERCLT=>clt:%d-socket:%d, not register devip port, but recv cmd:0x%x, seqno:%d, len:%d, refuse agent!!\n", 
					serclt->cltid, serclt->nsocket, cmd_head->cmd, cmd_head->seqno, serclt->wr_off- serclt->rd_off);
			serclt->error_count++;

			serclt->wr_off = 0;
			serclt->rd_off = 0;	
			break;
		}

		remain_len = serclt->wr_off- serclt->rd_off;
		packet = serclt->data_blob + serclt->rd_off;
		if( serclt->net_op.transmitpacket != NULL){
			nret = serclt->net_op.transmitpacket(serclt->net_op.param,serclt->devip,serclt->devport,serclt->nsocket, packet, remain_len);
			struct cmd_packet *pCmd = (struct cmd_packet *)packet;
			ERRSYS_INFOPRINT("SERCLT=>send to dev[%s:%d] by clt:%d-socket:%d - cmd:0x%x- seqno:%d, cmdLen:%d-0x%x, nret=%d\n", serclt->devip, serclt->devport, 
						serclt->cltid, serclt->nsocket, pCmd->cmd, pCmd->seqno, remain_len, pCmd->f.length, nret);
			dump_file(packet, remain_len, serclt, 1, 1);
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
		ERRSYS_ERRPRINT("SERCLT=>clt:%d-socket:%d fail to send data length %d\n",serclt->cltid, serclt->nsocket, len);
	}
	struct cmd_packet *pCmd = (struct cmd_packet *)data;
	ERRSYS_INFOPRINT("SERCLT=>recv from dev[%s:%d] to clt:%d-socket:%d - cmd:0x%x- seqno:%d, cmdLen:%d-0x%x, nret=%d\n", serclt->devip, serclt->devport, 
						serclt->cltid, serclt->nsocket, pCmd->cmd, pCmd->seqno, len, pCmd->f.length, retval);
	dump_file(data, len, serclt, 0, 1);
	return retval;
}

