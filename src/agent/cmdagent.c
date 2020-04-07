#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "devcom.h"
#include "sercom.h"
#include "cmdagent.h"

#define MODNAME			"[A15MT]"

void usage(void)
{
	printf("Usage: cmdagent-0 [OPTION]\n");
	printf("  -v\t\tprint version\n");
	printf("  -h\t\tprint this help\n");
	exit(0);
}

void cmdagent_version(char *image)
{
	printf("%s %s Build: %s %s\n", image, CMDAGENT_VERSION, __DATE__, __TIME__);
	exit(0);
}

int verify_argument(struct cmdagent_info *agent)
{
	return 0;
}

int parse_argument(int argc, char* argv[], struct cmdagent_info *agent)
{
	int retval = 0;
	int opt;
	while ((opt = getopt(argc, argv, CMDAGENT_OPTIONS)) != -1) {
		switch (opt) {
			case 'v':
				cmdagent_version(argv[0]);
				break;
			case 'h':
				usage();
				break;
			default:
				retval = -1;
				break;
		}
	}
	retval = verify_argument(agent);
	return retval;
}

void cmdagent_exit(struct cmdagent_info *agent) 
{

}

static int cmdagent_tasks_initialize(struct cmdagent_info *agent)
{
	int i;
	struct tasks_cluster *tcluster = agent->tcluster;
	struct task_info *task = tcluster->task;

	/* multiple datasink entities */
	task_instantiate2(tcluster, CMDAGENT_TASK_SERVICE, CMDAGENT_TASK_NAME_SERVICE, 0, sercom_initialize, sercom_release, (void *)agent);
	for(i = 0; i < agent->dev_num; i++){
		sprintf(agent->dev_info[i].taskname, CMDAGENT_TASK_NAME_DEVICE, i);
		agent->dev_info[i].taskindex = CMDAGENT_TASK_DEVICE + i;
		task_instantiate2(tcluster, agent->dev_info[i].taskindex, agent->dev_info[i].taskname, 0, 
			devcom_initialize, devcom_release, i);
	}
	
	for (i = 0;i < agent->dev_num + 1;i ++) {
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

static int cmdagent_tasks_startup(struct cmdagent_info *agent) 
{
	int retval = 0;
	unsigned long i = 0;
	struct task_info *task = agent->tcluster->task;

	for (i = 0; i < agent->dev_num + 1; i++) {
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

void cmdagent_tasks_release(struct cmdagent_info *agent)
{
	int i;
	struct task_info *task = agent->tcluster->task;

	for (i = 0;i < agent->dev_num + 1;i ++) {
		if (task[i].release != NULL) {
			task[i].release(agent->tcluster, i);
		}
	}
}

int cmdagent_initdxt(struct cmdagent_info *agent)
{
	return 0;
}

int cmdagent_uninitdxt(struct cmdagent_info *agent)
{
	return 0;
}

int cmdagnet_initconfig(struct cmdagent_info *agent)
{
#if 0
	int idx = 0;
	unsigned short usport = 9900;

	evthread_use_pthreads();//enable threads 
	
	agent->dev_num = 2;
	for(idx = 0; idx < agent->dev_num; idx++){
		strcpy(agent->dev_info[idx].ipaddr, "192.168.245.128");
		agent->dev_info[idx].usport = usport+idx;
		agent->dev_info[idx].rack_index = idx;
	}
#else
	agent->dev_num = 0;
#endif
	return 0;
}

int cmdagent_uninitconfig(struct cmdagent_info *agent)
{
	return 0;
}

int main(int argc, char *argv[])
{
	int i,retval = -1;
	struct task_info *task;
	
	struct cmdagent_info *cmdagent = zmalloc(sizeof(struct cmdagent_info));
	
	errsys_features(ERRSYS_FEATURE_PRINT, ERRSYS_LEVEL_DEBUG, NULL);
	errsys_features(ERRSYS_FEATURE_SYSLOG, ERRSYS_LEVEL_INFO, NULL);
	
	if (errsys_initialize() < 0) {
		goto err1;
	}

	if( cmdagent_initdxt(cmdagent) < 0){
		ERRSYS_FATALPRINT("init dxt failed\n");
		goto err2;
	}
	
	if (signal_initialize(cmdagent) < 0) {
		ERRSYS_FATALPRINT("Fail to register signal handler\n");
		goto err3;
	}

	if (parse_argument(argc, argv, cmdagent) < 0) {
		ERRSYS_FATALPRINT("Fail to parse arguments\n");
		goto err3;
	}

	if (singleton_initialize(NULL) < 0) {
		ERRSYS_FATALPRINT("Fail to initialize singleton\n");
		goto err3;
	}

	if( cmdagnet_initconfig(cmdagent) < 0){
		ERRSYS_FATALPRINT("Fail to parse arguments\n");
		goto err4;
	}
	
	cmdagent->tcluster = taskcluster_create(cmdagent->dev_num + 1, MODNAME, cmdagent);//modname as cluster task log prefix & pts directory
	if (cmdagent->tcluster == NULL) {
		ERRSYS_FATALPRINT("Fail to create task cluster\n");
		goto err5;
	}
	task = cmdagent->tcluster->task;
	
	// wait all app startup.
	sleep(5);
	if ((retval = cmdagent_tasks_initialize(cmdagent)) < 0) {
		goto err6;
	}

	if ((retval =cmdagent_tasks_startup(cmdagent)) < 0) {
		goto err6;
	}

	//poll the exit state
	ERRSYS_INFOPRINT("Polling tasks status\n");
	while(1) {
		for (i = 0; i < cmdagent->dev_num + 1; i++) {
			if (!IS_TASK_QUIT(&task[i])) {
				break;
			}
		}
		if (i == cmdagent->dev_num + 1) {
			break;
		}
		sleep(1);
	}
	cmdagent_tasks_release(cmdagent);

err6:
	taskcluster_release(cmdagent->tcluster);
err5:
	cmdagent_uninitconfig(cmdagent);
err4:
	singleton_release();
err3:
	cmdagent_uninitdxt(cmdagent);
err2:
	errsys_release();
err1:
	return 0;
}

int cmdagent_register_device(void *param, const char *szdevip, unsigned short usport)
{
	if( szdevip == NULL || usport == 0){
		return -1;
	}
	int idx = 0, nret = -1, bfind = 0, taskindex = 0;
	struct cmdagent_info *agent = (struct cmdagent_info *)param;
	struct tasks_cluster *tcluster = agent->tcluster;
	struct task_info *task = tcluster->task;


	for( idx = 0; idx < agent->dev_num; idx++){
		if( strcmp(szdevip, agent->dev_info[idx].ipaddr) == 0 && usport == agent->dev_info[idx].usport){
			bfind = 1;
			break;
		}
	}
	if( bfind){
		ERRSYS_WARNPRINT("CMDAGENT add device ip=%s, port=%d, but this device is create!!!\n", szdevip, usport);
		return 0;
	}

	idx = agent->dev_num;
	if( idx >= CMDAGENT_MAX_DEV_NUM ){
		ERRSYS_WARNPRINT("CMDAGENT add device ip=%s, port=%d, but device list is full, size=%d !!!\n", szdevip, usport, agent->dev_num);
		return -1;
	}

	taskindex = CMDAGENT_TASK_DEVICE + idx;
	sprintf(agent->dev_info[idx].taskname, CMDAGENT_TASK_NAME_DEVICE, idx);
	agent->dev_info[idx].taskindex = taskindex;
	task_instantiate2(agent->tcluster, agent->dev_info[idx].taskindex, agent->dev_info[idx].taskname, 0, 
			devcom_initialize, devcom_release,idx);
	
	if( task[taskindex].initialize != NULL) {
		if (task[taskindex].initialize(tcluster, taskindex) < 0) {
			ERRSYS_FATALPRINT("CMDAGENT Fail to initialize task #%d, device ip=%s, port=%d\n", taskindex, szdevip, usport);
			return -1;
		}
	}
	
	if( task[taskindex].startup(&task[taskindex])){
		ERRSYS_FATALPRINT("CMDAGENT Fail to startup task #%d, device ip=%s, port=%d\n", taskindex, szdevip, usport);
		return -1;
	}
	return 0;
}

int cmdagent_sendto_device(void *param, const char *szdevip, unsigned short usport, int serid, void *data, int len)
{
	if( szdevip == NULL || usport == 0 || data == 0){
		return -1;
	}
	int idx = 0, nret = -1;
	struct cmdagent_info *agent = (struct cmdagent_info *)param;
	int bfind = 0;
	struct devcom_proc *devproc;
	
	for( idx = 0; idx < agent->dev_num; idx++){
		if( strcmp(szdevip, agent->dev_info[idx].ipaddr) == 0 && usport == agent->dev_info[idx].usport){
			bfind = 1;
			break;
		}
	}
	if( !bfind){
		ERRSYS_WARNPRINT("CMDAGENT not find dev ip=%s, port=%d\n", szdevip, usport);
		return -1;
	}
	devproc = (struct devcom_proc *)agent->dev_info[idx].devproc;
		
	nret = devcom_write(devproc,serid,data, len);
	if( nret < 0){
		ERRSYS_ERRPRINT("CMDAGENT serid:%d,datalen:%d, write data to dev ip=%s, port=%d failed!!!\n",serid,len,szdevip,usport);
	}
	return 0;
}

