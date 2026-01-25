#include        <sys/types.h>   
#include        <sys/socket.h>  
#include        <time.h>                
#include        <netinet/in.h>  
#include        <arpa/inet.h>   
#include        <errno.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include 	    <unistd.h>
#include        <sys/epoll.h>
#include        "net.h"
#include        "auth.h"
#include        "game_server.h"

#define DB_FILE "users.csv"
#define LISTENQ 10
#define MAXLINE 1024

#define MAX_CLIENTS 2
#define MAXEVENTS 20

GameRoom rooms[50];



int main(int argc, char **argv)
{
    init_server();
	int				listenfd, connfd;
    char				buff[MAXLINE], str[INET_ADDRSTRLEN+1];
	socklen_t			len;
	struct sockaddr_in	servaddr, cliaddr;
    int epollfd, eventstriggered, currfd;
	struct epoll_event events[MAXEVENTS];
	struct epoll_event ev;

    if((epollfd = epoll_create(MAXEVENTS)) == -1){ 	
          fprintf(stderr,"epoll_create() error : %s\n", strerror(errno));
          return -1;
    }
    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            fprintf(stderr,"socket error : %s\n", strerror(errno));
            return 1;
    }
    int on = 1;               
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
        fprintf(stderr,"SO_REUSEADDR setsockopt error : %s\n", strerror(errno));
    }
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr   = htonl(INADDR_ANY);
	servaddr.sin_port   = htons(8080);	

    if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
            fprintf(stderr,"bind error : %s\n", strerror(errno));
            return 1;
    }

    if ( listen(listenfd, LISTENQ) < 0){
            fprintf(stderr,"listen error : %s\n", strerror(errno));
            return 1;
    }
    ev.events = EPOLLIN; 
    ev.data.fd = listenfd;
    
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1){
        fprintf(stderr,"listen error : %s\n", strerror(errno));
        return 1;
	}

	fprintf(stderr,"Waiting for clients ... \n");
	for ( ; ; ) {
		
        int n = epoll_wait(epollfd, events, MAXEVENTS, -1);
        for(int i=0; i<n; i++) {
            int currfd = events[i].data.fd;

            if (currfd == listenfd) {
                int connfd = accept(listenfd, NULL, NULL);
                ev.events = EPOLLIN;
                ev.data.fd = connfd;
        
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1){
                    fprintf(stderr,"listen error : %s\n", strerror(errno));
                    return 1;
	             }
                else {
                    printf("Client connected on socket %d\n", connfd);
                    // assign_to_room(connfd);
                }
            } else {
                if (handle_client_message(currfd) == -1) {
                    cleanup_socket(currfd, epollfd);
                }
            }
        }



       
    }

    return 0;
}
