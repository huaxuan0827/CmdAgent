#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include "l0001-0/l0001-0.h"
#include "l0002-0/l0002-0.h"
#include "l0003-0/l0003-0.h"
#include "netcom.h"
#include "netcmd.h"

#define MODNAME									"[L6][NC]"

#define NETCOM_RDBUFFER_SIZE					0x300000	// 4MB, over 5s all 16lanes 5shelves data
#define NETCOM_RDBUFFER_FORCEREWIND_SIZE		0x20000
#define NETCOM_RDBUFFER_FORCEREWIND_OFFSET		(NETCOM_RDBUFFER_SIZE - 0x20000)	//when getting over last 128KB, force rewind

static void set_tcp_no_delay(evutil_socket_t fd)
{
	int one = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

static int netcom_find_header(struct l0006_netcom_info *nc, uint32_t offset)
{
	struct l0006_netcom_rdbuf *ncrdbuf = &nc->ncrdbuf;
	struct l0006_netcom_operation *ncop = nc->ncop;
	uint32_t rd1_off = ncrdbuf->rd1_off + offset;
	int head_found = 0;
	uint8_t *blob = ncrdbuf->blob;
	uint8_t *packet;
	int packetheaderlength = ncop->getpacketheaderlength(nc);

	while(rd1_off + packetheaderlength < ncrdbuf->wr_off) {
		packet = (uint8_t*)(blob + rd1_off);
		l0006_netcom_dump(packet, packetheaderlength, 0);
		if (ncop->isvalidpacketheader(nc, packet, packetheaderlength)) {

			//ok, we find the head
			head_found = 1;
			break;
		}
		rd1_off += 1;
	}

	if (head_found) {
		printf("Head found rd1off 0x%x move to old rd1off 0x%x\n", rd1_off, ncrdbuf->rd1_off);
		memmove(blob + ncrdbuf->rd1_off, blob + rd1_off, ncrdbuf->wr_off - rd1_off);
		ncrdbuf->wr_off -= rd1_off - ncrdbuf->rd1_off;
//		dump_proto((uint8_t*)blob + addata->rd1_off, COM_PROTO_SIZE, 0);
	}
	else {
		printf("Head not found! Copy last 7 bytes to rd1off 0x%x\n", ncrdbuf->rd1_off);
		memmove(blob + ncrdbuf->rd1_off, blob + rd1_off, ncrdbuf->wr_off - rd1_off);
		ncrdbuf->wr_off -= rd1_off - ncrdbuf->rd1_off;
		return -1;
	}
	return 0;
}

void l0006_netcom_dump(uint8_t *proto, uint32_t size, int dir)
{
	int i;
	if (dir)
		printf("===> NETCOM DUMP BEG ===>\n");
	else
		printf("<=== NETCOM DUMP BEG <===\n");
		
	for (i = 0;i < size;i++){
		printf("%02x ", proto[i]);
		if ((i & 0xF) == 0xF) {
			printf("\n");
		}
	}
	printf("\n");
	if (dir)
		printf("===> NETCOM DUMP END ===>\n");
	else
		printf("<=== NETCOM DUMP END <===\n");
}

void l0006_netcom_stop(struct l0006_netcom_info *nc)
{
	LIST_WLOCK(nc);
	nc->flags &= ~L0006_NETCOM_FLAGS_RX;
	nc->ncrdbuf.rd2_off = nc->ncrdbuf.rd1_off;
	LIST_UNLOCK(nc);
}

void l0006_netcom_start(struct l0006_netcom_info *nc)
{
	LIST_WLOCK(nc);
	nc->flags |= L0006_NETCOM_FLAGS_RX;
	LIST_UNLOCK(nc);
}

uint8_t l0006_netcom_getseqno(struct l0006_netcom_info *nc)
{	
	uint8_t retval;
	LIST_WLOCK(nc);
	retval = nc->seqno ++;
	LIST_UNLOCK(nc);

	return retval;
}

void dump_proto2(uint8_t *proto, uint32_t size, char *ip, struct l0006_netcom_rdbuf *ncrdbuf)
{
	int i;
	static FILE* fp = NULL;
	static int nCount = 0;
	struct tm *tmp_ptr = NULL;
	time_t tnow;
	char time_str[256];
	
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
		sprintf(szFileName, "/tmp/log_dump/%s_datadump_%s", ip, time_str);
		printf("szFileName:%s \n", szFileName);
		fp = fopen(szFileName, "w");
		nCount = 0;
	}
	if( fp == NULL){
		return;
	}

	fprintf(fp, "[%s] size=%d ,wr:%d, rd1:%d, rd2:%d\n", time_str, size, ncrdbuf->wr_off, ncrdbuf->rd1_off, ncrdbuf->rd2_off);
	//fprintf(fp, "[%s] size=%d \n", time_str, size);
	nCount += size;
	
	for (i = 0;i < size;i++){
		fprintf(fp, "%02x ", proto[i]);
		if ((i & 0xF) == 0xF) {
			fprintf(fp,"\n");
		}
	}
	fprintf(fp, "\n");
}


