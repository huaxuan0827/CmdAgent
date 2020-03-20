#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "misc.h"
#include "singleton.h"

#define FULLNAME_MAX						128

static int fd_singleton = -1;

int singleton_initialize(char *lockfile)
{
	char fullname[FULLNAME_MAX];

	memset(fullname, 0, FULLNAME_MAX);
	if (lockfile != NULL) {
		strncpy(fullname, lockfile, FULLNAME_MAX);
	}
	else if (get_exec_fullname(fullname, FULLNAME_MAX) != 0) {
		return -1;
	}
	
	if((fd_singleton = open(fullname, O_RDONLY)) != -1) {
		if(flock(fd_singleton, LOCK_EX | LOCK_NB) == 0){
			return 0;
		}
		close(fd_singleton);
		fd_singleton = -1;
	}

	return -1;
}

void singleton_release(void)
{
	if(fd_singleton != -1) {
		flock(fd_singleton, LOCK_UN);
		close(fd_singleton);
		fd_singleton = -1;
	}
}
