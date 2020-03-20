#include <stdint.h>
#include <sys/time.h>

#include "l0001-0/l0001-0.h"
#include "l0002-0/l0002-0.h"
#include "l0003-0/l0003-0.h"
#include "netcom.h"
#include "netcmd.h"

#define MODNAME		"[L6][CMD]"

#define NETCMD_UNKNOWN_STR					"UNKOWN CMD"
#define NETCMD_BUZZ_STR						"BUZZ"
#define NETCMD_BLINK_STR					"BLINK"
#define NETCMD_WINSIZE_STR					"RESIZE WIN"
#define NETCMD_RESET_STR					"RESET"
#define NETCMD_ADDATA_STR					"ADDATA"
#define NETCMD_VERSION_STR					"VERSION"
#define NETCMD_DEBUGMODE_STR				"DEBUGMODE"
#define NETCMD_DEVSTAT_STR					"DEVSTAT"
#define NETCMD_RECOVER_STR					"RECOVER"
#define NETCMD_SAVEENV_STR					"SAVEENV"

#define NETCMD_BUZZACK_STR					"BUZZ ACK"
#define NETCMD_BLINKACK_STR					"BLINK ACK"
#define NETCMD_WINSIZEACK_STR				"RESIZE WIN ACK"
#define NETCMD_RESETACK_STR					"RESET ACK"
#define NETCMD_VERSIONACK_STR					"VERSION ACK"
#define NETCMD_DEBUGMODEACK_STR				"DEBUGMODE ACK"
#define NETCMD_DEVSTATACK_STR				"DEVSTAT ACK"
#define NETCMD_RECOVERACK_STR				"RECOVER ACK"
#define NETCMD_SAVEENVACK_STR					"SAVEENV"

int l0006_netcmd_getackcmd(int cmd)
{
	int ack = -1;
	switch (cmd) {
		case L0006_NETCMD_BUZZ:
			ack = L0006_NETCMD_BUZZACK;
			break;
		case L0006_NETCMD_BLINK:
			ack = L0006_NETCMD_BLINKACK;
			break;
		case L0006_NETCMD_WINSIZE:
			ack = L0006_NETCMD_WINSIZEACK;
			break;
		case L0006_NETCMD_RESET:
			ack = L0006_NETCMD_RESETACK;
			break;
		case L0006_NETCMD_VERSION:
			ack = L0006_NETCMD_VERSIONACK;
			break;
		case L0006_NETCMD_DEBUGMODE:
			ack = L0006_NETCMD_DEBUGMODEACK;
			break;
		case L0006_NETCMD_DEVSTAT:
			ack = L0006_NETCMD_DEVSTATACK;
			break;
		case L0006_NETCMD_RECOVER:
			ack = L0006_NETCMD_RECOVERACK;
			break;
		case L0006_NETCMD_SAVECONF:
			ack = L0006_NETCMD_SAVECONFACK;
			break;
		default:
			break;
	}

	return ack;
}