static void netcom_readcb(struct bufferevent *bev, void *ctx)
{
	struct l0006_netcom_info *nc = (struct l0006_netcom_info*)ctx;
	struct l0006_netcom_rdbuf *ncrdbuf = &nc->ncrdbuf;
	struct l0006_netcom_operation *op = nc->ncop;
	ssize_t sz;
	uint32_t length, packetlength;
	uint8_t *blob = ncrdbuf->blob;
	uint32_t space, space_to_end;
	int packetheaderlength = op->getpacketheaderlength(nc);
	uint8_t *packet;
	
	if( ncrdbuf->errorflag){
		ERRSYS_FATALPRINT("################# nc rdbuf occur error, so force reset netcomm_info, wr_off:%d, rd1_off:%d, rd2_off:%d, rd1_end:%d\n", ncrdbuf->wr_off, 
			ncrdbuf->rd1_off, ncrdbuf->rd2_off, ncrdbuf->rd1_end);

		ncrdbuf->wr_off = 0;
		ncrdbuf->rd1_off = 0;
		ncrdbuf->rd2_off = 0;
		ncrdbuf->rd1_end = 0;	
		ncrdbuf->errorflag = 0;
		
		evbuffer_drain(bufferevent_get_input(nc->bev), -1);
		return;
	}
	/* 
	 * delay parse the command header. When meeting with end of blob, 
	 * delay until the blob start region is free, and copy to the blob start
	 */
	space = CIRC_SPACE(ncrdbuf->wr_off, ncrdbuf->rd2_off, NETCOM_RDBUFFER_SIZE);
	space_to_end = CIRC_SPACE_TO_END(ncrdbuf->wr_off, ncrdbuf->rd2_off, NETCOM_RDBUFFER_SIZE);

	nc->stats.rx_count ++;

	if (space == 0) {
		ERRSYS_INFOPRINT("nc rdbuf buffer is full, wroff reaches rd2off, drop packets!\n");
		ncrdbuf->wr_off = ncrdbuf->rd1_off;
		//just drop the buffer
		evbuffer_drain(bufferevent_get_input(nc->bev), -1);
		return;
	}
	
	if (ncrdbuf->wr_off > NETCOM_RDBUFFER_FORCEREWIND_OFFSET) {
		printf("step cross 0x%x rd1off 0x%x rd2off 0x%x wroff 0x%x\n", NETCOM_RDBUFFER_FORCEREWIND_OFFSET, ncrdbuf->rd1_off, ncrdbuf->rd2_off, ncrdbuf->wr_off);
		/* check if we can move to the start of the blob */
		if (space > ncrdbuf->wr_off - ncrdbuf->rd1_off + NETCOM_RDBUFFER_FORCEREWIND_SIZE) {
			/* mark the end of ncrdbuf */
			
//			printf("Move to the start of the blob\n");
			memmove(blob, blob + ncrdbuf->rd1_off, ncrdbuf->wr_off - ncrdbuf->rd1_off);
			ncrdbuf->wr_off -= ncrdbuf->rd1_off;

			printf("Mark the end of ncrdbuf rd1end 0x%x\n", ncrdbuf->rd1_end);
			space = CIRC_SPACE(ncrdbuf->wr_off, ncrdbuf->rd2_off, NETCOM_RDBUFFER_SIZE);
			space_to_end = CIRC_SPACE_TO_END(ncrdbuf->wr_off, ncrdbuf->rd2_off, NETCOM_RDBUFFER_SIZE);
			
			ncrdbuf->rd1_end = ncrdbuf->rd1_off;
			ncrdbuf->rd1_off = 0;
		}
		else {
			ERRSYS_INFOPRINT("nc rdbuf buffer is full, drop packets!\n");
			ncrdbuf->wr_off = ncrdbuf->rd1_off;
			//just drop the buffer
			evbuffer_drain(bufferevent_get_input(nc->bev), -1);
			return;
		}
	}
	
//	space = space < space_to_end ? space : space_to_end;
	sz = bufferevent_read(bev, ncrdbuf->blob + ncrdbuf->wr_off, space_to_end);
//	dump_proto(ncrdbuf->blob + ncrdbuf->wr_off, sz, 0);
	ncrdbuf->wr_off += sz;
//	if (ncrdbuf->wr_off & 0x3) {
//		printf("COMM wroff 0x%x not aligned!\n", ncrdbuf->wr_off);
//		ncrdbuf->wr_off = (ncrdbuf->wr_off + 3) & 0xFFFFFFFC;// 4 bytes aligned
//	}

	LIST_WLOCK(nc);
//	printf("=============================\n");
//	printf("NETCOM1 rd1off 0x%x rd2off 0x%x wroff 0x%x\n", ncrdbuf->rd1_off, ncrdbuf->rd2_off, ncrdbuf->wr_off);
//	l0006_netcom_dump(blob + ncrdbuf->rd1_off, ncrdbuf->wr_off - ncrdbuf->rd1_off, 0);
//	l0006_netcom_dump(blob + ncrdbuf->rd1_off, 16, 0);

//	ERRSYS_INFOPRINT("READ %d bytes\n", sz);

	while (ncrdbuf->wr_off != ncrdbuf->rd1_off) {
		length = ncrdbuf->wr_off - ncrdbuf->rd1_off;
		if (length >= packetheaderlength) {
//			printf("rd1 length %u\n", length);
			packet = (uint8_t*)(blob + ncrdbuf->rd1_off);

			/* here we got the complete proto command */
			if (op->isvalidpacketheader(nc, packet, packetheaderlength)) {
//				ERRSYS_INFOPRINT("Valid packet header!\n");
				if ((packetlength = op->verifystream(nc, packet, length)) != 0) {
//					ERRSYS_INFOPRINT("Stream packet header! packetlength %u length %u\n", packetlength, length);
					if (packetlength == 0 || packetlength > L0006_STREAM_MAX_SIZE) {
						/* bad packet */
						if (netcom_find_header(nc, packetheaderlength) < 0)
							break;
					}
					else if (packetlength <= length) {
						/* good packet */
						if (L0006_IS_NETCOM_RX(nc)) {
							//wake up loadcell
//							printf("Wakeup loadtask!!!!!!!\n");
							if (op->readpacket(nc, packet, length) == 0) {
								/* update rd1_off */
								ncrdbuf->rd1_off += packetlength;
							}
							else {
								if (netcom_find_header(nc, packetheaderlength) < 0)
									break;
							}
						}
						else {
							memmove(blob + ncrdbuf->rd1_off, blob + ncrdbuf->rd1_off + packetlength, ncrdbuf->wr_off - ncrdbuf->rd1_off - packetlength);
							ncrdbuf->wr_off -= packetlength;
						}
					}
					else {
						/* packet hasn't been complete yet! */
//						ERRSYS_INFOPRINT("Packet incompleted, assumed %d now %d\n", packetlength, length);
//						l0006_netcom_dump(packet, length, 0);
						break;
					}
				}
				else {
					uint32_t seqno;
					if ((packetlength = op->verifycmdpacket(nc, packet, length, &seqno)) != 0) {
//						ERRSYS_INFOPRINT("ack packetlength %d\n", packetlength);
						/* command acknowledges */
						if (packetlength <= length) {
							struct l0006_netcom_wrbuf *ncwrbuf, *n;
							struct l0006_netcom_action *action;
					
							//wakeup process
							list_for_each_entry_safe(ncwrbuf, n, &nc->wrbuf_used_list, node) {
								action = &ncwrbuf->action;
//								ERRSYS_INFOPRINT("Waiting list seqno %d\n", action->seqno);
								if (action->seqno == seqno) {
//									ERRSYS_INFOPRINT("netcom command acked\n");
									//todo action callback & wakeup process
									action->actioncb(action->in, action->out, packet, 0);
									list_move_tail(&ncwrbuf->node, &nc->wrbuf_unused_list);
									break;
								}
							}
						}
						else {
							/* imcompleted cmd */
							break;
						}
					}
					else {
						ERRSYS_INFOPRINT("Invalid cmd packet\n");
					}
					memmove(blob + ncrdbuf->rd1_off, blob + ncrdbuf->rd1_off + packetlength, ncrdbuf->wr_off - ncrdbuf->rd1_off - packetlength);
					ncrdbuf->wr_off -= packetlength;
				}
			}
			else {
				/* BAD CASE!!
				 * Invalid head, search the magic
				 */
				ERRSYS_INFOPRINT("Invalid head rd1off 0x%x rd2off 0x%x wroff 0x%x\n", ncrdbuf->rd1_off, ncrdbuf->rd2_off, ncrdbuf->wr_off);
				l0006_netcom_dump(packet, packetheaderlength, 0);
				if (netcom_find_header(nc, packetheaderlength) < 0)
					break;
			}
		}
		else {
//			printf("Incomplete head, be patient(%d)!\n", sz);
//			dump_proto(ncrdbuf->blob + ncrdbuf->rd1_off, ncrdbuf->wr_off - ncrdbuf->rd1_off, 0);
			break;
		}
	}
	LIST_UNLOCK(nc);
}

