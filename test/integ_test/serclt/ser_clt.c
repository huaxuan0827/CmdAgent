#include <stdlib.h>  
#include <stdio.h>  
#include <stddef.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <errno.h>  
#include <string.h>  
#include <unistd.h>  

#include "cmddef.h"

#define MAXLINE 80  
 
char *client_path = "/tmp/agent.clt_a0001-0";  
char *server_path = "/tmp/agent.sercom";

struct agent_req{
	struct agent_packet agent_head;
	struct cmd_packet 	cmd_head;
}__attribute__((packed));


int main(int argc, char** argv) {  
    struct  sockaddr_un cliun, serun;  
    int len;  
    char buf[100];  
	char buf2[100];  
    int sockfd, n;  
	unsigned short usport;
	
	if( argc != 2){
		printf("argc != 2, please input port!");
		return -1;
	}

	usport = atoi(argv[1]);
	
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){  
        perror("client socket error");  
        exit(1);  
    }  
      
#if 0
	// 一般显式调用bind函数，以便服务器区分不同客户端  
    memset(&cliun, 0, sizeof(cliun));  
    cliun.sun_family = AF_UNIX;  
    strcpy(cliun.sun_path, client_path);  
    len = offsetof(struct sockaddr_un, sun_path) + strlen(cliun.sun_path);  
    unlink(cliun.sun_path);  
    if (bind(sockfd, (struct sockaddr *)&cliun, len) < 0) {  
        perror("bind error");  
        exit(1);  
    }  
 #endif
 
    memset(&serun, 0, sizeof(serun));  
    serun.sun_family = AF_UNIX;  
    strcpy(serun.sun_path, server_path);  
    len = offsetof(struct sockaddr_un, sun_path) + strlen(serun.sun_path);  
    if (connect(sockfd, (struct sockaddr *)&serun, len) < 0){  
        perror("connect error");  
        exit(1);  
    }  

	struct agent_req *req;
	struct cmd_packet *resp;

	req = (struct agent_req *)buf;
	req->agent_head.magic = AGENT_INIT_PACKET_HEAD_MAGIC;
	req->agent_head.devip[0] = 192;
	req->agent_head.devip[1] = 168;
	req->agent_head.devip[2] = 11;
	req->agent_head.devip[3] = 128;
	req->agent_head.port = usport;
	req->agent_head.cltid = 1;

	write(sockfd, &req->agent_head, sizeof(struct agent_packet));  
	printf("send targetinfo, usport:%d \n",usport);

	uint8_t seqno = 12;
	while(1){
		 sleep(10);

		 req->cmd_head.seqno = seqno++;
		 req->cmd_head.f.length = 12;
		 req->cmd_head.cmd = 0x12;
		 
         write(sockfd, &req->cmd_head, req->cmd_head.f.length);  
		 printf("send data seqno:%d, usport:%d \n", req->cmd_head.seqno, usport);
		 
		 sleep(1);
		 
         n = read(sockfd, buf2, MAXLINE);  
		 resp = (struct cmd_packet*)buf2;
		 printf("recv data n=%d, cmd:%d, seqno:%d \n", n,resp->cmd, resp->seqno);             
    }   
    close(sockfd);  
    return 0;  
}

