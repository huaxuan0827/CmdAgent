/*
 *  File: ea0125-0.c                         Project: 5782-28
 *                                          Synchronism DTA
 *  Author: Ma Jieyu
 *  Date:   29 July 2011
 *
 *           ((C)) Copyright Wescon Controls (Shanghai) , Inc., 2011.
 *                           All rights reserved.
 *       License material - Property of Wescon Controls (Shanghai) , Inc.
 *
 *  DESCRIPTION:
 *
 *  This is the core of the errlog system, NOT THREAD-SAFE
 *
 *  REVISION HISTORY:
 *
 *  Num:    Modified by:        Date:       Reason:
 *  ----    -----------------   ---------   ---------------------------------
 *  1.0      Ma Jieyu             29 July 11    Created.
 *
 */
#include <stdlib.h>
#include <stdarg.h>
#include "l0002-0.h"

LIST_HEAD(errsys_op_list);

int errsys_del_op(struct errsys_operation *op)
{
	list_del(&(op->head));
	return 0;
}

/* call this before errsys_init */
int errsys_add_op(struct errsys_operation *op)
{
	list_add(&(op->head),&errsys_op_list);
	return 0;
}

static int errsys_init(void)
{
	int rc = 0;
	struct list_head *p;
	struct errsys_operation *op;

	list_for_each(p,&errsys_op_list){/* traverse the op list to do the outputing */
		op = list_entry(p, struct errsys_operation, head);
		if(op->errsys_init && (rc = op->errsys_init(op)) < 0)
			return rc;
	}

	return 0;
}

static int errsys_rel(void)
{
	struct list_head *p;
	struct errsys_operation *op;

	list_for_each(p,&errsys_op_list){/* traverse the op list to do the outputing */
		op = list_entry(p, struct errsys_operation, head);
		if(op->errsys_rel)
			op->errsys_rel(op);
	}
	
	INIT_LIST_HEAD(&errsys_op_list);

	return 0;
}

void errsys_print(const char*func,int lineno,int level, const char* mod, const char* levelstr, const char*fmt, ...)
{
	char buf[256];
	int modlen = 0;
	struct list_head *p;
	struct errsys_operation *op;
	va_list args;

	va_start(args, fmt);
	memset(buf, 0, 256);
	modlen = snprintf(buf, 255, "%s%s", levelstr, mod);
	if (modlen > 0) {
		vsnprintf(buf + modlen, 255 - modlen, fmt, args);
	//	printf("%s(%d):",func,lineno);
		list_for_each(p,&errsys_op_list){/* traverse the op list to do the outputing */
			op = list_entry(p, struct errsys_operation, head);
			if(op->level <= level)
				op->errsys_output(op->fdout,buf);
		}
	}
	va_end(args);
}

void errsys_features(unsigned long cap, int level, const char* filename)
{
	switch (cap) {
		case ERRSYS_FEATURE_PRINT:
			printlog_errsys_op.level = level;
			errsys_add_op(&printlog_errsys_op);
			break;
		case ERRSYS_FEATURE_SYSLOG:
			syslog_errsys_op.level = level;
			errsys_add_op(&syslog_errsys_op);
			break;
		case ERRSYS_FEATURE_FILE:
			filelog_errsys_op.level = level;
			filelog_errsys_op.init_param = (void*)filename;
			errsys_add_op(&filelog_errsys_op);
			break;
		default:
			break;
	}
}

int errsys_initialize(void)
{
	return errsys_init();
}

void errsys_release(void)
{
	errsys_rel();
}