static void netcom_timeoutcb(evutil_socket_t fd, short what, void *arg)
{
	struct l0006_netcom_info *nc = arg;
	struct timespec ts;


	LIST_WLOCK(nc);
	if (!list_empty(&nc->wrbuf_used_list)) {
		struct l0006_netcom_wrbuf *ncwrbuf, *n;

		clock_gettime(CLOCK_MONOTONIC, &ts);

		//scan wrbuf list
		list_for_each_entry_safe(ncwrbuf, n, &nc->wrbuf_used_list, node) {
			if (l0001_timespecelapsed(&ts, &ncwrbuf->ts) > (uint64_t)(L0006_NETCOM_TIMEOUT_PERIOD_MS)) {
				struct l0006_netcom_action *action = &ncwrbuf->action;
				action->actioncb(action->in, action->out, NULL, 1);
				list_move_tail(&ncwrbuf->node, &nc->wrbuf_unused_list);
				ERRSYS_ERRPRINT("netcom command timed out\n");
			}
			else {
				break;
			}
		}
	}

	LIST_UNLOCK(nc);

	if (nc->ncop->timeout) {
		nc->ncop->timeout(nc);
	}

	return;
}

struct l0006_netcom_rdbuf *l0006_netcom_getbuf(struct l0006_netcom_info *nc)
{
	return &nc->ncrdbuf;
}
struct event_base* l0006_netcom_getbase(struct l0006_netcom_info *nc)
{
	return nc->base;
}

