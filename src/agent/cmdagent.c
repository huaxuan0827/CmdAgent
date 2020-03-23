#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "cmdagent.h"
#include "hboard.h"


#define MODNAME			"[A18MT]"

void usage(void)
{
	printf("Usage: cmdagent-0 [OPTION]\n");
	printf("  -v\t\tprint version\n");
	printf("  -h\t\tprint this help\n");
	exit(0);
}

void cmdagent_version(char *image)
{
	printf("%s %s Build: %s %s\n", image, MONITOR_VERSION, __DATE__, __TIME__);
	exit(0);
}

int verify_argument(struct monitor_info *db)
{
	return 0;
}

int parse_argument(int argc, char* argv[], struct monitor_info *monitor)
{
	int retval = 0;
	int opt;
	while ((opt = getopt(argc, argv, MONITOR_OPTIONS)) != -1) {
		switch (opt) {
			case 'v':
				monitor_version(argv[0]);
				break;
			case 'h':
				usage();
				break;
			default:
				retval = -1;
				break;
		}
	}
	retval = verify_argument(monitor);
	return retval;
}

void cmdagent_exit(struct monitor_info *monitor) 
{

}

static int cmdagent_tasks_initialize(struct monitor_info *monitor)
{
	int i;
	struct tasks_cluster *tcluster = monitor->tcluster;
	struct task_info *task = tcluster->task;

	/* multiple datasink entities */
	task_instantiate(tcluster, MONITOR_TASK_ALARM, MONITOR_TASK_NAME_ALARM, 0, alarm_initialize, alarm_release);
	task_instantiate(tcluster, MONITOR_TASK_STATUS, MONITOR_TASK_NAME_STATUS, 0, status_initialize, status_release);
	task_instantiate(tcluster, MONITOR_TASK_CCA, MONITOR_TASK_NAME_CCA, 0, ccagent_initialize, ccagent_release);

	for (i = 0;i < MONITOR_TASKS_MAX;i ++) {
		if (task[i].initialize != NULL) {
			if (task[i].initialize(tcluster, i) < 0) {
				ERRSYS_FATALPRINT("Fail to initialize task #%d\n", i);
				goto err;
			}
		}
	}
	return 0;
err:
	i--;
	for (;i >= 0;i --) {
		if (task[i].release != NULL) {
			task[i].release(tcluster, i);
		}
	}
	return -1;
}

static int cmdagent_tasks_startup(struct monitor_info *monitor) 
{
	int retval = 0;
	unsigned long i = 0;
	struct task_info *task = monitor->tcluster->task;

	for (i = 0; i < MONITOR_TASKS_MAX; i++) {
		if (task[i].startup) {
			retval = task[i].startup(&task[i]);
			if (retval < 0)
				goto err1;
		}
	}

	return 0;

err1: 
	for (; i >= 0; i--) {
		if (task[i].stop) {
			task[i].stop(&task[i]);
		}
	}
	return retval;
}


void cmdagent_tasks_release(struct monitor_info *monitor)
{
	int i;
	struct task_info *task = monitor->tcluster->task;

	for (i = 0;i < MONITOR_TASKS_MAX;i ++) {
		if (task[i].release != NULL) {
			task[i].release(monitor->tcluster, i);
		}
	}
}

uint32_t page_align(uint32_t sz)
{
	uint32_t s = (sz + L0007_GLOBALADSYNC_SIZE_DEFAULT - 1) & (~(L0007_GLOBALADSYNC_SIZE_DEFAULT - 1));
	ERRSYS_INFOPRINT("Align size %u to %u\n", sz, s);

	return s;
}

int open_shmstruct_globalconfig(struct monitor_info *monitor)
{	
	struct l0017_shmstruct* shmstruct = NULL;
	int nlen = 0;

	nlen = sizeof(struct l0017_shmstruct) + sizeof(struct l0007_global_conf);
	shmstruct = l0017_shmstruct_open(SHM_STRUCT_NAME, nlen, 0);
	if( shmstruct == NULL){
		ERRSYS_FATALPRINT("Fail to open shmstruct shm:%s\n", SHM_STRUCT_NAME);
		goto err1;
	}
	monitor->shm_struct = shmstruct;
	monitor->global_config = shmstruct->mem;
 
	ERRSYS_INFOPRINT("shm struct ok:  nlen=%d, sz=%d \n", nlen, shmstruct->sz);
	return 0;
err1:
	return -1;
}

int open_sharedmem_dataxchgtable(struct monitor_info *monitor)
{
	// open dataxchgtable.
	size_t tSize = 0;
	if( l0017_dataxchgtable_open(NULL, &tSize, 0) != 0){
		ERRSYS_FATALPRINT("Fail to open Data Exchange Table\n");
		goto err;
	}

	// parse dxt conf.
	char errInfo[L0017_DXT_ERRINFO_MAX_LEN] = {0};
	strcpy(monitor->dxt_cfgfile, DXTI_CONF_FILE);
	if( l0017_dxt_parseconf(monitor->dxt_conf, monitor->dxt_cfgfile, errInfo, L0017_DXT_ERRINFO_MAX_LEN, 0) != 0){
		ERRSYS_FATALPRINT("dxt_parseconf failed, cfgfile:%s, errinfo:%s\n", monitor->dxt_cfgfile, errInfo);
		goto err;
	}
	
	// print dxt conf.
	l0017_dxt_printconf(monitor->dxt_conf);

	// get entry.
	if( l0017_dxt_get_entry(monitor->dxt_conf, monitor->dxt_info, errInfo, L0017_DXT_ERRINFO_MAX_LEN) != 0){
		ERRSYS_FATALPRINT("dxt_get_entry failed, errinfo:%s\n", errInfo);
		goto err;
	}

	// get rel entry.
	if( l0017_dxt_get_refentry(monitor->dxt_conf, monitor->dxt_info, errInfo, L0017_DXT_ERRINFO_MAX_LEN) != 0){
		ERRSYS_FATALPRINT("dxt_get_entry failed, errinfo:%s\n", errInfo);
		goto err;
	}
	return 0;
err:
	return -1;
}