const char* l0006_netcmd_getcmdstring(int cmd)
{
	const char* str = NETCMD_UNKNOWN_STR;

	switch (cmd) {
		case L0006_NETCMD_BUZZ:
			str = NETCMD_BUZZ_STR;
			break;
		case L0006_NETCMD_BLINK:
			str = NETCMD_BLINK_STR;
			break;
		case L0006_NETCMD_WINSIZE:
			str = NETCMD_WINSIZE_STR;
			break;
		case L0006_NETCMD_RESET:
			str = NETCMD_RESET_STR;
			break;
		case L0006_NETCMD_ADDATA:
			str = NETCMD_ADDATA_STR;
			break;
		case L0006_NETCMD_VERSION:
			str = NETCMD_VERSION_STR;
			break;
		case L0006_NETCMD_DEBUGMODE:
			str = NETCMD_DEBUGMODE_STR;
			break;
		case L0006_NETCMD_DEVSTAT:
			str = NETCMD_DEVSTAT_STR;
			break;
		case L0006_NETCMD_RECOVER:
			str = NETCMD_RECOVER_STR;
			break;
		case L0006_NETCMD_SAVECONF:
			str = NETCMD_SAVEENV_STR;
			break;

		case L0006_NETCMD_BUZZACK:
			str = NETCMD_BUZZACK_STR;
			break;
		case L0006_NETCMD_BLINKACK:
			str = NETCMD_BLINKACK_STR;
			break;
		case L0006_NETCMD_WINSIZEACK:
			str = NETCMD_WINSIZEACK_STR;
			break;
		case L0006_NETCMD_RESETACK:
			str = NETCMD_RESETACK_STR;
			break;
		case L0006_NETCMD_VERSIONACK:
			str = NETCMD_VERSIONACK_STR;
			break;
		case L0006_NETCMD_DEBUGMODEACK:
			str = NETCMD_DEBUGMODEACK_STR;
			break;
		case L0006_NETCMD_DEVSTATACK:
			str = NETCMD_DEVSTATACK_STR;
			break;
		case L0006_NETCMD_RECOVERACK:
			str = NETCMD_RECOVERACK_STR;
			break;
		case L0006_NETCMD_SAVECONFACK:
			str = NETCMD_SAVEENVACK_STR;
			break;

		default:
			break;
	}

	return str;
}

int l0006_netcmd_write(struct l0006_netcom_info *nc, void *data, int len, struct l0006_netcom_action *action, int ack)
{
	int retval = -1;

	if (len > L0006_NETCOM_WRBUFFER_DEFAULT || len < L0006_NETCMD_HEADER_SIZE) {
		ERRSYS_ERRPRINT("Invalid netcmd command size(%d)\n", len);
		return -1;
	}
	if (l0006_netcom_write(nc, data, len, action, ack) < 0) {
		ERRSYS_ERRPRINT("Fail to write command length %d\n", len);
	}
	else {
		retval = 0;
	}

	return retval;
}

int l0006_netcmd_ex_general_action(void **p_in, void **p_out, void *p, int timedout)
{
	struct task_info *wait = (struct task_info*)p_in[0];
	int cmd = (L0005_PTRTYPE)p_in[2];
	int *outret = (int*)p_out[0];
	uint8_t *buf = (uint8_t*)p_out[1];
	uint8_t sz = (L0005_PTRTYPE)p_out[2];
	uint8_t flag, length;

//	uint8_t func[2];
	struct l0006_netcmd_ex_packet *pkt = (struct l0006_netcmd_ex_packet*)p;	//ack packet text

	if (!timedout) {
		printf("%s: cmd %x ack cmd %x\n", __func__, cmd, pkt->cmd);
		if ((int)(pkt->cmd) == l0006_netcmd_getackcmd(cmd)) {
			if (pkt->cmd == L0006_NETCMD_SAVECONFACK) {
				flag = pkt->c[0];
				length = pkt->c[1];

//				ERRSYS_INFOPRINT("Flag 0x%x length %d sz %d\n", (int)flag, (int)length, (int)sz);
				
				if (flag == L0006_NETCMD_SAVECONF_FLAG_SUCCESS) {
					if (sz == 0) {
						*outret = L0006_NETCOM_ACK_SUCCESS;
					}
					else if (length < sz) {
						memcpy(buf, &pkt->c[2], length);
						*outret = L0006_NETCOM_ACK_SUCCESS;
					}
					else {
						*outret = L0006_NETCOM_ACK_FAIL;
					}
				}
				else {
					*outret = L0006_NETCOM_ACK_FAIL;
				}
			}
			else {
				*outret = L0006_NETCOM_ACK_SUCCESS;
			}
			
			wait->wakeup(wait);

			return 0;
		}
		else {
			ERRSYS_ERRPRINT("Waiting for %s response failed, got ack cmd %s\n", l0006_netcmd_getcmdstring(cmd), l0006_netcmd_getcmdstring(pkt->cmd));
			*outret = L0006_NETCOM_ACK_FAIL;
		}
	}
	else {
		ERRSYS_ERRPRINT("%s(%d) timed out!\n", l0006_netcmd_getcmdstring(cmd), cmd);
		*outret = L0006_NETCOM_ACK_TIMEOUT;
		wait->wakeup(wait);
	}
	printf("#### NETCMD ACTION RET(%d) ###\n", *outret);

	return -1;
}