void l0006_netcom_set_timeoutcb(struct l0006_netcom_info *nc, int (*timeout)(void*))
{
	nc->ncop->timeout = timeout;
}

int l0006_netcom_write(struct l0006_netcom_info *nc, void *data, int len, struct l0006_netcom_action *ncaction, int ack)
{
	int retval = -1;

	LIST_WLOCK(nc);

	if (L0006_IS_NETCOM_CONNECTED(nc)) {
		if (!list_empty(&nc->wrbuf_unused_list)) {
			if (nc->bev) {
				struct l0006_netcom_wrbuf *ncwrbuf = list_entry(nc->wrbuf_unused_list.next, struct l0006_netcom_wrbuf, node);
				struct evbuffer *output = bufferevent_get_output(nc->bev);
				memcpy(&ncwrbuf->action, ncaction, sizeof(struct l0006_netcom_action));
				/* save the time */
				clock_gettime(CLOCK_MONOTONIC, &ncwrbuf->ts);
		
				/* push to the evbuffer */
				retval = evbuffer_add(output, data, len);
				//bufferevent_write(bev, http_request, strlen(http_request));
				if (retval == -1) {
					ERRSYS_ERRPRINT("Fail to send data length %d\n", len);
				}
				else {
					retval = 0;
					if (ack) {
						ncwrbuf->flags = L0006_NETCOM_WAITACK;
						list_move_tail(nc->wrbuf_unused_list.next, &nc->wrbuf_used_list);
					}
					else {
						ncwrbuf->flags = L0006_NETCOM_NOACK;
					}
				}
			}
			else {
				ERRSYS_ERRPRINT("netcom disconnected\n");			
			}
		}
		else {
			ERRSYS_ERRPRINT("Fail to send data length %d\n", len);
		}
	}

	LIST_UNLOCK(nc);

	return retval;
}


