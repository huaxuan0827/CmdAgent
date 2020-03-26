#include <signal.h>

#include "cmdagent.h"

static struct cmdagent_info *g_agent = NULL;


/* TODO: illegal instruction & segment fault */
void signal_handler(int signo)
{
	switch(signo){
		case SIGINT:
			printf("SIGINT Catched\n");
			break;
		case SIGQUIT:
			printf("SIGQUIT Catched\n");
			break;
		case SIGUSR1:
			printf("SIGUSR1 Catched\n");
			break;
		case SIGUSR2:
			printf("SIGUSR2 Catched\n");
			break;
		case SIGALRM:
			printf("SIGALRM Catched\n");
			break;
		default:
			break;
	}
	//racksagent_exit(g_monitor);
}

int signal_initialize(struct cmdagent_info *agent_info)
{
	struct sigaction act;
	
	g_agent = agent_info;

	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART | SA_SIGINFO;
	
	if (sigaction(SIGINT, &act, NULL) < 0)
		return -1;
	if (sigaction(SIGQUIT, &act, NULL) < 0)
		return -1;
	if (sigaction(SIGINT, &act, NULL) < 0)
		return -1;
	if (sigaction(SIGUSR1, &act, NULL) < 0)
		return -1;
	/* this makes el0021-0 heartbeat not work */
//	if (sigaction(SIGUSR2, &act, NULL) < 0)
//		return (int)SIG_ERR;
	if (sigaction(SIGALRM, &act, NULL) < 0)
		return -1;
	return 0;
}



