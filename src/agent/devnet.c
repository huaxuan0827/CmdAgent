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
	size_t nread;
	
	nread = bufferevent_read(bev, devnet->data_blob, DEVNET_RDBUFFER_SIZE);
	if( nread > 0){
		devnet->wr_off += nread;
		devnet->net_op.dealpacket(devnet->net_op.param, devnet->nserid,devnet->data_blob, nread);
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

	//SimuList_Create(&devnet->msglist);

	devnet->net_op.dealpacket = op->dealpacket;
	devnet->flags = DEVNET_FLAGS_DROPADDATA;	
	strcpy(devnet->ipaddr, ipaddr);
	devnet->usport = port;

	devnet->nserid = 0;
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

int devnet_write(struct devnet_info *devnet,int serid,void *data, int len)
{
	int retval = -1;
	if(!devnet_isconnected(devnet)){
		ERRSYS_WARNPRINT("write data, but devnet disconnect\n");
		return -1;
	}

	devnet->nserid = serid;
	struct evbuffer *output = bufferevent_get_output(devnet->bev);
	retval = evbuffer_add(output, data, len);
	if (retval == -1) {
		ERRSYS_ERRPRINT("fail to send data length %d, serid:%d \n", len, serid);
	}
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