static void netcom_eventcb(struct bufferevent *bev, short events, void *ctx)
{
//	struct process_info *proc = (struct process_info*)ctx;
//	struct task_info *dsmaint_task = proc->parent;
//	struct tasks_cluster *tcluster = dsmaint_task->parent;
//	struct dsmaint_proc *dsmaint = (struct dsmaint_proc*)proc->priv;
	struct l0006_netcom_info *nc = ctx;

	LIST_WLOCK(nc);
	if (events & BEV_EVENT_CONNECTED) {
		evutil_socket_t fd = bufferevent_getfd(bev);
		set_tcp_no_delay(fd);
		nc->flags |= L0006_NETCOM_FLAGS_CONNECTED;

		//if (nc->cp.checkpoint[0]) {
		//	nc->cp.checkpoint[0](nc->cp.sectdata[0], (void*)L0005_COMMSTAT_GOOD);
		//}

		ERRSYS_ERRPRINT("netcom connected!!!!\n");
	}
	else if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
		if (events & BEV_EVENT_ERROR) {
			ERRSYS_INFOPRINT("netcom event error\n");
		}
		if (events & BEV_EVENT_EOF) {
			ERRSYS_INFOPRINT("netcom event eof\n");
		}
		nc->flags &= ~L0006_NETCOM_FLAGS_CONNECTED;

		//if (nc->cp.checkpoint[0]) {
		//	nc->cp.checkpoint[0](nc->cp.sectdata[0], (void*)L0005_COMMSTAT_FAIL);
		//}

//		event_base_loopbreak(nc->base);
	}
	else {
		ERRSYS_ERRPRINT("netcom unknown event %d\n", events);

		//if (nc->cp.checkpoint[0]) {
		//	nc->cp.checkpoint[0](nc->cp.sectdata[0], (void*)L0005_COMMSTAT_FAIL);
		//}
	}
	LIST_UNLOCK(nc);
}

void l0006_netcom_loop(struct l0006_netcom_info *nc)
{
	event_base_dispatch(nc->base);
}

void l0006_netcom_breakloop(struct l0006_netcom_info *nc)
{
	LIST_WLOCK(nc);
	event_base_loopbreak(nc->base);
	LIST_UNLOCK(nc);
}
int l0006_netcom_connect(struct l0006_netcom_info *nc, const char *url, int port)
{
	int retval = -1;

	LIST_WLOCK(nc);
	if (nc->bev) {
		bufferevent_free(nc->bev);
		nc->bev = NULL;
	}
	
	nc->bev = bufferevent_socket_new(nc->base, -1, BEV_OPT_CLOSE_ON_FREE);
	if (nc->bev == NULL) {
		goto err1;
	}
	bufferevent_setcb(nc->bev, netcom_readcb, NULL, netcom_eventcb, nc);
	bufferevent_enable(nc->bev, EV_READ | EV_WRITE | EV_PERSIST);

	ERRSYS_INFOPRINT("netcom connect %s:%d\n", url, port);
	if (bufferevent_socket_connect_hostname(nc->bev, NULL, AF_INET, url, port) < 0) {
		ERRSYS_ERRPRINT("Fail to connect %s:%d\n", url, port);
		bufferevent_free(nc->bev);
		goto err2;
	}
	LIST_UNLOCK(nc);

	return 0;
err2:
	bufferevent_free(nc->bev);
	nc->bev = NULL;
err1:
	LIST_UNLOCK(nc);
	return retval;
}

void l0006_netcom_disconnect(struct l0006_netcom_info *nc)
{
	if (nc->base) {
		event_base_loopbreak(nc->base);
	}
	
	if (nc->bev) {
		bufferevent_free(nc->bev);
		nc->bev = NULL;
	}
}

