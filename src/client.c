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
#include        "protocol.h"
#include        "server.h"

#define MAX_INPUT 256
#define MAXLINE 1024
#define SA      struct sockaddr

void print_help() {
    printf("\nAvailable actions:\n");
    printf("  login <username> <password>    - Log in to the game\n");
    printf("  register <username> <password> - Create an account\n");
    printf("  exit                           - Exit the game\n");
    printf("  help                           - Display this help page\n");
}

int
main(int argc, char **argv)
{
	int					sockfd, n;
	struct sockaddr_in	servaddr;
	char				recvline[MAXLINE + 1];
	int err;
    GameMessage msg, response;
    char input_line[MAX_INPUT];
    char command[32];
    char arg1[32];
    char arg2[32];

	if (argc != 2){
		fprintf(stderr, "ERROR: usage: a.out <IPaddress>  \n");
		return 1;
	}
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(8080);	/* daytime server */
	if ( (err=inet_pton(AF_INET, argv[1], &servaddr.sin_addr)) <= 0){
		if(err == 0 )
			fprintf(stderr,"inet_pton error for %s \n", argv[1] );
		else
			fprintf(stderr,"inet_pton error for %s : %s \n", argv[1], strerror(errno));
		return 1;
	}
	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"connect error : %s \n", strerror(errno));
		return 1;
	}
    printf("Connected to server.\n");
    print_help();


    while(1){

        //getting user's input
        if (!fgets(input_line, sizeof(input_line), stdin)) {
            break;
        }
        int parsed = sscanf(input_line, "%31s %31s %31s", command, arg1, arg2);

        if (parsed < 1) continue; //empty line, ignore

        memset(&msg, 0, sizeof(msg)); //erase message struct

        if (strcmp(command, "login") == 0) {
            if (parsed < 3) {
                printf("Error: usage: login <username> <password>\n");
                continue;
            }
            msg.type = MSG_LOGIN;
            strncpy(msg.username, arg1, 31);
            strncpy(msg.password, arg2, 31);
        }
        else if (strcmp(command, "register") == 0) {
            if (parsed < 3) {
                printf("Error: usage: register <username> <password>\n");
                continue;
            }
            msg.type = MSG_REGISTER;
            strncpy(msg.username, arg1, 31);
            strncpy(msg.password, arg2, 31);
        }
        else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            msg.type = MSG_EXIT;
            send(sockfd, &msg, sizeof(msg), 0);
            break; 
        }
        else if (strcmp(command, "help") == 0) {
            print_help();
            continue; 
        }
        else {
            printf("Unknown command: %s. Type 'help' to see all available commands.\n", command);
            continue;
        }
        
        if (send(sockfd, &msg, sizeof(msg), 0) == -1) {
            perror("Send failed");
            break;
        }

        if (recv(sockfd, &response, sizeof(response), 0) > 0) {
            if (response.type == MSG_RESPONSE) {
                printf("[OK] %s\n", response.message_text);                
            } else if (response.type == MSG_ERROR) {
                printf("[ERROR] %s\n", response.message_text);
            }
        } else {
            printf("Server disconnected.\n");
            break;
        }
        
    }
    close(sockfd);
    exit(0);
}
