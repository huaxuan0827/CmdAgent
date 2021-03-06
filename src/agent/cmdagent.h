#ifndef __CMDAGENT_H__
#define __CMDAGENT_H__

#include "l0001-0/l0001-0.h"
#include "l0002-0/l0002-0.h"
#include "l0003-0/l0003-0.h"

#define CMDAGENT_VERSION				"V1.00"
#define CMDAGENT_OPTIONS				"vh"
#define CMDAGENT_NAMEMAX				128


#define CMDAGENT_TASK_SERVICE			0	
#define CMDAGENT_TASK_DEVICE			1

#define CMDAGENT_TASK_NAME_SERVICE		"service"
#define CMDAGENT_TASK_NAME_DEVICE		"device_%d"

#define CMDAGENT_MAX_DEV_NUM 			32
#define CMDAGENT_DEV_PORT_DEF			7777

struct deventity_info {
	char ipaddr[32];
	unsigned short usport;
	uint32_t rack_index;	
	
	int taskindex;
	char taskname[32];
	void *devproc;
};

struct cmdagent_info {		
	struct tasks_cluster *tcluster;
	struct deventity_info dev_info[CMDAGENT_MAX_DEV_NUM];
	uint32_t dev_num;
};

int cmdagent_register_device(void *param, const char *szdevip, unsigned short usport);
int cmdagent_sendto_device(void *param,const char *szdevip, unsigned short usport, int serid, void *data, int len);
int signal_initialize(struct cmdagent_info *agent_info);
void cmdagent_exit(struct cmdagent_info *agent_info);


#endif

