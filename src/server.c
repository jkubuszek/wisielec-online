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
#include    "protocol.h"
#include    "server.h"

#define LISTENQ 10
#define MAXLINE 1024

void sig_chld(int signo){
	pid_t	pid;
	int		stat;
	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);
	return;
}

void sig_pipe(int signo){
	printf("Server received SIGPIPE - Default action is exit \n");
	exit(1);
}

int authenticate_player(const char username, const char password){

    FILE *file = fopen("users.csv", "r");
    
    if (file == NULL) {
        perror("ERROR: couldn't open/find users.csv");
        return 0; 
    }

    char file_user[MAX_LEN];
    char file_pass[MAX_LEN];
    int auth_success = 0; 

    while (fscanf(file, "%31s;%31s", file_user, file_pass) == 2) {
        if (strcmp(username, file_user) == 0 && strcmp(password, file_pass) == 0) {
            auth_success = 1;
            break; 
        }
    }

    fclose(file);
    return auth_success;
}

int process_guess(GameSession session, char letter){

}

void handle_client(int connfd) {
}

int main(int argc, char **argv)
{
	int				listenfd, connfd;
    char				buff[MAXLINE], str[INET_ADDRSTRLEN+1];
	socklen_t			len;
	struct sockaddr_in	servaddr, cliaddr;


        if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                fprintf(stderr,"socket error : %s\n", strerror(errno));
                return 1;
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

    signal(SIGCHLD, sig_chld);
	signal(SIGPIPE, sig_pipe);

	fprintf(stderr,"Waiting for clients ... \n");
	for ( ; ; ) {
		len = sizeof(cliaddr);
        	if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0){
                	fprintf(stderr,"accept error : %s\n", strerror(errno));
                	continue;
        	}

		bzero(str, sizeof(str));
	   	inet_ntop(AF_INET, (struct sockaddr  *) &cliaddr.sin_addr,  str, sizeof(str));
		printf("Connection from %s\n", str);

        pid_t pid = fork();

        if (pid == -1) {
            perror("ERROR: fork error");
            close(connfd);
        } 
        else if (pid == 0) {
            close(listenfd);
            handle_client(connfd);
            close(connfd);
            exit(0); 
        } 
        else {
            close(connfd);
        }
    }

    return 0;
}
