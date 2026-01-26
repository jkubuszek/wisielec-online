#define _GNU_SOURCE
#include        <sys/types.h>   // time functions
#include        <sys/socket.h>  // sockets
#include        <netinet/in.h>  // sockaddr, inet
#include        <arpa/inet.h>   // htons, htonl, inet_
#include        <errno.h> // errno
#include        <stdio.h> // perror for daemon
#include        <string.h> // str...
#include 	    <unistd.h> // daemon, close
#include        <sys/epoll.h> // epoll
#include        <syslog.h> // syslog
#include        <sys/stat.h> // umask
#include        "net.h"
#include        "auth.h"
#include        "server_utils.h"

#define DB_FILE "users.csv"
#define LISTENQ 10
#define MAXLINE 1024

#define MAX_CLIENTS 2
#define MAXEVENTS 20

GameRoom rooms[50];

int main(int argc, char **argv){
    if (daemon(1, 0) == -1) {
        perror("daemon");
        return 1;
    }
    umask(077);

    openlog("wisielec-server", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Server started");

    init_server();
    
	int				listenfd, connfd, mcastfd;
    char				buff[MAXLINE], str[INET_ADDRSTRLEN+1];
	socklen_t			len;
	struct sockaddr_in	servaddr, cliaddr;
    int epollfd, eventstriggered, currfd;
	struct epoll_event events[MAXEVENTS];
	struct epoll_event ev;

    if((epollfd = epoll_create(MAXEVENTS)) == -1){ 	
        syslog(LOG_ERR, "epoll_create() error : %s\n", strerror(errno));
        return -1;
    }
    // unicast socket
    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        syslog(LOG_ERR, "socket() error : %s\n", strerror(errno));
        return 1;
    }

    int on = 1;               
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
        syslog(LOG_ERR, "SO_REUSEADDR setsockopt error : %s\n", strerror(errno));
    }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr   = htonl(INADDR_ANY);
	servaddr.sin_port   = htons(8080);	

    if (bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        syslog(LOG_ERR, "bind() error : %s\n", strerror(errno));
        return 1;
    } 

    if (listen(listenfd, LISTENQ) < 0){
        syslog(LOG_ERR, "listen() error : %s\n", strerror(errno));
        return 1;
    }
    
    // multicast socket
    if((mcastfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        syslog(LOG_ERR, "socket() error : %s\n", strerror(errno));
        return 1;
    }

    if (setsockopt(mcastfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
        syslog(LOG_ERR, "SO_REUSEADDR setsockopt error : %s\n", strerror(errno));
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    addr.sin_port = htons(MULTICAST_PORT);

    if (bind(mcastfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        syslog(LOG_ERR, "multicast bind() error : %s\n", strerror(errno));
        close(mcastfd);
    }
    
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_IP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(mcastfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        syslog(LOG_ERR, "IP_ADD_MEMBERSHIP setsockopt error : %s\n", strerror(errno));
        close(mcastfd);
    }

    // add listenfd to epoll
    ev.events = EPOLLIN; 
    ev.data.fd = listenfd;
    
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1){
        syslog(LOG_ERR, "epoll_ctl() error : %s\n", strerror(errno));
        return 1;
	}

    // add mcastfd to epoll
    ev.events = EPOLLIN; 
    ev.data.fd = mcastfd;
    
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, mcastfd, &ev) == -1){
        syslog(LOG_ERR, "epoll_ctl() error : %s\n", strerror(errno));
        return 1;
	}

    syslog(LOG_INFO, "Waiting for clients ... \n");
	for ( ; ; ) {
		
        int n = epoll_wait(epollfd, events, MAXEVENTS, -1);
        for(int i=0; i<n; i++) {
            int currfd = events[i].data.fd;

            if (currfd == listenfd) {
                int connfd = accept(listenfd, NULL, NULL);
                ev.events = EPOLLIN;
                ev.data.fd = connfd;
        
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1){
                    syslog(LOG_ERR, "listen error : %s\n", strerror(errno));
                    return 1;
	             }
                else {
                    syslog(LOG_INFO, "Client connected on socket %d\n", connfd);
                    // assign_to_room(connfd);
                }
            } else if (currfd == mcastfd) {
                char buf[256];
                struct sockaddr_in client_addr;
                socklen_t addrlen = sizeof(client_addr);
                
                int n = recvfrom(mcastfd, buf, sizeof(buf), 0, (struct sockaddr*)&client_addr, &addrlen);
                if (n > 0) {
                    buf[n] = 0;
                    if (strstr(buf, DISCOVERY_MSG)) {
                        syslog(LOG_INFO, "Received discovery request from %s\n", inet_ntoa(client_addr.sin_addr));
                        sendto(mcastfd, DISCOVERY_RESPONSE, strlen(DISCOVERY_RESPONSE), 0, (struct sockaddr*)&client_addr, addrlen);
                    }
                }
            } else {
                if (handle_client_message(currfd) == -1) {
                    cleanup_socket(currfd, epollfd);
                }
            }
        }
    }
    closelog();
    return 0;
}
