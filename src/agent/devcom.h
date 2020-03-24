#ifndef __DEVCOM_H__
#define __DEVCOM_H__

#include "devnet.h"

#define DEVCOM_TIMEOUT_PERIOD_US		1000000UL
#define DEVCOM_TIMEOUT_PERIOD_MS		(DEVCOM_TIMEOUT_PERIOD_US / 1000)

struct devcom_proc{
	int dev_idx;
	int rack_index;
	void *devcom_task;
	struct devnet_info dev_net;
	int reconnect_count;
};

int devcom_initialize(struct process_info *proc, int dev_idx);
void devcom_release(struct process_info *proc);

int devcom_write(struct devcom_proc *devproc, void *data, int len);

#endif