int l0006_netcmd_general_command_ex(struct l0006_netcom_info *nc, struct task_info *wait_task, struct l0006_netcmd_ex_packet *netcmd, int *outret, uint8_t *config, uint8_t sz)
{
	int retval = -1;
	struct l0006_netcom_action action;

	memset(&action, 0, sizeof(struct l0006_netcom_action));
	action.in[0] = (void*)wait_task;
	action.in[1] = netcmd;
	action.in[2] = (void*)((L0005_PTRTYPE)netcmd->cmd);
	action.in[3] = (void*)((L0005_PTRTYPE)netcmd->f.func[0]);
	action.in[4] = (void*)((L0005_PTRTYPE)netcmd->f.func[1]);
	action.out[0] = (void*)(L0005_PTRTYPE)outret;
	action.out[1] = (void*)(L0005_PTRTYPE)config;
	action.out[2] = (void*)(L0005_PTRTYPE)sz;
	
	action.actioncb = l0006_netcmd_ex_general_action;
	action.seqno = netcmd->seqno;
//	ERRSYS_INFOPRINT("%s seqno %d\n", __func__, netcmd->seqno);

	wait_task->wakeupclear(wait_task);
	if (l0006_netcmd_write(nc, (void*)netcmd, sizeof(struct l0006_netcmd_ex_packet), &action, 1) < 0) {
		ERRSYS_FATALPRINT("Fail to send netcom comand\n");
		goto err1;
	}

	retval = 0;
err1:
	return retval;
}

int l0006_netcmd_general_action(void **p_in, void **p_out, void *p, int timedout)
{
	struct task_info *wait = (struct task_info*)p_in[0];
//	struct l0006_netcmd_packet *netcmd = p_in[1];
	int cmd = (L0005_PTRTYPE)p_in[2];
	int *outret = (int*)p_out[0];
//	uint8_t func[2];
	struct l0006_netcmd_packet *pkt = (struct l0006_netcmd_packet*)p;	//ack packet text
#if 1
//	func[0] = (uint8_t)((int)p_in[2]);
//	func[1] = (uint8_t)((int)p_in[3]);
	printf("%s\n", __func__);

	if (!timedout) {
		printf("%s: cmd %x ack cmd %x\n", __func__, cmd, pkt->cmd);
		if ((int)(pkt->cmd) == l0006_netcmd_getackcmd(cmd)) {
			*outret = L0006_NETCOM_ACK_SUCCESS;
			wait->wakeup(wait);

			return 0;
		}
		else {
			ERRSYS_ERRPRINT("Waiting for %s response failed, got ack cmd %s\n", l0006_netcmd_getcmdstring(cmd), l0006_netcmd_getcmdstring(pkt->cmd));
			*outret = L0006_NETCOM_ACK_FAIL;
		}
	}
	else {
		ERRSYS_ERRPRINT("%s(%d) timed out!\n", l0006_netcmd_getcmdstring(cmd), cmd);
		*outret = L0006_NETCOM_ACK_TIMEOUT;
		wait->wakeup(wait);
	}
	printf("#### NETCMD ACTION RET(%d) ###\n", *outret);

	return -1;
#else
	ERRSYS_INFOPRINT("FAST ACK##########\n");
	*outret = L0006_NETCOM_ACK_SUCCESS;
	wait->wakeup(wait);
	return 0;

#endif
}

int l0006_netcmd_general_command(struct l0006_netcom_info *nc, struct task_info *wait_task, struct l0006_netcmd_packet *netcmd, int *outret)
{
	int retval = -1;
	struct l0006_netcom_action action;

	memset(&action, 0, sizeof(struct l0006_netcom_action));
	action.in[0] = (void*)wait_task;
	action.in[1] = netcmd;
	action.in[2] = (void*)((L0005_PTRTYPE)netcmd->cmd);
	action.in[3] = (void*)((L0005_PTRTYPE)netcmd->f.func[0]);
	action.in[4] = (void*)((L0005_PTRTYPE)netcmd->f.func[1]);
	action.out[0] = (void*)(L0005_PTRTYPE)outret;
	action.actioncb = l0006_netcmd_general_action;
	action.seqno = netcmd->seqno;

	wait_task->wakeupclear(wait_task);
	if (l0006_netcmd_write(nc, (void*)netcmd, L0006_NETCMD_HEADER_SIZE, &action, 1) < 0) {
		ERRSYS_FATALPRINT("Fail to send netcom comand\n");
		goto err1;
	}

	retval = 0;
err1:
	return retval;
}

