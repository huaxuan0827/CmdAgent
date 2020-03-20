#include "filelogger.h"

static int fd_filelogger = 0;
#define FILELOGGER_DEFAULT_PATH		"./file.log"

int filelogOpen(struct errsys_operation* op)
{
	char *logpath = op->init_param;
	*((int*)op->fdout) = open(logpath, O_CREAT | O_RDWR,S_IWUSR);
	if(*((int*)op->fdout) < 0) {
		perror("Fail to open filelog");
		return -1;
	}

	return 0;
}

void filelogClose(struct errsys_operation* op)
{
	close(*((int*)op->fdout));
	*((int*)op->fdout) = 0;
}

void filelogWrite(void* fd,char* buf)
{
	write(*((int*)fd),buf,strlen(buf));
}

struct errsys_operation filelog_errsys_op = {
	.init_param = FILELOGGER_DEFAULT_PATH,/* file path */
	.rel_param = NULL,
	.level = ERRSYS_LEVEL_DEBUG,
	.fdout = &fd_filelogger,
	.errsys_init = (ERRSYS_INIT)filelogOpen,
	.errsys_rel = (ERRSYS_REL)filelogClose,
	.errsys_output = (ERRSYS_OUTPUT)filelogWrite,
};

