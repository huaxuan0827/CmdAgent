#include "syslogger.h"

int syslogOpen(struct errsys_operation* op)
{
	openlog((char*)op->init_param, LOG_CONS | LOG_PID, 0);

	return 0;
}

void syslogClose(struct errsys_operation* op)
{
	closelog();
}

void syslogWrite(void* fd,char* buf)
{
//	syslog(LOG_ERR | LOG_USER, buf);
	syslog(LOG_INFO | LOG_AUTH , buf);	//log to /var/log/auth.log
	syslog(LOG_INFO | LOG_KERN , buf);	//log to /var/log/syslog
}

struct errsys_operation syslog_errsys_op = {
	.init_param = NULL,/* APP name */
	.rel_param = NULL,
	.level = ERRSYS_LEVEL_DEBUG,
	.fdout = NULL,
	.errsys_init = (ERRSYS_INIT)syslogOpen,
	.errsys_rel = (ERRSYS_REL)syslogClose,
	.errsys_output = (ERRSYS_OUTPUT)syslogWrite,
};