int l0006_netcmd_general_action2(void **p_in, void **p_out, void *p, int timedout)
{
	struct task_info *wait = (struct task_info*)p_in[0];
	int cmd = (L0005_PTRTYPE)p_in[2];
	int *outret = (int*)p_out[0];
	uint8_t **pp_packet = (uint8_t**)p_out[1];
	struct l0006_netcmd_packet *pkt = (struct l0006_netcmd_packet*)p;	//ack packet text
	printf("%s\n", __func__);

	if (!timedout) {
		printf("%s: cmd %x ack cmd %x\n", __func__, cmd, pkt->cmd);
		if ((int)(pkt->cmd) == l0006_netcmd_getackcmd(cmd)) {
			if (pp_packet) {
				*pp_packet = zmalloc(sizeof(struct l0006_netcmd_packet));
				if (*pp_packet) {
					memcpy(*pp_packet, p, sizeof(struct l0006_netcmd_packet));
				}
			}
			*outret = L0006_NETCOM_ACK_SUCCESS;
			wait->wakeup(wait);

			return 0;
		}
		else {
			ERRSYS_ERRPRINT("Waiting for %s response failed, got ack cmd %s\n", l0006_netcmd_getcmdstring(cmd), l0006_netcmd_getcmdstring(pkt->cmd));
			*outret = L0006_NETCOM_ACK_FAIL;
		}
	}
	else {
		ERRSYS_ERRPRINT("%s(%d) timed out!\n", l0006_netcmd_getcmdstring(cmd), cmd);
		*outret = L0006_NETCOM_ACK_TIMEOUT;
		wait->wakeup(wait);
	}
	printf("#### NETCMD ACTION RET(%d) ###\n", *outret);

	return -1;

}

int l0006_netcmd_general_command2(struct l0006_netcom_info *nc, struct task_info *wait_task, struct l0006_netcmd_packet *netcmd, int *outret, uint8_t** pp_packet)
{
	int retval = -1;
	struct l0006_netcom_action action;

	memset(&action, 0, sizeof(struct l0006_netcom_action));
	action.in[0] = (void*)wait_task;
	action.in[1] = netcmd;
	action.in[2] = (void*)((L0005_PTRTYPE)netcmd->cmd);
	action.in[3] = (void*)((L0005_PTRTYPE)netcmd->f.func[0]);
	action.in[4] = (void*)((L0005_PTRTYPE)netcmd->f.func[1]);
	action.out[0] = (void*)(L0005_PTRTYPE)outret;
	action.out[1] = (void*)pp_packet;
	action.actioncb = l0006_netcmd_general_action2;
	action.seqno = netcmd->seqno;

	wait_task->wakeupclear(wait_task);
	if (l0006_netcmd_write(nc, (void*)netcmd, L0006_NETCMD_HEADER_SIZE, &action, 1) < 0) {
		ERRSYS_FATALPRINT("Fail to send netcom comand\n");
		goto err1;
	}

	retval = 0;
err1:
	return retval;
}


/* call by netcom.timeout */
static int netcmd_timeoutcb(void *p)
{
	return 0;
}

