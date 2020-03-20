#ifndef __L0003_0_H__
#define __L0003_0_H__

#include <unistd.h>
/*
 * Framework library
 */

/* exclusive state */
#define TASK_IDLE					0x0
#define TASK_PREP					0x1
#define TASK_RUNNING				0x2
#define TASK_ABORT					0x3
#define TASK_MASK					0xf

#define CLUSTERNAME_MAX				32

#define IS_TASK_RUNNING(TASK)				(((TASK)->flags & TASK_MASK) == TASK_RUNNING)
#define IS_TASK_PREP(TASK)					(((TASK)->flags & TASK_MASK) == TASK_PREP)
#define IS_TASK_ABORT(TASK)					(((TASK)->flags & TASK_MASK) == TASK_ABORT)
#define WRITE_TASK_FLAGS(TASK, FLAGS)		((TASK)->flags = (FLAGS))
#define SET_TASK_FLAGS(TASK, FLAGS)			((TASK)->flags |= (FLAGS))
#define UNSET_TASK_FLAGS(TASK, FLAGS)		((TASK)->flags &= ~(FLAGS))

#define FULLNAME_MAX						64

/* some critical error occured, need re-do, but not in a hurry way */
#define TAKEABREATH()							usleep(1000000);
#define TAKEASHORTBREATH()						usleep(500000);
#define HEARTBEATS(TASKINFO)					(TASKINFO)->heartbeats++;
#define STATMACH_WLOCK(TCLUSTER)				pthread_rwlock_wrlock(&(TCLUSTER)->stm_rwlock);
#define STATMACH_RLOCK(TCLUSTER)				pthread_rwlock_rdlock(&(TCLUSTER)->stm_rwlock);
#define STATMACH_UNLOCK(TCLUSTER)				pthread_rwlock_unlock(&(TCLUSTER)->stm_rwlock);

#define LIST_WLOCK(RES)							pthread_rwlock_wrlock(&(RES)->rwlock);
#define LIST_RLOCK(RES)							pthread_rwlock_rdlock(&(RES)->rwlock);
#define LIST_UNLOCK(RES)						pthread_rwlock_unlock(&(RES)->rwlock);

#define TASK_LOCK(TASK)							pthread_mutex_lock(&(TASK)->mutex);
#define TASK_UNLOCK(TASK)						pthread_mutex_unlock(&(TASK)->mutex);

#define GET_TASK(TCLUSTER, TASK_INDEX)			(&((TCLUSTER)->task[TASK_INDEX]))

struct tasks_cluster {
	char name[CLUSTERNAME_MAX];
	struct task_info *task;
	uint32_t tasks_num;
	void *parent;	//point to application instance

	/* process state machine global lock */
	pthread_rwlockattr_t rwlock_attr;
	pthread_rwlock_t stm_rwlock;

};

struct tasks_cluster* taskcluster_create(uint32_t tasks_num, const char* name, void* parent);
void taskcluster_release(struct tasks_cluster* tcluster);

#define L0003_0_VERSION		100
#include "task.h"

#endif
