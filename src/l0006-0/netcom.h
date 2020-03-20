#ifndef __L0006_0_NETCOM_H__
#define __L0006_0_NETCOM_H__

#define L0006_NETCOM_TIMEOUT_PERIOD_US			3000000UL
#define L0006_NETCOM_TIMEOUT_PERIOD_MS			(L0006_NETCOM_TIMEOUT_PERIOD_US / 1000)

#define L0006_NETCOM_WRBUFFER_DEFAULT				(128)		//each write buffer size
#define L0006_NETCOM_WRBUFFER_MAX					16			//total write buffer count

#define L0006_NETCOM_FLAGS_INIT					0
#define L0006_NETCOM_FLAGS_CONNECTED				1
#define L0006_NETCOM_FLAGS_RX						2

#define L0006_IS_NETCOM_RX(NETCOM)			((NETCOM)->flags & L0006_NETCOM_FLAGS_RX)

#define L0006_NETCOM_NOACK					0
#define L0006_NETCOM_WAITACK					1

#define L0006_IS_NETCOM_CONNECTED(NC)			((NC)->flags & L0006_NETCOM_FLAGS_CONNECTED)
#define L0006_IS_NETCOM_RDEMPTY(NC)			((NC)->rd1_off == (NC)->rd2_off)
#define L0006_IS_NETCOM_RDNOTEMPTY(NC)			((NC)->rd1_off != (NC)->rd2_off)

struct l0006_netcom_rdbuf {
	//circ_buf, blob size is COMM_ADDATA_SIZE
	uint8_t *blob;
	uint32_t rd1_off;//comm use this to indicate loadcell ADDATA boundary
	uint32_t rd2_off;//loadcell starts here to parse addata
	uint32_t wr_off;//next empty byte for write data
	uint32_t rd1_end;//end of rd1 pointer, rewind to 0

	//this let us know ticks to walltime, protected by list_wlock
	uint64_t ticks;
	struct timeval tv;
	uint32_t time_valid;
	//free lock access
	uint64_t ticks_copy;
	struct timeval tv_copy;
	int errorflag; // 1, occur error; 0, on error;
};

#define L0006_NCPARM_MAX							8

struct l0006_netcom_action {
	void *in[L0006_NCPARM_MAX];			//action private input list
	void *out[L0006_NCPARM_MAX];			//action private output list
	int (*actioncb)(void**, void**, void*, int);//in, out, ackmsg, timedout
	uint8_t seqno;
//	uint8_t cmd;
};

struct l0006_netcom_wrbuf{
	//blob/len/max_len useful in read
	uint8_t *blob;
	uint32_t off;	//offset of blob, in case of garbage info.
	uint32_t len;	//current length of blob
	uint32_t max_len;
	struct timespec ts;//use to just rx/tx timeout
	struct l0006_netcom_action action;
	struct list_head node;
	uint32_t flags;	//comm tx use this to note need ack
};

struct l0006_netcom_operation {
	//callback functions
	int (*timeout)(void*);	//void *l0006_netcom_info
	int (*verifycmdpacket)(void*, uint8_t*, uint32_t, uint32_t*);//void *l0006_netcom_info, uint8_t *packet, uint32_t len, uint32_t *seqno
	int (*verifystream)(void*, uint8_t*, uint32_t);	//void *l0006_netcom_info, uint8_t *packet, uint32_t len
	int (*isvalidpacketheader)(void*, uint8_t*, uint32_t);//void *l0006_netcom_info, uint8_t *packet, uint32_t len
	int (*getpacketheaderlength)(void*);
	int (*readpacket)(void*, uint8_t*, uint32_t);
};

struct l0006_netcom_stats {
	uint32_t rx_count;			//stream rx count
};

struct l0006_netcom_info {
	uint8_t seqno;
	struct event_base *base;
	struct bufferevent *bev;
	struct event *evtimeout;
	struct timeval timeout;

	struct list_head wrbuf_unused_list; 		//ununsed wrbuffer(struct comm_buf)
	struct list_head wrbuf_used_list;			//unsed wrbuffer(struct comm_buf)

	//to ease resource release
	struct l0006_netcom_rdbuf ncrdbuf;
	void *wrbuf_blob;
	void *wrbuf_ncbuf;

	pthread_rwlockattr_t rwlock_attr;
	pthread_rwlock_t rwlock;

	void *parent;
	uint32_t flags;
	struct l0006_netcom_stats stats;
	struct l0006_netcom_operation *ncop;
};

void l0006_netcom_release(struct l0006_netcom_info *nc);
int l0006_netcom_initialize(struct l0006_netcom_info *nc, void* parent, struct l0006_netcom_operation *ncop, uint32_t timeout_ms);
int l0006_netcom_connect(struct l0006_netcom_info *nc, const char *url, int port);
void l0006_netcom_disconnect(struct l0006_netcom_info *nc);
int l0006_netcom_write(struct l0006_netcom_info *nc, void *data, int len, struct l0006_netcom_action *ncaction, int ack);
void l0006_netcom_loop(struct l0006_netcom_info *nc);
void l0006_netcom_breakloop(struct l0006_netcom_info *nc);
uint8_t l0006_netcom_getseqno(struct l0006_netcom_info *nc);
void l0006_netcom_stop(struct l0006_netcom_info *nc);
void l0006_netcom_start(struct l0006_netcom_info *nc);
void l0006_netcom_set_timeoutcb(struct l0006_netcom_info *nc, int (*timeout)(void*));
struct event_base* l0006_netcom_getbase(struct l0006_netcom_info *nc);
struct l0006_netcom_rdbuf *l0006_netcom_getbuf(struct l0006_netcom_info *nc);
void l0006_netcom_dump(uint8_t *proto, uint32_t size, int dir);

#endif
