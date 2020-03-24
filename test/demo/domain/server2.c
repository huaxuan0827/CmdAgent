#include <stdlib.h>  
#include <stdio.h>  
#include <stddef.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <errno.h>  
#include <string.h>  
#include <unistd.h>  
#include <ctype.h>  

#include <netinet/in.h>  
#include <sys/socket.h>  
#include <unistd.h>  
  
#include <stdio.h>  
#include <string.h>  
  
#include <event2/event.h>  
#include <event2/listener.h>  
#include <event2/bufferevent.h>  

 
#define MAXLINE 80  
 
char *socket_path = "server.socket";  
 
void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,  
                 struct sockaddr *sock, int socklen, void *arg);  
void socket_read_cb(struct bufferevent *bev, void *arg);  
void socket_error_cb(struct bufferevent *bev, short events, void *arg);  


 void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,  
                 struct sockaddr *sock, int socklen, void *arg)  
{  
    struct event_base *base = (struct event_base*)arg;  
  
    struct bufferevent *bev =  bufferevent_socket_new(base, fd,  
                                               BEV_OPT_CLOSE_ON_FREE);  
  
    bufferevent_setcb(bev, socket_read_cb, NULL, socket_error_cb, NULL);  
    bufferevent_enable(bev, EV_READ | EV_PERSIST);  
}  
  
  
void socket_read_cb(struct bufferevent *bev, void *arg)  
{  
    char msg[4096];  
  
    size_t len = bufferevent_read(bev, msg, sizeof(msg)-1 );  
  
    msg[len] = '\0';  
    printf("server read the data %s\n", msg);  
  
    char reply[] = "I has read your data";  
    bufferevent_write(bev, reply, strlen(reply) );  
}  
  
  
void socket_error_cb(struct bufferevent *bev, short events, void *arg)  
{  
    if (events & BEV_EVENT_EOF)  
        printf("connection closed\n");  
    else if (events & BEV_EVENT_ERROR)  
        printf("some other error\n");  
  
    //这将自动close套接字和free读写缓冲区  
    bufferevent_free(bev);  
}  

 
int main(void)  
{  
    evthread_use_pthreads();//enable threads 
	
	struct sockaddr_un serun, cliun;  
    socklen_t cliun_len;  
    int listenfd, connfd, size;  
    char buf[MAXLINE];  
    int i, n;  
 
    memset(&serun, 0, sizeof(serun));  
    serun.sun_family = AF_UNIX;  
    strcpy(serun.sun_path, socket_path);  
    size = offsetof(struct sockaddr_un, sun_path) + strlen(serun.sun_path);  
    unlink(socket_path); 
 
    struct event_base *base = event_base_new();  
    struct evconnlistener *listener  =  evconnlistener_new_bind(base, listener_cb, base,  
                                      LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE,  
                                      10, (struct sockaddr *)&serun,  size);  
    event_base_dispatch(base);  
  
    evconnlistener_free(listener);  
    event_base_free(base);  
    return 0;  
}

