#include <stdlib.h>  
#include <stdio.h>  
#include <stddef.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <errno.h>  
#include <string.h>  
#include <unistd.h>  
#include <ctype.h>   
#include <stdint.h>
#include <string.h>

#include "l0001-0/l0001-0.h"
#include "l0002-0/l0002-0.h"
#include "l0003-0/l0003-0.h"
#include "l0006-0/l0006-0.h"

#include "devnet.h"
#include "devcom.h"
#include "cmddef.h"

#define MODNAME	 "[devnet_ut]"

void *thread_proc(void *param){
	struct devcom_proc* proc = (struct devcom_proc *)param;

	char szData[1024] = {0};
	struct cmd_packet *packet = (struct cmd_packet *)szData;
	unsigned char seqno = 0;

	printf("thread_proc begin!!! \n");
	while(1){
		usleep(10000);
		packet->cmd = 0xFF;
		seqno++;
		if( seqno >= 255)
			seqno = 0;
		packet->seqno = seqno;
		packet->f.length = 8 + seqno%10;
		if( seqno == 0)
			packet->f.length = 2047;

		if( devnet_isconnected(&proc->dev_net)){
			devnet_write(&proc->dev_net, 12,packet->seqno, (void *)szData, packet->f.length);	
			printf("Send Cmd seqno:%d, length:%d \n", packet->seqno, packet->f.length);
		}
	}
	printf("thread_proc exit!!!!!!!1 \n");
	return 0;
}

int dealpacket(void *param,int serid, uint8_t seqno, void *data, int len){
	struct devcom_proc* proc = (struct devcom_proc *)param;	

	static int scount = 0;
	//if( scount++ % 10 == 0)
	printf("Recv Cmd serid:%d, seqno:%d, len:%d error_count:%d\n",serid, seqno, len, proc->dev_net.error_count);
	return 0;
}

int main(int argc, char *argv[])  
{ 
    printf("netcom_ut test!!!\n"); 

	//	errsys_features(ERRSYS_FEATURE_PRINT, ERRSYS_LEVEL_DEBUG, NULL);
	errsys_features(ERRSYS_FEATURE_SYSLOG, ERRSYS_LEVEL_INFO, NULL);
	if(errsys_initialize() < 0) {
		return 1;
	}
	ERRSYS_INFOPRINT("init netcom!!! \n");


	struct l0006_netcom_info netcom;
	struct devnet_op dev_op;
	
	if((dev_proc = (struct devcom_proc*)zmalloc(sizeof(struct devcom_proc))) == NULL) {
		ERRSYS_FATALPRINT("Fail to allocate devcom task\n");		
		return 1;
	}
	dev_proc->dev_idx = 0;
	dev_proc->rack_index = 0;
	dev_op.dealpacket = dealpacket;
	dev_op.param = (void *)dev_proc;
	
	if( devnet_initialize(&dev_proc->dev_net, ipaddr, usport, &dev_op) < 0){
		return 1;
	}	

	pthread_t tid;
	pthread_create(&tid, NULL, thread_proc, dev_proc);

	while(1){
		time_t tbegin, tend;
		ERRSYS_INFOPRINT("#### DEVCOM PREP #####\n");
		int nret = devnet_connect(&dev_proc->dev_net);
		printf("devnet_connet ret:%d \n", nret);

		ERRSYS_INFOPRINT("#### DEVCOM RUNNING #####\n");
		devnet_loop(&dev_proc->dev_net);
		ERRSYS_WARNPRINT("%s:%d devnet_loop break \n", dev_proc->dev_net.ipaddr, dev_proc->dev_net.usport);
		devnet_disconnect(&dev_proc->dev_net);	
		sleep(2);
	}
	
	devnet_release(&dev_proc->dev_net);
    return 0;  
}

