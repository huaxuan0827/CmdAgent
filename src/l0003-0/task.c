#include <errno.h>
#include <sys/syscall.h>
#include <l0001-0/l0001-0.h>
#include <l0002-0/l0002-0.h>
#include "l0003-0.h"

#define gettid() syscall(__NR_gettid)
#define MODNAME_MAX			32

static char modname[MODNAME_MAX];

#define MODNAME modname

static int task_setpriority(struct task_info *task, int priority)
{
	if (task) {
		task->priority = priority;
		return setpriority(PRIO_PROCESS,0,task->priority);
	}

	return -1;
}

static void task_changestate(struct task_info *task, uint32_t newstate)
{
	struct tasks_cluster* tcluster = (struct tasks_cluster*)task->parent;

	STATMACH_WLOCK(tcluster);
	if (!IS_TASK_ABORT(task)) {
		WRITE_TASK_FLAGS(task, newstate);
	}
	else if (newstate == TASK_QUIT) {
		WRITE_TASK_FLAGS(task, TASK_QUIT);
	}
	STATMACH_UNLOCK(tcluster);
}

static void task_wakeup(struct task_info *task)
{
	pthread_mutex_lock(&task->mutex);
	pthread_cond_broadcast(&task->cond);
	task->wakeup_early = 1;
	pthread_mutex_unlock(&task->mutex);
}

static void task_wakeupclear(struct task_info *task)
{
	pthread_mutex_lock(&task->mutex);
	task->wakeup_early = 0;
	pthread_mutex_unlock(&task->mutex);
}

static void task_wait(struct task_info *task)
{
	struct timespec ts;
	int retval = -1;

	pthread_mutex_lock(&task->mutex);
	clock_gettime(CLOCK_MONOTONIC, &ts);
	while(retval && !IS_TASK_ABORT(task) && task->wakeup_early == 0) {
		ts.tv_sec ++;
		retval = pthread_cond_timedwait(&task->cond, &task->mutex, &ts);
		//TODO: heartbeat
//		printf("%s heartbeat\n", task->name);
		HEARTBEATS(task);
	}
	task->wakeup_early = 0;
	pthread_mutex_unlock(&task->mutex);
}

static void task_heartbeat(struct task_info *task)
{
	task->heartbeats ++;
}

static void task_idle(struct process_info *proc)
{
	struct task_info *task = (struct task_info*)proc->parent;
	TAKEASHORTBREATH();
	HEARTBEATS(task);
}

static int task_process(void *p)
{
	struct task_info *task = (struct task_info*)p;
	uint32_t statmach;
	
	task->tid = gettid();
	ERRSYS_INFOPRINT("Task [%s] starts(PID:%d TID:%d).\n", task->name, getpid(), task->tid);
	setpriority(PRIO_PROCESS,0,task->priority);
	task->chgstat(task, TASK_PREP);
	
	while(!IS_TASK_ABORT(task)) {
		statmach = task->flags & TASK_MASK;
		if (task->statmach_func[statmach])
			task->statmach_func[statmach](&task->proc);
	}
	
	ERRSYS_INFOPRINT("Task [%s] stopped.\n", task->name);
	return 0;
}

static void task_release(struct tasks_cluster* tcluster, int task_index)/* release is to free the resource */
{
	struct task_info *task;
	task = &tcluster->task[task_index];

	if(task){
		ERRSYS_INFOPRINT("Release task %s.\n", task->name);

		task->proc.release(&task->proc);
		
		if (pthread_rwlock_destroy(&(task->rwlock)) < 0) {
			ERRSYS_FATALPRINT("Fail to destroy rwlock\n");		
		}
		if (pthread_rwlockattr_destroy(&(task->rwlock_attr)) < 0) {
			ERRSYS_FATALPRINT("Fail to destroy rwlock attribute\n");
		}
		if (pthread_cond_destroy(&task->cond) < 0) {
			ERRSYS_FATALPRINT("Fail to destroy cond\n");		
		}
		if (pthread_condattr_destroy(&task->cattr) < 0) {
			ERRSYS_FATALPRINT("Fail to destroy condattr\n");		
		}
	
		if (pthread_mutex_destroy(&task->mutex) < 0) {
			ERRSYS_FATALPRINT("Fail to destroy mutex\n");		
		}

		close(task->fd_pts);
	}
}

