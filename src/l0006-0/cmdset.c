#include "l0001-0/l0001-0.h"
#include "l0002-0/l0002-0.h"
#include "l0003-0/l0003-0.h"
#include "netcom.h"
#include "netcmd.h"

#define MODNAME			"[L6][SET]"

int l0006_cmdset_save_sensorconfig(struct l0006_netcom_info *nc, struct task_info *wait_task, int chnl_index, int node_index, void *config, uint8_t sz)
{
	struct l0006_netcmd_ex_packet netcmd;
	uint8_t *body = netcmd.c;
	struct timespec ts_start, ts_now;
	uint32_t timeelapsed;
	int retval = 0;

	ERRSYS_INFOPRINT("#### Saving CH%dND%d config ####\n", chnl_index, node_index);
	L0006_NETCMD_FORMAT(&netcmd, L0006_NETCMD_SAVECONF, l0006_netcom_getseqno(nc), chnl_index, node_index);
	clock_gettime(CLOCK_MONOTONIC, &ts_start);
	body[0] = L0006_NETCMD_SAVECONF_FLAG_WRITE;
	body[1] = sz;
	memcpy(body + 2, config, sz);

	if (l0006_netcmd_general_command_ex(nc, wait_task, &netcmd, &retval, NULL, 0) < 0) {
		ERRSYS_ERRPRINT("Fail to send command ex!\n");
		return -1;
	}

	TASK_WAIT_COND(wait_task, (!IS_TASK_ABORT(wait_task) 
		&& (clock_gettime(CLOCK_MONOTONIC, &ts_now), (timeelapsed = l0001_second_elapsed(&ts_now, &ts_start)) <= L0006_NETCMD_COMMAND_TIMEOUT)));

	if (retval != L0006_NETCOM_ACK_SUCCESS) {
		ERRSYS_ERRPRINT("Saving CH%dND%d config failed: %d\n", chnl_index, node_index, retval);
		return -1;
	}

	ERRSYS_INFOPRINT("CH%dND%d config saved\n", chnl_index, node_index);

	return 0;
}

int l0006_cmdset_load_sensorconfig(struct l0006_netcom_info *nc, struct task_info *wait_task, int chnl_index, int node_index, void **config, uint8_t *rdsz)
{
	struct l0006_netcmd_ex_packet netcmd;
	uint8_t *body = netcmd.c;
	struct timespec ts_start, ts_now;
	uint32_t timeelapsed;
	int retval = 0;
	
	if (config && rdsz) {
		*config = zmalloc(64);
		if (*config) {
			ERRSYS_INFOPRINT("#### Loading CH%dND%d config ####\n", chnl_index, node_index);
			L0006_NETCMD_FORMAT(&netcmd, L0006_NETCMD_SAVECONF, l0006_netcom_getseqno(nc), chnl_index, node_index);
			clock_gettime(CLOCK_MONOTONIC, &ts_start);
			body[0] = L0006_NETCMD_SAVECONF_FLAG_READ;
			body[1] = *rdsz;

			if (l0006_netcmd_general_command_ex(nc, wait_task, &netcmd, &retval, *config, 64) < 0) {
				ERRSYS_ERRPRINT("Fail to send command ex!\n");
				return -1;
			}
			
			TASK_WAIT_COND(wait_task, (!IS_TASK_ABORT(wait_task) 
				&& (clock_gettime(CLOCK_MONOTONIC, &ts_now), (timeelapsed = l0001_second_elapsed(&ts_now, &ts_start)) <= L0006_NETCMD_COMMAND_TIMEOUT)));
			
			if (retval != L0006_NETCOM_ACK_SUCCESS) {
				ERRSYS_ERRPRINT("Reading CH%dND%d config failed: %d\n", chnl_index, node_index, retval);
				return -1;
			}
			
			ERRSYS_INFOPRINT("CH%dND%d config read\n", chnl_index, node_index);
			return 0;
		}
		else {
			ERRSYS_ERRPRINT("Fail to malloc for l0006_netcmd_ex_packet\n");
		}
	}
	else {
		ERRSYS_ERRPRINT("config is null\n");
	}

	return -1;
}

