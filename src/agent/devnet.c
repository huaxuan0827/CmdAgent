#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <string.h>

#include "cmddef.h"
#include "devnet.h"

#include "l0001-0/l0001-0.h"
#include "l0002-0/l0002-0.h"
#include "l0003-0/l0003-0.h"

#define MODNAME	 "[A15DN]"

static void _devnet_readcb(struct bufferevent *bev, void *ctx);
static void _devnet_eventcb(struct bufferevent *bev, short events, void *ctx);
static void _devnet_msglistclear(SimuList_t *msglist);

void _devnet_readcb(struct bufferevent *bev, void *ctx)
{
	struct devnet_info *devnet = ctx;
	size_t nread, remain_len;
	uint32_t space_to_end;
	uint8_t *packet;
	struct cmd_packet *packet_head;
	int errorflag = 0;
	
	space_to_end = DEVNET_RDBUFFER_SIZE - devnet->wr_off;
	nread = bufferevent_read(bev, devnet->data_blob + devnet->wr_off, space_to_end);
	if( nread > 0){
		devnet->wr_off += nread;
	}

	while( devnet->wr_off - devnet->rd_off > CMD_PACKET_HEAD_LEN){
		packet = devnet->data_blob + devnet->rd_off;
		packet_head = (struct cmd_packet *)packet;
		remain_len = devnet->wr_off- devnet->rd_off;

		if( packet_head->f.length >= CMD_PACKET_MAX_LENGTH || packet_head->f.length < CMD_PACKET_HEAD_LEN){
			errorflag = 1;
			ERRSYS_INFOPRINT("[%s-%d] recv error packet length:%d \n",devnet->ipaddr, devnet->usport,packet_head->f.length);
			break;
		}
		if( packet_head->f.length > remain_len){
			break;
		}

		if( devnet->net_op.dealpacket != NULL){
			SimuListNode_t *pnode = NULL;
			struct devnet_msg *pmsg = NULL;
			int nserid = -1;

			LIST_WLOCK(devnet);
			pnode = SimuList_Get(&devnet->msglist, packet_head->seqno);
			if(pnode != NULL){
				pmsg = (struct devnet_msg *)pnode->pData;
				nserid = pmsg->nserid;
				SimuList_Del(&devnet->msglist, packet_head->seqno, NULL);
			}
			LIST_UNLOCK(devnet);
			if(nserid > 0){
				devnet->net_op.dealpacket(devnet->net_op.param, nserid, packet_head->seqno, packet, packet_head->f.length);
			}else{
				ERRSYS_INFOPRINT("[%s-%d] recv missing packet, seqno:%d, length:%d\n",devnet->ipaddr, devnet->usport, 
					packet_head->seqno, packet_head->f.length);
			}
		}
		devnet->rd_off += packet_head->f.length;
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

void _devnet_eventcb(struct bufferevent *bev, short events, void *ctx)
{
	struct devnet_info *devnet = ctx;
	
	if(events & BEV_EVENT_CONNECTED) {
		evutil_socket_t fd = bufferevent_getfd(bev);
		int one = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
		devnet->flags |= DEVNET_FLAGS_CONNECTED;

		printf("connet %s-%d sucess!\n", devnet->ipaddr, devnet->usport);
		ERRSYS_INFOPRINT("connet %s-%d sucess!\n", devnet->ipaddr, devnet->usport);
	}else if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
		if (events & BEV_EVENT_ERROR) {
			ERRSYS_ERRPRINT("%s-%d devcom BEV_EVENT_ERROR\n", devnet->ipaddr, devnet->usport);
		}
		if (events & BEV_EVENT_EOF) {
			ERRSYS_ERRPRINT("%s-%d devcom BEV_EVENT_EOF\n", devnet->ipaddr, devnet->usport);
		}
		devnet->flags &= ~DEVNET_FLAGS_CONNECTED;
		event_base_loopbreak(devnet->base);
		ERRSYS_ERRPRINT("%s-%d devcom disconnected\n", devnet->ipaddr, devnet->usport);
	}
	else {
		ERRSYS_ERRPRINT("%s-%d devcom unknown event %d\n",devnet->ipaddr, devnet->usport, events);
	}
}

void _devnet_msglistclear(SimuList_t *msglist)
{
	SimuListNode_t *p1;
	SimuListNode_t *p2;

	struct devnet_msg *msg = NULL;
	time_t tnow = time(NULL);
	
	p1 = msglist->pHead->pNext;
	p2 = msglist->pHead;
	while( p1 != NULL){
		msg = (struct devnet_msg *)p1->pData;
		if(tnow - msg->reqtime > CMD_MAX_RESP_TIME_SEC){
			p2->pNext = p1->pNext;
			ERRSYS_WARNPRINT("Req Msg, seqno:%d, serid:%d, timeout!!!, not recv resp!!", msg->nseqno, msg->nserid);
			free(msg);
			free(p1);
			msglist->nCount--;
			p1 = p2->pNext;
		}else{
			p2 = p1;
			p1 = p1->pNext;
		}
	}
}

int devnet_initialize(struct devnet_info *devnet,const char *ipaddr, int port, struct devnet_op *op)
{
	int retval;
	uint8_t *blob;

	if ((retval = pthread_rwlockattr_init(&(devnet->rwlock_attr))) < 0) {
		ERRSYS_FATALPRINT("fail to initialize rwlock attribute for devnet: %s\n", strerror(errno));
		goto err1;
	}
	if ((retval = pthread_rwlock_init(&(devnet->rwlock),&(devnet->rwlock_attr))) < 0) {
		ERRSYS_FATALPRINT("fail to initialize rwlock for devnet: %s\n", strerror(errno));
		goto err2;
	}

	devnet->data_blob = blob = zmalloc(DEVNET_RDBUFFER_SIZE);
	if( blob == NULL){
		goto err3;
	}

	devnet->base = event_base_new();
	if (devnet->base == NULL) {
		ERRSYS_FATALPRINT("fail to new event base!\n");
		TAKEABREATH();
		goto err4;
	}

	SimuList_Create(&devnet->msglist);

	devnet->net_op.dealpacket = op->dealpacket;
	devnet->flags = DEVNET_FLAGS_DROPADDATA;	
	strcpy(devnet->ipaddr, ipaddr);
	devnet->usport = port;

	devnet->net_op.dealpacket = op->dealpacket;
	devnet->net_op.param = op->param;
	
	return 0;
err4:
	free(devnet->data_blob);
err3:	
	pthread_rwlock_destroy(&devnet->rwlock);
err2:
	pthread_rwlockattr_destroy(&devnet->rwlock_attr);
err1:
	return -1;
}

void devnet_release(struct devnet_info *devnet)
{
	if (devnet->base) {
		event_base_loopbreak(devnet->base);
		event_base_free(devnet->base);
		devnet->base = NULL;
	}
	free(devnet->data_blob);
	pthread_rwlockattr_destroy(&devnet->rwlock_attr);
	pthread_rwlock_destroy(&devnet->rwlock);
}

int devnet_connect(struct devnet_info *devnet)
{
	int retval = -1;
	if (devnet->bev) {
		bufferevent_free(devnet->bev);
		devnet->bev = NULL;
	}
	devnet->bev = bufferevent_socket_new(devnet->base, -1, BEV_OPT_CLOSE_ON_FREE);
	if (devnet->bev == NULL) {
		goto err1;
	}

	bufferevent_setcb(devnet->bev, _devnet_readcb, NULL, _devnet_eventcb, devnet);
	bufferevent_enable(devnet->bev, EV_READ | EV_WRITE | EV_PERSIST);

	ERRSYS_INFOPRINT("devnet connect %s:%d\n", devnet->ipaddr, devnet->usport);
	if (bufferevent_socket_connect_hostname(devnet->bev, NULL, AF_INET, devnet->ipaddr, devnet->usport) < 0) {
		ERRSYS_ERRPRINT("fail to connect %s:%d\n", devnet->ipaddr, devnet->usport);
		goto err2;
	}
	return 0;
err2:
	bufferevent_free(devnet->bev);
	devnet->bev = NULL;
err1:
	return retval;	
}

int devnet_isconnected(struct devnet_info *devnet)
{
	if( devnet == NULL || devnet->bev == NULL){
		return 0;
	}
	if( devnet->flags & DEVNET_FLAGS_CONNECTED){
		return 1;
	}
	return 0;
}

int devnet_write(struct devnet_info *devnet,int serid, uint8_t seqno, void *data, int len)
{
	int retval = -1;
	SimuListNode_t *pnode = NULL;
	struct devnet_msg *pmsg = NULL;

	if(!devnet_isconnected(devnet)){
		ERRSYS_WARNPRINT("write data, but devnet disconnect\n");
		return -1;
	}

	LIST_WLOCK(devnet);
	_devnet_msglistclear(&devnet->msglist);
	pnode = SimuList_Get(&devnet->msglist, seqno);
	if(pnode != NULL){
		ERRSYS_WARNPRINT("find the same seqno:%d \n",seqno);
		LIST_UNLOCK(devnet);
		return -1;
	}
	
	struct evbuffer *output = bufferevent_get_output(devnet->bev);
	retval = evbuffer_add(output, data, len);
	if (retval == -1) {
		ERRSYS_ERRPRINT("fail to send data length %d\n", len);
	}else{
		pmsg = zmalloc(sizeof(struct devnet_msg));
		pmsg->nseqno = seqno;
		pmsg->nserid = serid;
		pmsg->reqtime = time(NULL);
		SimuList_Add(&devnet->msglist, seqno, pmsg, 0);
		printf("MsgList count:%d \n", devnet->msglist.nCount);
	}
	LIST_UNLOCK(devnet);
	return retval;
}

void devnet_disconnect(struct devnet_info *devnet)
{
	if (devnet->base) {
		event_base_loopbreak(devnet->base);
	}
	if (devnet->bev) {
		bufferevent_free(devnet->bev);
		devnet->bev = NULL;
	}
}

void devnet_loop(struct devnet_info *devnet)
{
	event_base_dispatch(devnet->base);
}

void devnet_breakloop(struct devnet_info *devnet)
{
	event_base_loopbreak(devnet->base);
}