static int task_initialize(struct tasks_cluster* tcluster, int task_index)
{
	struct task_info *task;
	int retval = -1;

	if (tcluster == NULL) {
		ERRSYS_FATALPRINT("Invalid argument for task_initialize\n");		
		goto err1;
	}
	
	task = &tcluster->task[task_index];
	task->parent = (void*)tcluster;
	task->proc.parent = task;
	task->proc.root = tcluster;
	task->wakeup_early = 0;
	snprintf(task->ptsname, PTSNAME_MAX, "%s/%s/%s", PTSNAME_DIR, tcluster->name, task->name);
	
	if ((task->fd_pts = ptsopen(task->ptsname)) < 0) {
		ERRSYS_FATALPRINT("Fail to create pts(%s) for %s: %s\n", task->ptsname, task->name, strerror(errno));
		goto err1;
	}
	
	if ((retval = pthread_mutex_init(&task->mutex, NULL)) < 0) {
		ERRSYS_FATALPRINT("Fail to initialize mutex for %s: %s\n", task->name, strerror(errno));
		goto err2;
	}
	if ((retval = pthread_condattr_init(&task->cattr)) < 0) {
		ERRSYS_FATALPRINT("Fail to initialize condattr for %s: %s\n", task->name, strerror(errno));
		goto err3;
	}
	if ((retval = pthread_condattr_setclock(&(task->cattr), CLOCK_MONOTONIC)) < 0) {
		ERRSYS_FATALPRINT("Fail to set CLOCK_MONOTONIC for %s: %s\n", task->name, strerror(errno));
		goto err4;			
	}
	if ((retval = pthread_cond_init(&task->cond, &(task->cattr))) < 0) {
		ERRSYS_FATALPRINT("Fail to initialize cond for %s: %s\n", task->name, strerror(errno));
		goto err4;
	}
	
	if ((retval = pthread_rwlockattr_init(&(task->rwlock_attr))) < 0) {
		ERRSYS_FATALPRINT("Fail to initialize rwlock attribute for %s: %s\n", task->name, strerror(errno));
		goto err5;
	}
	if ((retval = pthread_rwlock_init(&(task->rwlock),&(task->rwlock_attr))) < 0) {
		ERRSYS_FATALPRINT("Fail to initialize rwlock for %s: %s\n", task->name, strerror(errno));
		goto err6;
	}

	//TODO: some private initialization
	if (task->proc.initialize) {
		if ((retval = task->proc.initialize(&task->proc)) < 0) {
			ERRSYS_FATALPRINT("Fail to initialize task private structure for %s\n", task->name);
			goto err7;
		}
	}
	
	ERRSYS_INFOPRINT("%s task initialized.\n", task->name);

	return 0;

err7:
	if (pthread_rwlock_destroy(&(task->rwlock)) < 0) {
		ERRSYS_FATALPRINT("Fail to destroy rwlock: %s\n", strerror(errno));
	}
err6:
	if (pthread_rwlockattr_destroy(&(task->rwlock_attr)) < 0) {
		ERRSYS_FATALPRINT("Fail to destroy rwlock attribute: %s\n", strerror(errno));
	}
	
err5:
	if (pthread_cond_destroy(&task->cond) < 0) {
		ERRSYS_FATALPRINT("Fail to destroy cond: %s\n", strerror(errno));		
	}
err4:
	if (pthread_condattr_destroy(&task->cattr) < 0) {
		ERRSYS_FATALPRINT("Fail to destroy condattr: %s\n", strerror(errno));		
	}

err3:
	if (pthread_mutex_destroy(&task->mutex) < 0) {
		ERRSYS_FATALPRINT("Fail to destroy mutex: %s\n", strerror(errno));		
	}

err2:
	close(task->fd_pts);
err1:
	return retval;
}

int task_startup(pthread_t *thrd,pthread_attr_t* attr,void*p,int(*fun)(void*))
{
	size_t stacksize;

	/* init default ai thread */
	if (pthread_attr_init(attr) != 0)
		return -1;

	if(pthread_attr_setstacksize(attr,0x00800000) != 0)
		return -1;
	
	if(pthread_attr_getstacksize(attr,&stacksize) != 0)
		return -1;

//	printf("Thread default stack size: 0x%08lx\n",(unsigned long)stacksize);
	
	/* main thread waiting for its exiting */
	if (pthread_attr_setdetachstate(attr, PTHREAD_CREATE_JOINABLE) != 0)
		return -1;

	/* create thread */
	if (pthread_create(thrd, attr, (void *)fun, (void*)p) != 0)
		return -1;

	return 0;
}

static int task_start(struct task_info *task)
{
	int rc;
	rc = task_startup(&task->taskid, &task->attr, task, task_process);

	return rc;
}

static int task_stop(struct task_info *task)
{
	/* set task to exit */
	ERRSYS_INFOPRINT("Stopping %s\n", task->name);
	task->chgstat(task, TASK_ABORT);
	pthread_join(task->taskid, NULL);
	pthread_attr_destroy(&(task->attr));
	task->chgstat(task, TASK_QUIT);
	ERRSYS_DEBUGPRINT("Task %s quit\n", task->name);
	return 0;
}

int task_set_statmach(struct task_info *task, int state, STATMACH_FUNC func)
{
	if (state < TASKSTATMACH_MAX) {
		task->statmach_func[state] = func;
		return 0;
	}

	return -1;
}

