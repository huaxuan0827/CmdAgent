#include <event2/event.h>#include <event2/bufferevent.h>#include <event2/buffer.h>#include <sys/socket.h>#include <sys/un.h>#include <netdb.h>#include <netinet/tcp.h>#include "cmdagent.h"#include "sercomm.h"
static void sercom_prep(struct process_info *proc);
static void sercom_run(struct process_info *proc);
// void sercom_prep(struct process_info *proc)
{	struct tasks_cluster *tcluster = (struct tasks_cluster*)proc->root;	struct sercom_proc *ser_proc = (struct devcom_proc*)proc->priv;
	struct task_info *ser_task = GET_TASK(tcluster, CMDAGENT_TASK_NAME_SERVICE);
	
	ERRSYS_INFOPRINT("#### SERCOM PREP #####\n");

	ser_task->chgstat(ser_task, TASK_RUNNING);
}void sercom_run(struct process_info *proc)
{	struct tasks_cluster *tcluster = (struct tasks_cluster*)proc->root;	struct sercom_proc *ser_proc = (struct devcom_proc*)proc->priv;
	struct task_info *ser_task = GET_TASK(tcluster, CMDAGENT_TASK_NAME_SERVICE);
	ERRSYS_INFOPRINT("#### SERCOM RUNNING #####\n");
		sernet_loop(&ser_proc->ser_net);
}// interface.int sercom_initialize(struct process_info *proc, int dev_idx)
{	int retval = -1;	struct tasks_cluster *tcluster = (struct tasks_cluster*)proc->root;	struct sercom_proc *ser_proc;
	struct cmdagent_info agent_info;	struct serclt_op ser_op;
		if ((ser_proc = (struct devcom_proc*)zmalloc(sizeof(struct devcom_proc))) == NULL) {
		ERRSYS_FATALPRINT("Fail to allocate sercom task\n");
		goto err1;	}	proc->priv = ser_proc;
	agent_info = (struct cmdagent_info *)tcluster->parent;

	ser_op->transmitpacket = cmdagent_sendto_device;
	ser_op->param = sercom_proc;
	
	if(sernet_initialize(&ser_proc->ser_net, SERCOM_NET_PATH ,&ser_op) < 0){
		goto err1;	}
	task_set_statmach(proc->parent, TASK_PREP, sercom_prep);
	task_set_statmach(proc->parent, TASK_RUNNING, sercom_run);
	ERRSYS_INFOPRINT("sercom process initialized.\n");
	return 0;err1:	return retval;}void sercom_release(struct process_info *proc)
{	struct tasks_cluster *tcluster = (struct tasks_cluster*)proc->root;	struct devcom_proc *dev_proc = (struct devcom_proc*)proc->priv;	struct task_info *dev_task = GET_TASK(tcluster, CMDAGENT_TASK_NAME_DEVICE);	devnet_disconnect(&dev_proc->dev_net);	devnet_release(&dev_proc->dev_net);}