static int netcom_allocate_buffer(struct l0006_netcom_info *nc)
{
	int i;
	uint8_t *blob;
	struct l0006_netcom_wrbuf *ncbuf;

	/* write nc buffer */
	nc->wrbuf_ncbuf = ncbuf = zmalloc(sizeof(struct l0006_netcom_wrbuf) * L0006_NETCOM_WRBUFFER_MAX);
	if (ncbuf == NULL) {
		ERRSYS_FATALPRINT("Fail to allocate %d struct l0006_netcom_wrbuf\n", L0006_NETCOM_WRBUFFER_MAX);
		goto err1;
	}

	nc->wrbuf_blob = blob = zmalloc(L0006_NETCOM_WRBUFFER_MAX * L0006_NETCOM_WRBUFFER_DEFAULT);
	if (blob == NULL) {
		free(ncbuf);
		goto err2;
	}

	INIT_LIST_HEAD(&nc->wrbuf_unused_list);
	INIT_LIST_HEAD(&nc->wrbuf_used_list);

	for (i = 0; i < L0006_NETCOM_WRBUFFER_MAX; i ++, ncbuf ++, blob += L0006_NETCOM_WRBUFFER_DEFAULT) {
		ncbuf->blob = blob;
		ncbuf->len = 0;
		ncbuf->max_len = L0006_NETCOM_WRBUFFER_DEFAULT;
		INIT_LIST_HEAD(&ncbuf->node);
		list_add(&ncbuf->node, &nc->wrbuf_unused_list);
	}

	memset(&nc->ncrdbuf, 0, sizeof(struct l0006_netcom_rdbuf));
	nc->ncrdbuf.blob = zmalloc(NETCOM_RDBUFFER_SIZE);
	if (nc->ncrdbuf.blob == NULL) {
		ERRSYS_FATALPRINT("Fail to allocate ncrdbuf blob\n");
		goto err3;
	}

	nc->flags = L0006_NETCOM_FLAGS_INIT;

	return 0;
err3:
	free(nc->wrbuf_blob);
err2:
	free(nc->wrbuf_ncbuf);
err1:
	return -1;
}

void l0006_netcom_release(struct l0006_netcom_info *nc)
{
	if (nc->base) {
		event_base_loopbreak(nc->base);
		event_base_free(nc->base);
		nc->base = NULL;
	}

	if (nc->evtimeout) {
		evtimer_del(nc->evtimeout);
		event_free(nc->evtimeout);
		nc->evtimeout = NULL;
	}

	free(nc->ncrdbuf.blob);
	free(nc->wrbuf_blob);
	free(nc->wrbuf_ncbuf);
	pthread_rwlockattr_destroy(&nc->rwlock_attr);
	pthread_rwlock_destroy(&nc->rwlock);
}

int l0006_netcom_initialize(struct l0006_netcom_info *nc, void* parent, struct l0006_netcom_operation *ncop, uint32_t timeout_us)
{
	int retval;
	nc->parent = parent;
	if ((retval = pthread_rwlockattr_init(&(nc->rwlock_attr))) < 0) {
		ERRSYS_FATALPRINT("Fail to initialize rwlock attribute for netcom: %s\n", strerror(errno));
		goto err1;
	}
	if ((retval = pthread_rwlock_init(&(nc->rwlock),&(nc->rwlock_attr))) < 0) {
		ERRSYS_FATALPRINT("Fail to initialize rwlock for netcom: %s\n", strerror(errno));
		goto err2;
	}

	if (netcom_allocate_buffer(nc) < 0) {
		ERRSYS_FATALPRINT("Fail to allocate netcom buffer\n");
		goto err3;
	}

	nc->base = event_base_new();
	if (nc->base == NULL) {
		ERRSYS_FATALPRINT("Fail to new event base!\n");
		TAKEABREATH();
		goto err4;
	}
	nc->evtimeout = evtimer_new(nc->base, netcom_timeoutcb, nc);
	if (nc->evtimeout == NULL) {
		goto err5;
	}
	event_assign(nc->evtimeout, nc->base, -1, EV_PERSIST, netcom_timeoutcb, (void*)nc);
	
	evutil_timerclear(&nc->timeout);
	nc->timeout.tv_sec = timeout_us / 1000000;
	nc->timeout.tv_usec = timeout_us % 1000000;
	event_add(nc->evtimeout, &nc->timeout);

	if (ncop != NULL) {
		nc->ncop = ncop;
	}
	else {
		goto err6;
	}

	return 0;
err6:	
	evtimer_del(nc->evtimeout);
	event_free(nc->evtimeout);
err5:
	event_base_free(nc->base);
	nc->base = NULL;
	
err4:
	free(nc->ncrdbuf.blob);
	free(nc->wrbuf_blob);
	free(nc->wrbuf_ncbuf);
err3:	
	pthread_rwlock_destroy(&nc->rwlock);
err2:
	pthread_rwlockattr_destroy(&nc->rwlock_attr);
err1:
	return -1;
}

