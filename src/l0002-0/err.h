#ifndef __EA0125_0_ERR_H__
#define __EA0125_0_ERR_H__

#include <l0001-0/include/list.h>

#define MODNAME_MAX			32

#define ERRSYS_FEATURE_PRINT	1
#define ERRSYS_FEATURE_SYSLOG	2
#define ERRSYS_FEATURE_FILE		3

#define ERRSYS_LEVEL_FATAL   	4
#define ERRSYS_LEVEL_ERROR		3
#define ERRSYS_LEVEL_WARN		2
#define ERRSYS_LEVEL_INFO		1
#define ERRSYS_LEVEL_DEBUG		0

/* MODNAME must be define in each .c */
//#define myprintf(templt,...) fprintf(stderr,templt,__VA_ARGS__)
#define ERRSYS_FATALPRINT(fmt,args...)			errsys_print(__FUNCTION__,__LINE__,ERRSYS_LEVEL_FATAL, MODNAME, "[FATAL]", fmt, ##args)
#define ERRSYS_ERRPRINT(fmt,args...)			errsys_print(__FUNCTION__,__LINE__,ERRSYS_LEVEL_ERROR, MODNAME, "[ERR]", fmt, ##args)
#define ERRSYS_WARNPRINT(fmt,args...)		errsys_print(__FUNCTION__,__LINE__,ERRSYS_LEVEL_WARN, MODNAME, "[WARN]", fmt, ##args)
#define ERRSYS_INFOPRINT(fmt,args...)		errsys_print(__FUNCTION__,__LINE__,ERRSYS_LEVEL_INFO, MODNAME, "[INFO]", fmt, ##args)
#define ERRSYS_DEBUGPRINT(fmt,args...)		errsys_print(__FUNCTION__,__LINE__,ERRSYS_LEVEL_DEBUG, MODNAME, "[DEBUG]", fmt, ##args)

typedef int (*ERRSYS_INIT)(void*);
typedef int (*ERRSYS_REL)(void*);
typedef int (*ERRSYS_OUTPUT)(void* ,char*);

struct errsys_operation
{
	void *fdout;
	int level;
	void *init_param;
	void *rel_param;
	int (*errsys_init)(void*p);
	int (*errsys_rel)(void*p);
	int (*errsys_output)(void* fdout,char*buf);
	
	struct list_head head;
};

typedef struct __errsys_msg
{
	long base;
	unsigned long range;
	char **text;

	struct list_head head;
}errsys_msg;

int errsys_del_op(struct errsys_operation *op);
int errsys_del_msg(errsys_msg *msg);
int errsys_add_op(struct errsys_operation *op);
int errsys_add_msg(errsys_msg *msg);
void errsys_strerr(const char*func,int lineno,int level,int errnum,...);
void errsys_print(const char*func,int lineno,int level, const char* mod, const char* levelstr, const char*fmt, ...);
int errsys_initialize(void);
void errsys_release(void);
void errsys_features(unsigned long cap, int level, const char* filename);

#endif
