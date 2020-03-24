#ifndef __SERCOM_H__
#define __SERCOM_H__

#include "sernet.h"

#define SERCOM_TIMEOUT_PERIOD_US		1000000UL
#define SERCOM_TIMEOUT_PERIOD_MS		(DEVCOM_TIMEOUT_PERIOD_US / 1000)

struct sercom_proc{
	void *sercom_task;
	struct devnet_info dev_net;
	int reconnect_count;
};

int sercom_initialize(struct process_info *proc, int dev_idx);
void sercom_release(struct process_info *proc);

int sercom_write(struct sercom_proc *serproc, void *data, int len);

#endif



