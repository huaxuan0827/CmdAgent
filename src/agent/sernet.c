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

#define MODNAME	 "[A15SN]"

//////////////////////////////////////////////////
static void _sernet_acceptcb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *arg);
static void _sernet_eventcb(struct bufferevent* bev, short what, void* arg);
static void _sernet_readcb(struct bufferevent* bev,void* arg);

//////////////////////////////////////////////////

void _sernet_acceptcb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *arg)
{
	struct sernet_info *sernet = (struct sernet_info *)arg;

	int enable = 1;
	if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&enable, sizeof(enable)) < 0){
		return;
	}
	evthread_make_base_notifiable(sernet->evbase);
	
	struct bufferevent* newbuffer = bufferevent_socket_new(sernet->evbase, fd, 
		/*BEV_OPT_CLOSE_ON_FREE*/BEV_OPT_THREADSAFE | BEV_OPT_UNLOCK_CALLBACKS|BEV_OPT_DEFER_CALLBACKS);
	if( newbuffer == NULL){
		 return;
	}

	bufferevent_setcb(newbuffer,_sernet_readcb, NULL,_sernet_eventcb, sernet);
	bufferevent_enable(newbuffer,EV_READ|EV_WRITE);

	struct serclt_info *serclt = zmalloc(sizeof(serclt_info));
	if( serclt == NULL){
		return;
	}
	serclt->nsocket = fd;
	serclt->flags = SERCLT_FLAGS_DROPADDATA;
	serclt->evbuffer = newbuffer;
	
	if( serclt_initialize(serclt, sernet, &sernet->ser_op) != 0){
		return;
	}
	LIST_WLOCK(sernet);
	SimuList_Add(&sernet->serclt_list, fd, serclt, 0);
	LIST_UNLOCK(sernet);
}

void _sernet_eventcb(struct bufferevent* bev, short what, void* arg)
{
	int socketfd = 0;
	char disconnectflag = 0;
	struct sernet_info *sernet = (struct sernet_info *)arg;

	socketfd = bufferevent_getfd(bev);
	if( what & BEV_EVENT_READING && what & BEV_EVENT_WRITING){
		disconnectflag = 1;
	}
	if( what & BEV_EVENT_EOF || what & BEV_EVENT_ERROR){
		disconnectflag = 1;
	}
	if( disconnectflag){
		SimuListNode_t *node = NULL;
		struct serclt_info *serclt = NULL;
		LIST_WLOCK(sernet);
		node = SimuList_Get(&sernet->serclt_list, socketfd);
		if( node != NULL){
			serclt = (struct serclt_info *)node->pData;
			serclt_release(serclt);
			SimuList_Del(&sernet->serclt_list, socketfd, NULL);
		}
		LIST_UNLOCK(sernet);
	}
}

void _sernet_readcb(struct bufferevent* bev,void* arg)
{
	char tmpbuff[8192];
	char *pbuff = NULL;
	size_t nlen = 0;
	struct evbuffer* input = NULL;
	int socketfd = 0;
	struct sernet_info *sernet = (struct sernet_info *)arg;

	socketfd = bufferevent_getfd(bev);
	input = bufferevent_get_input(bev);
	nlen = evbuffer_get_length(input);

	if( nlen <= 0){
		return;
	}
	if( nlen > 8192){
		pbuff = zmalloc(nlen);
	}else{
		pbuff = tmpbuff;
	}
	evbuffer_remove(input, pbuff, nlen);

	SimuListNode_t *node = NULL;
	struct serclt_info *serclt = NULL;
	
	LIST_WLOCK(sernet);
	node = SimuList_Get(&sernet->serclt_list, socketfd);
	LIST_UNLOCK(sernet);
	
	if( node != NULL){
		serclt = (struct serclt_info *)node->pData;
		serclt_recvdata(serclt, pbuff, nlen);
	}
	if( nlen > 8192){
		free(pbuff);
	}
}

int sernet_initialize(struct sernet_info *sernet, const char *netpath, void* param, struct serclt_op *op)
{
	struct sockaddr_un serun;  
    int size;  

	SimuList_Create(&sernet->serclt_list);	
    memset(&serun, 0, sizeof(serun));  
    serun.sun_family = AF_UNIX;  
    strcpy(serun.sun_path, netpath);  
    size = offsetof(struct sockaddr_un, sun_path) + strlen(serun.sun_path);  
    unlink(netpath); 
    sernet->evbase = event_base_new();  
	if(sernet->evbase == NULL){

	}
	sernet->evlistener = evconnlistener_new_bind(base, _sernet_acceptcb, base,LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE,  
                                      10, (struct sockaddr *)&serun,  size);  
	if( sernet->evlistener == NULL){

	}
	return 0;
}

void sernet_release(struct sernet_info *sernet)
{
	SimuListNode_t *pNode = NULL;
	struct serclt_info *serclt = NULL;
	
	LIST_WLOCK(sernet);
	pNode = sernet->serclt_list.pHead;
	while( pNode != NULL){
		serclt = (struct serclt_info *)pNode->pData;
		serclt_release(serclt);
		free(serclt);
		pNode = pNode->pNext;
	}
	SimuList_Destory(&sernet->serclt_list, NULL);
	LIST_UNLOCK(sernet);
	
	event_base_loopbreak(sernet->evbase);
	sleep(1);
	evconnlistener_free(sernet->evlistener);
	event_base_free(sernet->evbase);
}

int sernet_write(struct sernet_info *sernet, void *data, int len, int serid, int seqno)
{
	SimuListNode_t *node = NULL;
	struct serclt_info *serclt = NULL;
	int nret = -1;
	
	LIST_WLOCK(sernet);
	node = SimuList_Get(&sernet->serclt_list, serid);
	if( node != NULL){
		serclt = (struct serclt_info *)node->pData;
		nret = serclt_writedata(serclt, data, len);
	}
	LIST_UNLOCK(sernet);
	return nret;
}

void sernet_loop(struct sernet_info *sernet)
{
	event_base_dispatch(sernet->evbase);
}

void sernet_breakloop(struct sernet_info *sernet)
{
	event_base_loopbreak(sernet->evbase);
}