void l0006_cmdset_beep(struct l0006_netcom_info *nc, struct task_info *wait_task, int msec, int repeat)
{
	struct l0006_netcmd_packet netcmd;
	struct timespec ts_start, ts_now;
	uint32_t timeelapsed;
	int retval = 0;
	ERRSYS_INFOPRINT("#### BEEP ####\n");
	L0006_NETCMD_FORMAT(&netcmd, L0006_NETCMD_BUZZ, l0006_netcom_getseqno(nc), msec, repeat);
	clock_gettime(CLOCK_MONOTONIC, &ts_start);

	if (l0006_netcmd_general_command(nc, wait_task, &netcmd, &retval) < 0) {
		ERRSYS_ERRPRINT("Fail to beep!\n");
		return;
	}
	
	TASK_WAIT_COND(wait_task, (!IS_TASK_ABORT(wait_task) 
		&& (clock_gettime(CLOCK_MONOTONIC, &ts_now), (timeelapsed = l0001_second_elapsed(&ts_now, &ts_start)) <= L0006_NETCMD_COMMAND_TIMEOUT)));
	
	if (retval != L0006_NETCOM_ACK_SUCCESS) {
		ERRSYS_ERRPRINT("Beep timeout %d\n", retval);
	}
	else {
		ERRSYS_INFOPRINT("Beep\n");
		/* beep */
	}
}

int l0006_cmdset_setwindowsize(struct l0006_netcom_info *nc, struct task_info *wait_task, uint8_t chnl_bmp)
{
	struct l0006_netcmd_packet netcmd;
	struct timespec ts_start, ts_now;
	uint32_t timeelapsed;
	int retval = 0;

	L0006_NETCMD_FORMAT(&netcmd, L0006_NETCMD_WINSIZE, l0006_netcom_getseqno(nc), L0006_NETCMD_WINDOWSIZE_DEFAULT, chnl_bmp);
	clock_gettime(CLOCK_MONOTONIC, &ts_start);

	if (l0006_netcmd_general_command(nc, wait_task, &netcmd, &retval) < 0) {
		ERRSYS_ERRPRINT("Fail to set window size!\n");
		return -1;
	}

	TASK_WAIT_COND(wait_task, (!IS_TASK_ABORT(wait_task) 
		&& (clock_gettime(CLOCK_MONOTONIC, &ts_now), (timeelapsed = l0001_second_elapsed(&ts_now, &ts_start)) <= L0006_NETCMD_COMMAND_TIMEOUT)));

	if (retval != L0006_NETCOM_ACK_SUCCESS) {
		ERRSYS_ERRPRINT("Set window size timeout %d\n", retval);
		return -1;
	}

	return 0;
}

int l0006_cmdset_setaddressmode(struct l0006_netcom_info *nc, struct task_info *wait_task)
{
	struct timespec ts_start, ts_now;
	struct l0006_netcmd_packet netcmd;
	uint32_t timeelapsed;
	int retval = 0;
	
	/* send set address */
	L0006_NETCMD_FORMAT(&netcmd, L0006_NETCMD_DEVSTAT, l0006_netcom_getseqno(nc), L0006_NETCMD_DEVSTAT_FUNC1_SET, L0006_NETCMD_DEVSTAT_FUNC2_ADDRMODE);
	clock_gettime(CLOCK_MONOTONIC, &ts_start);
//		dump_proto((uint8_t*)&dscmd, L0006_NETCMD_HEADER_SIZE, 1);
	if (l0006_netcmd_general_command(nc, wait_task, &netcmd, &retval) < 0) {
		return -1;
	}
	//waiting for setaddr ack
	TASK_WAIT_COND(wait_task, (!IS_TASK_ABORT(wait_task) 
		&& (clock_gettime(CLOCK_MONOTONIC, &ts_now), (timeelapsed = l0001_second_elapsed(&ts_now, &ts_start)) <= L0006_NETCMD_COMMAND_TIMEOUT)));

	if (retval != L0006_NETCOM_ACK_SUCCESS) {
		ERRSYS_ERRPRINT("set address mode failure %d\n", retval);
		retval = -1;
	}
	else {
		ERRSYS_INFOPRINT("Address mode is set\n");
		/* beep */
		l0006_cmdset_beep(nc, wait_task, 50, 5);
	}

	return retval;
}

