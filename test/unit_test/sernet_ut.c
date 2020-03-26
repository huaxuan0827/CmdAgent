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

#include "sernet.h"
#include "sercom.h"
#include "cmddef.h"

#define MODNAME	 "[sernet_ut]"


int sernet_dealdata(void *param, const char *szdevip, unsigned short usport, int serid, int seqno, void *data, int len){
	struct sernet_info *ser_net = param;
	
	printf("recv data ip:%s, port:%d, len:%d, serid:%d, seqno:%d\n",szdevip, usport, len, serid, seqno);
	sernet_write(ser_net, data, len, serid, seqno);
	return 0;
}

int main(int argc, char *argv[])  
{ 
    printf("sernet_ut test!!!\n");
	evthread_use_pthreads();//enable threads 

	errsys_features(ERRSYS_FEATURE_SYSLOG, ERRSYS_LEVEL_INFO, NULL);
	if(errsys_initialize() < 0) {
		return 1;
	}
	ERRSYS_INFOPRINT("init sernet!!! \n");

	struct sernet_info *ser_net;
	struct serclt_op ser_op;	
		ser_net = (struct sernet_info*)zmalloc(sizeof(struct sernet_info));
	

	ser_op.transmitpacket = sernet_dealdata;

	ser_op.param = ser_net;

	
	if( sernet_initialize(ser_net, SERCOM_NET_PATH ,&ser_op) < 0){
		printf("sernet init failed \n");
		return 0;
	}

	sernet_loop(ser_net);
	
    return 0;  
}