int close_sharedmem_dataxchgtable(struct monitor_info *monitor)
{
	l0017_dataxchgtable_close();
	return -1;
}

int dxts_status_init(struct monitor_info *monitor)
{
	char errbuf[L0017_DXT_ERRINFO_MAX_LEN];
	if( l0017_dxts_init(L0017_DXT_CONF_PATH, monitor->dxts_info, errbuf, L0017_DXT_ERRINFO_MAX_LEN) < 0){
		ERRSYS_FATALPRINT("dxts status init failed, errinfo:%s", errbuf);
	}
	l0017_dxts_print(monitor->dxts_info);
	return 0;
}

void release_sharedmem(struct monitor_info *monitor)
{
	ERRSYS_INFOPRINT("l0017_shmstruct_close shm_struct!!! \n");
	if (monitor->shm_struct != NULL){
		l0017_shmstruct_close(monitor->shm_struct);
		monitor->shm_struct = NULL;
	}	
}

int main(int argc, char *argv[])
{
	int i;
	int retval = -1;
	struct monitor_info *monitor = zmalloc(sizeof(struct monitor_info));
	struct task_info *task;

	monitor->dxt_conf = zmalloc(sizeof(struct l0017_dxt_appconf));
	monitor->dxt_info = zmalloc(sizeof(struct l0017_app_dxtinfo));
	monitor->dxts_info = zmalloc(sizeof(struct l0017_dxts_cfginfo));
	
	errsys_features(ERRSYS_FEATURE_PRINT, ERRSYS_LEVEL_DEBUG, NULL);
	errsys_features(ERRSYS_FEATURE_SYSLOG, ERRSYS_LEVEL_INFO, NULL);
	
	if (errsys_initialize() < 0) {
		goto err1;
	}

	if (monitor == NULL || monitor->dxts_info == NULL || monitor->dxt_conf == NULL || monitor->dxt_info == NULL){
		ERRSYS_FATALPRINT("Fail to allocate memeory\n");
		goto err2;
	}	

	if( open_sharedmem_dataxchgtable(monitor) < 0){
		ERRSYS_FATALPRINT("Fail to create share memory for dataxchgtable\n");
		goto err2;
	}

	if( dxti_initialize(monitor->dxt_conf, monitor->dxt_info) < 0){
		ERRSYS_FATALPRINT("Fail to init dxt info\n");
		goto err3;
	}

	dxti_appinfo_setval(DXTI_APP_ID_PROC_PID, getpid());
	dxti_appinfo_setval_procalarm(L0017_DXTS_ALARM_TURN_ON);
	dxti_appinfo_setval_procstatus(L0017_DXTS_PROC_ST_INIT);

	if (signal_initialize(monitor) < 0) {
		ERRSYS_FATALPRINT("Fail to register signal handler\n");
		goto err3;
	}

	if (parse_argument(argc, argv, monitor) < 0) {
		ERRSYS_FATALPRINT("Fail to parse arguments\n");
		goto err3;
	}

	if (singleton_initialize(NULL) < 0) {
		ERRSYS_FATALPRINT("Fail to initialize singleton\n");
		goto err3;
	}

	if (open_shmstruct_globalconfig(monitor) < 0){
		ERRSYS_FATALPRINT("Fail to open shm struct global config\n");
		goto err4;
	}
	
	if( dxts_status_init(monitor)< 0){
		ERRSYS_FATALPRINT("Fail to init dxts_status\n");
		goto err4;
	}
	
	monitor->tcluster = taskcluster_create(MONITOR_TASKS_MAX, MODNAME, monitor);//modname as cluster task log prefix & pts directory
	if (monitor->tcluster == NULL) {
		ERRSYS_FATALPRINT("Fail to create task cluster\n");
		goto err5;
	}
	task = monitor->tcluster->task;

	// wait all app startup.
	sleep(5);
	if ((retval = monitor_tasks_initialize(monitor)) < 0) {
		goto err6;
	}

	if ((retval = monitor_tasks_startup(monitor)) < 0) {
		goto err6;
	}

	//poll the exit state
	ERRSYS_INFOPRINT("Polling tasks status\n");
	dxti_appinfo_setval_procstatus(L0017_DXTS_PROC_ST_RUNNING);
	while(1) {
		for (i = 0; i < MONITOR_TASKS_MAX; i++) {
			if (!IS_TASK_QUIT(&task[i])) {
				break;
			}
		}
		if (i == MONITOR_TASKS_MAX) {
			break;
		}
		sleep(1);
	}
	monitor_tasks_release(monitor);
	dxti_appinfo_setval_procstatus(L0017_DXTS_PROC_ST_QUIT);

err6:
	taskcluster_release(monitor->tcluster);
err5:
	release_sharedmem(monitor);
err4:
	singleton_release();
err3:
	close_sharedmem_dataxchgtable(monitor);
err2:
	errsys_release();
err1:
	return 0;
}