int l0006_cmdset_setnormmode(struct l0006_netcom_info *nc, struct task_info *wait_task)
{
	struct timespec ts_start, ts_now;
	struct l0006_netcmd_packet netcmd;
	uint32_t timeelapsed;
	int retval = 0;
	
	/* send set address */
	L0006_NETCMD_FORMAT(&netcmd, L0006_NETCMD_DEVSTAT, l0006_netcom_getseqno(nc), L0006_NETCMD_DEVSTAT_FUNC1_SET, L0006_NETCMD_DEVSTAT_FUNC2_GENMODE);
	clock_gettime(CLOCK_MONOTONIC, &ts_start);
//		dump_proto((uint8_t*)&dscmd, L0006_NETCMD_HEADER_SIZE, 1);
	if (l0006_netcmd_general_command(nc, wait_task, &netcmd, &retval) < 0) {
		return -1;
	}
	//waiting for setaddr ack
	TASK_WAIT_COND(wait_task, (!IS_TASK_ABORT(wait_task) 
		&& (clock_gettime(CLOCK_MONOTONIC, &ts_now), (timeelapsed = l0001_second_elapsed(&ts_now, &ts_start)) <= L0006_NETCMD_COMMAND_TIMEOUT)));

	if (retval != L0006_NETCOM_ACK_SUCCESS) {
		ERRSYS_ERRPRINT("set address mode failure %d\n", retval);
		retval = -1;
	}
	else {
		ERRSYS_INFOPRINT("Address mode is set\n");
		/* beep */
		l0006_cmdset_beep(nc, wait_task, 255, 5);
	}

	return retval;
}

int l0006_cmdset_querymode(struct l0006_netcom_info *nc, struct task_info *wait_task, struct l0006_netcmd_packet *pkt)
{
	struct timespec ts_start, ts_now;
	struct l0006_netcmd_packet netcmd;
	uint32_t timeelapsed;
	int retval = 0;
	
	/* send set address */
	L0006_NETCMD_FORMAT(&netcmd, L0006_NETCMD_DEVSTAT, l0006_netcom_getseqno(nc), L0006_NETCMD_DEVSTAT_FUNC1_QUERY, 0);
	clock_gettime(CLOCK_MONOTONIC, &ts_start);
//		dump_proto((uint8_t*)&dscmd, L0006_NETCMD_HEADER_SIZE, 1);
	if (l0006_netcmd_general_command(nc, wait_task, &netcmd, &retval) < 0) {
		return -1;
	}
	//waiting for setaddr ack
	TASK_WAIT_COND(wait_task, (!IS_TASK_ABORT(wait_task) 
		&& (clock_gettime(CLOCK_MONOTONIC, &ts_now), (timeelapsed = l0001_second_elapsed(&ts_now, &ts_start)) <= L0006_NETCMD_COMMAND_TIMEOUT)));

	if (retval != L0006_NETCOM_ACK_SUCCESS) {
		ERRSYS_ERRPRINT("set normal mode failure %d\n", retval);
	}
	else {
		if (pkt) {
			memcpy(pkt, &netcmd, L0006_NETCMD_HEADER_SIZE);
		}
	}

	return 0;
}

