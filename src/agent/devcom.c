#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include "cmdagent.h"
#include "devcom.h"
#include "sercom.h"

#define MODNAME	 "[A15DC]"

static void devcom_prep(struct process_info *proc);
static void devcom_run(struct process_info *proc);

// 
void devcom_prep(struct process_info *proc)
{
	struct tasks_cluster *tcluster = (struct tasks_cluster*)proc->root;
	struct devcom_proc *dev_proc = (struct devcom_proc*)proc->priv;
	struct task_info *dev_task = GET_TASK(tcluster, CMDAGENT_TASK_DEVICE + dev_proc->dev_idx);
	time_t tbegin, tend;
	
	ERRSYS_INFOPRINT("[%s:%d]#### DEVCOM PREP ##### reconnect_count:%d \n",dev_proc->dev_net.ipaddr, 
		dev_proc->dev_net.usport, dev_proc->reconnect_count);

	dev_proc->reconnect_count++;
	if( dev_proc->reconnect_count >= 3){
		sleep(10);
	}else{
		sleep(1);
	}
	devnet_connect(&dev_proc->dev_net);
	dev_task->chgstat(dev_task, TASK_RUNNING);
}

void devcom_run(struct process_info *proc)
{
	struct tasks_cluster *tcluster = (struct tasks_cluster*)proc->root;
	struct devcom_proc *dev_proc = (struct devcom_proc*)proc->priv;
	struct task_info *dev_task = GET_TASK(tcluster, CMDAGENT_TASK_DEVICE + dev_proc->dev_idx);

	ERRSYS_INFOPRINT("[%s:%d]#### DEVCOM RUNNING #####\n",dev_proc->dev_net.ipaddr, dev_proc->dev_net.usport);
	
	devnet_loop(&dev_proc->dev_net);
	if( IS_TASK_RUNNING(dev_task)){
		devnet_disconnect(&dev_proc->dev_net);
		dev_task->chgstat(dev_task, TASK_PREP);
		ERRSYS_WARNPRINT("%s:%d devnet_loop break and will auto reconnect\n", dev_proc->dev_net.ipaddr, dev_proc->dev_net.usport);
	}
}

// interface.
int devcom_initialize(struct process_info *proc)
{
	int retval = -1;
	struct tasks_cluster *tcluster = (struct tasks_cluster*)proc->root;
	struct devcom_proc *dev_proc;
	struct cmdagent_info *agent_info;
	struct devnet_op dev_op;
	struct task_info *ser_task = GET_TASK(tcluster, CMDAGENT_TASK_SERVICE);
	
	int dev_idx = (int)proc->init_param;
	agent_info = (struct cmdagent_info *)tcluster->parent;
	
	if( dev_idx < 0 || dev_idx > agent_info->dev_num){
		goto err1;
	}
	
	if ((dev_proc = (struct devcom_proc*)zmalloc(sizeof(struct devcom_proc))) == NULL) {
		ERRSYS_FATALPRINT("Fail to allocate devcom task\n");
		goto err1;
	}
	proc->priv = dev_proc;
	agent_info->dev_info[dev_idx].devproc = dev_proc;
	
	dev_proc->dev_idx = dev_idx;
	dev_proc->rack_index = agent_info->dev_info[dev_idx].rack_index;

	strcpy(dev_proc->dev_net.ipaddr, agent_info->dev_info[dev_idx].ipaddr);
	dev_proc->dev_net.usport = agent_info->dev_info[dev_idx].usport;	

	dev_op.param = ser_task->proc.priv;
	dev_op.dealpacket = sercom_write;
		
	if( devnet_initialize(&dev_proc->dev_net, agent_info->dev_info[dev_idx].ipaddr, agent_info->dev_info[dev_idx].usport, &dev_op) < 0){
		goto err1;
	}
	dev_proc->reconnect_count = 0;
		
	task_set_statmach(proc->parent, TASK_PREP, devcom_prep);
	task_set_statmach(proc->parent, TASK_RUNNING, devcom_run);

	ERRSYS_INFOPRINT("[%s:%d]devcom process initialized.\n",dev_proc->dev_net.ipaddr, dev_proc->dev_net.usport);
	return 0;
err1:
	return retval;
}

void devcom_release(struct process_info *proc)
{
	struct tasks_cluster *tcluster = (struct tasks_cluster*)proc->root;
	struct devcom_proc *dev_proc = (struct devcom_proc*)proc->priv;
	//struct task_info *dev_task = GET_TASK(tcluster, CMDAGENT_TASK_NAME_DEVICE + dev_proc->dev_idx);

	devnet_disconnect(&dev_proc->dev_net);
	devnet_release(&dev_proc->dev_net);
}

int devcom_write(struct devcom_proc *devproc,int serid, void *data, int len)
{
	if( devproc == NULL || data == NULL){
		return -1;
	}
	return devnet_write(&devproc->dev_net,serid, data, len);
}


