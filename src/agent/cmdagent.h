#ifndef __CMDAGENT_H__
#define __CMDAGENT_H__

#include "l0001-0/l0001-0.h"
#include "l0002-0/l0002-0.h"
#include "l0003-0/l0003-0.h"
#include "l0006-0/l0006-0.h"


#define CMDAGENT_VERSION				"V1.00"
#define CMDAGENT_OPTIONS				"vh"
#define CMDAGENT_NAMEMAX				128


#define CMDAGENT_TASK_ALARM				0	
#define CMDAGENT_TASK_STATUS			1
#define CMDAGENT_TASK_CCA				2

#define CMDAGENT_TASK_NAME_SERVICE		"service"
#define CMDAGENT_TASK_NAME_DEVICE		"device"

#define CMDAGENT_MAX_DEV_NUM 			16

struct deventity_info {
	char ipaddr[24];
	unsigned short usport;
	uint32_t rack_index;
	
	struct tasks_cluster *tcluster;
};

struct cmdagent_info {
	struct tasks_cluster *tcluster;
	
	//struct l0017_shmstruct* shm_struct;
	//void *global_config;

	/* dataxchgtable*/
	//char dxt_cfgfile[L0017_DXT_PATH_MAX_LEN];
	//struct l0017_dxt_appconf *dxt_conf;
	//struct l0017_app_dxtinfo *dxt_info;

	struct deventity_info dev_info[CMDAGENT_MAX_DEV_NUM];
	uint32_t dev_bmp;
	uint32_t dev_num;
};


int signal_initialize(struct cmdagent_info *agent_info);
void cmdagent_exit(struct cmdagent_info *agent_info) ;

#endif

