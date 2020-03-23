#include <stdlib.h>  
#include <stdio.h>  
#include <stddef.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <errno.h>  
#include <string.h>  
#include <unistd.h>  
#include <ctype.h>   
#include "stdint.h"

#include "l0001-0/l0001-0.h"
#include "l0002-0/l0002-0.h"
#include "l0003-0/l0003-0.h"

#include "devnet.h"

void *thread_proc(void *param){
	struct devcom_proc* proc = (struct devcom_proc *)param;
	devnet_loop(&proc->dev_net);

	return 0;
}

int dealpacket(void* param, uint8_t* data, uint32_t len, uint32_t seqno){
	struct devcom_proc* proc = (struct devcom_proc *)param;	

	printf("Recv Cmd seqno:%d, len:%d \n", seqno, len);
	return 0;
}

int main(int argc, char *argv[])  
{ 
    printf("devnet_test \n"); 
	char ipaddr[32];
	unsigned short usport;

	stcpy(ipaddr, argv[1]);
	usport = atoi(argv[2]);
	
	if(errsys_initialize() < 0) {
		return 1;
	}

	struct devcom_proc *dev_proc;
	struct devnet_op dev_op;
	
	if((dev_proc = (struct devcom_proc*)zmalloc(sizeof(struct devcom_proc))) == NULL) {
		ERRSYS_FATALPRINT("Fail to allocate devcom task\n");		return 1;
	}
	dev_proc->dev_idx = 0;
	dev_proc->rack_index = 0;
	dev_op.dealpacket = dealpacket;
	
	if( devnet_initialize(&dev_proc->dev_net, ipaddr, usport, dev_proc, &dev_op) < 0){
		return 1;
	}
	devnet_connect(&dev_proc->dev_net);

	pthread_t tid;
	pthread_create(&tid, NULL, (void *)thread_proc, (void*)dev_proc);

	char szData[1024] = {0};
	struct cmd_packet *packet = (struct cmd_packet *)szData;
	char seqno = 0;
	
	while(1){
		sleep(5);
		
		packet->cmd = 0xFF;
		packet->seqno = seqno++;
		packet->f.length = 8+ seqno%10;
		devnet_write(&dev_proc->dev_net, szData, packet.f.length);
		printf("Send Cmd seqno:%d, length:%d \n", packet->seqno, packet->f.length);
	}

	devnet_release(&dev_proc->dev_net);
    return 0;  
}