static int netcmd_verifycmdpacket(void *p, uint8_t *packet, uint32_t len, uint32_t *seqno)
{
//	struct l0006_netcom_info *nc = p;
	struct l0006_netcmd_packet *netcmd = (struct l0006_netcmd_packet*)packet;

	if (netcmd->cmd != L0006_NETCMD_ADDATA && ((netcmd->cmd >= L0006_NETCMD_BUZZ && netcmd->cmd <= L0006_NETCMD_SAVECONF)
			|| (netcmd->cmd >= L0006_NETCMD_BUZZACK && netcmd->cmd <= L0006_NETCMD_SAVECONFACK))) {
		*seqno = netcmd->seqno;
		if (netcmd->cmd == L0006_NETCMD_SAVECONF || netcmd->cmd == L0006_NETCMD_SAVECONFACK) {
//			printf("cmdpacket size %d\n", L0006_NETCMD_EX_HEADER_SIZE);
			return L0006_NETCMD_EX_HEADER_SIZE;
		}
		
		return L0006_NETCMD_HEADER_SIZE;
	}

	return 0;
}

static int netcmd_verifystream(void *p, uint8_t *packet, uint32_t len)
{
	struct l0006_netcmd_packet *netcmd = (struct l0006_netcmd_packet*)packet;
	if (netcmd->cmd == L0006_NETCMD_ADDATA)
		return netcmd->f.length;
	return 0;
}

static int netcmd_isvalidpacketheader(void *p, uint8_t *packet, uint32_t len)
{
	struct l0006_netcmd_packet netcmd;
	memcpy(&netcmd, packet, sizeof(struct l0006_netcmd_packet));
	return (netcmd.magic == L0006_NETCMD_PACKET_MAGIC 
		&& ((netcmd.cmd >= L0006_NETCMD_BUZZ && netcmd.cmd <= L0006_NETCMD_SAVECONF)
			|| (netcmd.cmd >= L0006_NETCMD_BUZZACK && netcmd.cmd <= L0006_NETCMD_SAVECONFACK))
		&& netcmd.crc16 == crc16(0, (const unsigned char*)(&netcmd), L0006_NETCMD_CHECKSUM_SIZE));
}

static int netcmd_getpacketheaderlength(void *p)
{
	return L0006_NETCMD_HEADER_SIZE;
}

static int netcmd_readpacket(void *p, uint8_t *packet, uint32_t len)
{
	uint16_t ck, ck_packet;
	struct l0006_netcmd_packet *pkt = (struct l0006_netcmd_packet*)packet;
	
	ck = crc16(0, packet + L0006_NETCMD_HEADER_SIZE, pkt->f.length - L0006_NETCMD_HEADER_SIZE - L0006_STREAM_CRC16_SIZE);
	ck_packet = *(uint16_t*)(packet + pkt->f.length - L0006_STREAM_CRC16_SIZE);
	if (ck == ck_packet) {
		struct timeval tv;
		struct l0006_netcmd_stream_timestamp *ts = (struct l0006_netcmd_stream_timestamp*)(packet + L0006_NETCMD_HEADER_SIZE);

		gettimeofday(&tv, NULL);
		ts->tv_sec = tv.tv_sec;
		ts->tv_usec = tv.tv_usec;
		
		return 0;
	}

	return -1;
}

static struct l0006_netcom_operation netcmdops = {
	.timeout = netcmd_timeoutcb,
	.verifycmdpacket = netcmd_verifycmdpacket,
	.verifystream = netcmd_verifystream,
	.isvalidpacketheader = netcmd_isvalidpacketheader,
	.getpacketheaderlength = netcmd_getpacketheaderlength,
	.readpacket = netcmd_readpacket,
};

struct l0006_netcom_info* l0006_netcmd_initialize(void* parent)
{
	struct l0006_netcom_info *nc = NULL;
	if ((nc = (struct l0006_netcom_info*)zmalloc(sizeof(struct l0006_netcom_info))) == NULL) {
		ERRSYS_FATALPRINT("Fail to allocate netcom \n");
		goto err1;
	}
	
	if (l0006_netcom_initialize(nc, parent, &netcmdops, L0006_NETCOM_TIMEOUT_PERIOD_US) < 0) {
		ERRSYS_FATALPRINT("Fail to initialize netcom for DSCMD\n");
		goto err2;
	}

	return nc;
err2:
	free(nc);
err1:
	return NULL;
}

void l0006_netcmd_release(struct l0006_netcom_info *nc)
{
	l0006_netcom_release(nc);
	free(nc);
}
