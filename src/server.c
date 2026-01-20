#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>                /* timespec{} for pselect() */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>               /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include 	    <unistd.h>
#include        <signal.h>
#include        <wait.h>
#include        <sys/epoll.h>
#include        "net.h"
#include        "server.h"
#include        "auth.h"
#include        "game.h"

#define DB_FILE "users.csv"
#define LISTENQ 10
#define MAXLINE 1024

#define MAX_CLIENTS 2
#define MAXEVENTS 20

GameRoom rooms[50];



int main(int argc, char **argv)
{
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
    ev.events = EPOLLIN; // Czekaj na dane (połączenia)
    ev.data.fd = listenfd;
    
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1){
        fprintf(stderr,"listen error : %s\n", strerror(errno));
        return 1;
	}

	fprintf(stderr,"Waiting for clients ... \n");
	for ( ; ; ) {
		// len = sizeof(cliaddr);
        // 	if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0){
        //         	fprintf(stderr,"accept error : %s\n", strerror(errno));
        //         	continue;
        // 	}

		// bzero(str, sizeof(str));
	   	// inet_ntop(AF_INET, (struct sockaddr  *) &cliaddr.sin_addr,  str, sizeof(str));
		// printf("Connection from %s  \n", str);
        
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
	            } else {
                    printf("Client connected on socket %d", connfd);
                    assign_to_room(connfd);
                }
            } else {
                if (handle_client_message(currfd) == -1) {
                    cleanup_socket(currfd, epollfd);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, currfd, NULL);
                }
            }
        }



       
    }

    return 0;
}