void task_instantiate(struct tasks_cluster* tcluster, int task_index, const char *name, int priority, PROC_INIT init_func, PROC_REL rel_func)
{
	int i;

	struct task_info *task = &tcluster->task[task_index];
	
	memset(task, 0, sizeof(struct task_info));
	strncpy(task->name, name, TASKNAME_MAX);
	task->priority = priority;
	task->initialize = task_initialize;
	task->release = task_release;
	task->startup = task_start;
	task->stop = task_stop;
	task->wait = task_wait;
	task->wakeup = task_wakeup;
	task->wakeupclear = task_wakeupclear;
	task->chgstat = task_changestate;
	task->setpriority = task_setpriority;
	task->heartbeat = task_heartbeat;
	task->proc.initialize = init_func;
	task->proc.release = rel_func;
	task->proc.parent = task;
	task->proc.root = task->parent;

	for (i = 0;i < TASKSTATMACH_MAX;i ++) {
		task_set_statmach(task, i, task_idle);
	}
}

void task_instantiate2(struct tasks_cluster* tcluster, int task_index, const char *name, int priority, PROC_INIT init_func, PROC_REL rel_func, void *init_param)
{
	int i;

	struct task_info *task = &tcluster->task[task_index];
	
	memset(task, 0, sizeof(struct task_info));
	strncpy(task->name, name, TASKNAME_MAX);
	task->priority = priority;
	task->initialize = task_initialize;
	task->release = task_release;
	task->startup = task_start;
	task->stop = task_stop;
	task->wait = task_wait;
	task->wakeup = task_wakeup;
	task->wakeupclear = task_wakeupclear;
	task->chgstat = task_changestate;
	task->setpriority = task_setpriority;
	task->heartbeat = task_heartbeat;
	task->proc.initialize = init_func;
	task->proc.release = rel_func;
	task->proc.parent = task;
	task->proc.root = task->parent;
	task->proc.init_param = init_param;

	for (i = 0;i < TASKSTATMACH_MAX;i ++) {
		task_set_statmach(task, i, task_idle);
	}
}


struct tasks_cluster* taskcluster_create(uint32_t tasks_num, const char* name, void* parent)
{
	struct tasks_cluster* tcluster = zmalloc(sizeof(struct tasks_cluster));
	
	if (tcluster) {
		tcluster->task = zmalloc(sizeof(struct task_info) * tasks_num);
		if (tcluster->task) {
			char ptspath[128];
			tcluster->tasks_num = tasks_num;

			memset(modname, 0, MODNAME_MAX);
			memset(tcluster->name, 0, CLUSTERNAME_MAX);
			//log prefix name
			strncpy(modname, name, MODNAME_MAX - 1);
			//pts directory name
			strncpy(tcluster->name, name, CLUSTERNAME_MAX - 1);
			
			if ((pthread_rwlockattr_init(&(tcluster->rwlock_attr))) < 0) {
				ERRSYS_FATALPRINT("Fail to initialize rwlock attribute\n");
				goto err2;
			}
			if ((pthread_rwlock_init(&(tcluster->stm_rwlock),&(tcluster->rwlock_attr))) < 0) {
				ERRSYS_FATALPRINT("Fail to initialize rwlock\n");
				goto err3;
			}
			tcluster->parent = parent;

			memset(ptspath, 0, 128);
			snprintf(ptspath, 128, "%s/%s", PTSNAME_DIR, tcluster->name);
			if (mkdir(ptspath, PTSMODE) < 0 && errno != EEXIST) {
				ERRSYS_FATALPRINT("Fail to create directory %s: %s\n", PTSNAME_DIR, strerror(errno));
				goto err4;
			}

		}
		else {
			goto err1;
		}
	}

	return tcluster;

err4:
	if (pthread_rwlock_destroy(&(tcluster->stm_rwlock)) < 0) {
		ERRSYS_FATALPRINT("Fail to destroy task cluster rwlock\n");
	}
err3:
	if (pthread_rwlockattr_destroy(&(tcluster->rwlock_attr)) < 0) {
		ERRSYS_FATALPRINT("Fail to destroy task cluster rwlock attribute\n");
	}
err2:
	free(tcluster->task);
err1:
	free(tcluster);
	return NULL;
}

void taskcluster_release(struct tasks_cluster* tcluster)
{
	if (pthread_rwlock_destroy(&(tcluster->stm_rwlock)) < 0) {
		ERRSYS_FATALPRINT("Fail to destroy task cluster rwlock\n");
	}
	if (pthread_rwlockattr_destroy(&(tcluster->rwlock_attr)) < 0) {
		ERRSYS_FATALPRINT("Fail to destroy task cluster rwlock attribute\n");
	}

	free(tcluster->task);
	free(tcluster);
}

