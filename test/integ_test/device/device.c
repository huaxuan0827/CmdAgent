#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
#define MAXBUFFER 10240

int main(int argc, char** argv)
{
    int serverFd, connfd,nread;
    socklen_t len;
    struct sockaddr_in serveraddr,clientaddr;
    char readBuf[MAXBUFFER]={0};
    char ipaddr[32];
	unsigned short usport;

	if(argc < 3){
		printf("please input param: ipaddr and port!!!\n");
		exit(-1);
	}
	strcpy(ipaddr, argv[1]);
	usport = atoi(argv[2]);
	
    serverFd=socket(AF_INET,SOCK_STREAM,0);//创建socket
    if(serverFd < 0){
		printf("socket error:%s\n",strerror(errno));
        exit(-1);
    }
    bzero(&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(usport);
    inet_pton(AF_INET,ipaddr,&serveraddr.sin_addr);//将c语言字节序转换为网络字节序
    int ret=bind(serverFd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));//绑定IP和端口
    if(ret!=0){
        close(serverFd);
        printf("bind error:%s\n",strerror(errno));
        exit(-1);
    }
    ret=listen(serverFd,5);//监听
    if(ret!=0)
    {
       close(serverFd);
       printf("listen error:%s\n",strerror(errno));
       exit(-1);
    }
    len=sizeof(clientaddr);
    bzero(&clientaddr,sizeof(clientaddr));
    while (1)
    {
        connfd = accept(serverFd, (struct sockaddr *) &clientaddr, &len);//接受客户端的连接
        printf("%s 连接到服务器 \n",inet_ntop(AF_INET,&clientaddr.sin_addr,ipaddr,sizeof(ipaddr)));
        if (serverFd < 0)
        {
            printf("accept error : %s\n", strerror(errno));
            continue;
        }
        while((nread= read(connfd,readBuf,MAXBUFFER)))//读客户端发送的数据
        {
        	//sleep(1);
			static int nCount = 0;
			nCount++;
			/*if( nCount% 5 == 0){
				for( unsigned i = 0; i < nread; i++){
					readBuf[i] = 0xFF;
				}
			}*/
            write(connfd,readBuf,nread);//写回客户端
            //printf("ncount:%d, read:%d \n",nCount, nread);
            bzero(readBuf,MAXBUFFER);
        }

		if(nread==0){
            printf("客户端关闭连接\n");         
        }else{
            printf("read error:%s\n",strerror(errno));
        }
        close(connfd);
    }
    close(serverFd);
    return 0;
}