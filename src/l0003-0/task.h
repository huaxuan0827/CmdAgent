#ifndef __TASK_H__
#define __TASK_H__
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <stdint.h>

/* basic state machine, these states are exclusive. Extend state machine must not cover these */
#define TASK_IDLE					0x0
#define TASK_PREP					0x1
#define TASK_RUNNING				0x2
#define TASK_ABORT					0x3
#define TASK_QUIT					0x4

#define TASK_LAST					TASK_QUIT
#define TASK_MASK					0xf

#define IS_TASK_RUNNING(TASK)				(((TASK)->flags & TASK_MASK) == TASK_RUNNING)
#define IS_TASK_PREP(TASK)					(((TASK)->flags & TASK_MASK) == TASK_PREP)
#define IS_TASK_ABORT(TASK)					(((TASK)->flags & TASK_MASK) == TASK_ABORT)
#define IS_TASK_QUIT(TASK)					(((TASK)->flags & TASK_MASK) == TASK_QUIT)
#define WRITE_TASK_FLAGS(TASK, FLAGS)		((TASK)->flags = (FLAGS))
#define SET_TASK_FLAGS(TASK, FLAGS)			((TASK)->flags |= (FLAGS))
#define UNSET_TASK_FLAGS(TASK, FLAGS)		((TASK)->flags &= ~(FLAGS))
#define TASK_WAIT_COND(TASK, COND)		\
	do {\
		struct timespec __ts;\
		pthread_mutex_lock(&(TASK)->mutex);\
		clock_gettime(CLOCK_MONOTONIC, &__ts);\
		while(COND) {\
			__ts.tv_sec ++;\
			if (pthread_cond_timedwait(&(TASK)->cond, &(TASK)->mutex, &__ts) == 0) {\
				break;\
			}\
			/*printf("%s heartbeat\n", (TASK)->name);*/\
			HEARTBEATS((TASK));\
		}\
		pthread_mutex_unlock(&(TASK)->mutex);\
	}while(0);

#define TASK_WAIT_COND2(TASK, COND, MS)		\
	do {\
		struct timespec __ts;\
		pthread_mutex_lock(&(TASK)->mutex);\
		clock_gettime(CLOCK_MONOTONIC, &__ts);\
		while(COND) {\
			__ts.tv_nsec += MS * 1000000UL;\
			while (__ts.tv_nsec >= 1000000000UL) {\
				__ts.tv_nsec -= 1000000000UL;\
				__ts.tv_sec ++;\
			}\
			if (pthread_cond_timedwait(&(TASK)->cond, &(TASK)->mutex, &__ts) == 0) {\
				break;\
			}\
			/*printf("%s heartbeat\n", (TASK)->name);*/\
			HEARTBEATS((TASK));\
		}\
		pthread_mutex_unlock(&(TASK)->mutex);\
	}while(0);


#define TASKNAME_MAX				32
#define PTSNAME_MAX					128
#define TASKSTATMACH_MAX			16
#define PTSNAME_DIR					"/tmp/"
#define PTSMODE						0777

/* private process wrapper */
struct process_info {
	int (*initialize)(struct process_info*);
	void (*release)(struct process_info*);
	void *priv;	//point to xxx_proc
	void *parent;	//point to task_info
	void *root;		//point to tasks_cluster
	void *init_param;		//initialize parameter
};
typedef void (*STATMACH_FUNC)(struct process_info*);
typedef int (*PROC_INIT)(struct process_info*);
typedef void (*PROC_REL)(struct process_info*);

struct task_info
{
	int priority;
	int tid;
	int fd_pts;	//pts console
	
	char name[TASKNAME_MAX];
	char ptsname[PTSNAME_MAX];
	pthread_t taskid;/* tasks id */
	pthread_attr_t attr;/* tasks attr */
	pthread_rwlockattr_t rwlock_attr;
	pthread_rwlock_t rwlock;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_condattr_t cattr;
	int wakeup_early;		//set to 0 in wait, set to 1 in wakeup, this is to walk around the case in which wakeup is called earliear than wait causing waiting forever.

	unsigned long heartbeats;/* heartbeat counter */
	STATMACH_FUNC statmach_func[TASKSTATMACH_MAX];
	struct process_info proc;
	void *parent;	//point to tasks_cluster

	int (*startup)(struct task_info *);
	int (*stop)(struct task_info *);
	int (*initialize)(struct tasks_cluster*, int);
	//void (*release)(struct task_info *);
	void (*release)(struct tasks_cluster*, int);
	void (*wait)(struct task_info *);
	void (*wakeupclear)(struct task_info *);
	void (*wait_cond)(struct task_info *, int(*cond)(struct task_info*));
	void (*wakeup)(struct task_info *);
	void (*heartbeat)(struct task_info *);
	void (*chgstat)(struct task_info *, uint32_t);
	int (*setpriority)(struct task_info *, int);
	uint32_t flags;
};


void task_instantiate(struct tasks_cluster* tcluster, int task_index, const char *name, int priority, PROC_INIT init_func, PROC_REL rel_func);
int task_set_statmach(struct task_info *task, int state, STATMACH_FUNC func);

#endif
