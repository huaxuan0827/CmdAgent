#include "printlogger.h"
#include <unistd.h>
#include <string.h>

/* Info Log */
static int printlogOpen(void*dummy)
{
//	printf("%s\n",__func__);
	setvbuf(stderr, NULL, _IONBF, 0);
	return 0;
}

static void printlog(void* fd,char*buf)
{
	fprintf(*((FILE**)fd),buf,strlen(buf));
}

struct errsys_operation printlog_errsys_op = {
	.init_param = NULL,
	.rel_param = NULL,
	.level = ERRSYS_LEVEL_DEBUG,
	.fdout = &stderr,
	.errsys_init = printlogOpen,
	.errsys_rel = NULL,
	.errsys_output = (ERRSYS_OUTPUT)printlog,
};