int l0006_cmdset_recovercboard(struct l0006_netcom_info *nc, struct task_info *wait_task, int chnl_index)
{
	struct timespec ts_start, ts_now;
	struct l0006_netcmd_packet netcmd;
	uint32_t timeelapsed;
	int retval = 0;
	
	/* send set address */
	L0006_NETCMD_FORMAT(&netcmd, L0006_NETCMD_RECOVER, l0006_netcom_getseqno(nc), (1 << chnl_index), L0006_NETCMD_RECOVER_CBOARD_ADDR);
	clock_gettime(CLOCK_MONOTONIC, &ts_start);
//		dump_proto((uint8_t*)&netcmd, L0006_NETCMD_HEADER_SIZE, 1);
	if (l0006_netcmd_general_command(nc, wait_task, &netcmd, &retval) < 0) {
		return -1;
	}
	//waiting for setaddr ack
	TASK_WAIT_COND(wait_task, (!IS_TASK_ABORT(wait_task) 
		&& (clock_gettime(CLOCK_MONOTONIC, &ts_now), (timeelapsed = l0001_second_elapsed(&ts_now, &ts_start)) <= L0006_NETCMD_COMMAND_TIMEOUT)));

	if (retval != L0006_NETCOM_ACK_SUCCESS) {
		ERRSYS_ERRPRINT("recover cboard on CH#%d failure %d\n", chnl_index, retval);
		retval = -1;
	}
	else {
		ERRSYS_INFOPRINT("cboard on CH#%d recovered\n", chnl_index);
		/* beep */
		l0006_cmdset_beep(nc, wait_task, 255, 10);
	}

	return retval;
}

/*
 * Get hboard/cboard version
 * chnl_index = -1: hboard version
 * chnl_index >= 0, node_index >= 0: cboard version
 */
int l0006_cmdset_hardwareversion(struct l0006_netcom_info *nc, struct task_info *wait_task, int chnl_index, int node_index)
{
	struct timespec ts_start, ts_now;
	struct l0006_netcmd_packet netcmd;
	uint32_t timeelapsed;
	struct l0006_netcmd_packet *pkt = NULL;
	int retval = -1;

	if (chnl_index == -1) {
		L0006_NETCMD_FORMAT(&netcmd, L0006_NETCMD_VERSION, l0006_netcom_getseqno(nc), L0006_NETCMD_VERSION_HBOARD, 0);
	}
	else {
		if (chnl_index >= 8 || chnl_index < 0 || node_index >= 16 || node_index < 0) {
			ERRSYS_INFOPRINT("Invalid CBOARD CH%dND%d\n", chnl_index, node_index);
			goto out;
		}
		L0006_NETCMD_FORMAT(&netcmd, L0006_NETCMD_VERSION, l0006_netcom_getseqno(nc), L0006_NETCMD_VERSION_CBOARD, ((chnl_index << 4) | node_index));
	}

	clock_gettime(CLOCK_MONOTONIC, &ts_start);
	if (l0006_netcmd_general_command2(nc, wait_task, &netcmd, &retval, (uint8_t**)&pkt) < 0) {
		goto out;
	}
	
	TASK_WAIT_COND(wait_task, (!IS_TASK_ABORT(wait_task) 
		&& (clock_gettime(CLOCK_MONOTONIC, &ts_now), (timeelapsed = l0001_second_elapsed(&ts_now, &ts_start)) <= L0006_NETCMD_COMMAND_TIMEOUT)));

	if (retval != L0006_NETCOM_ACK_SUCCESS) {
		if (chnl_index == -1) {
			ERRSYS_INFOPRINT("Fail to get hardware version of HBOARD\n");
		}
		else {
			ERRSYS_INFOPRINT("Fail to get hardware version of CBOARD CH%dND%d\n", chnl_index, node_index);
		}
		retval = -1;
	}
	else if (pkt != NULL){
		retval = pkt->f.func[0];
		free(pkt);
	}

out:
	return retval;
}

