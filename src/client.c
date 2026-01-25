#include        <sys/types.h>
#include        <sys/socket.h>
#include        <sys/time.h>
#include        <netinet/in.h>
#include        <arpa/inet.h>
#include        <errno.h>
#include        <stdio.h>
#include        <string.h>
#include 	    <unistd.h>
#include        <sys/select.h>
#include        "game_client.h"
#include        "net.h"

#define MAX_INPUT 256
#define MAXLINE 1024
#define SA      struct sockaddr


int main(int argc, char **argv)
{
    int					sockfd, err;
	struct sockaddr_in	servaddr;
    char msg[1024];
    
    fd_set readfds;
    int max_sd;
    
    if (argc != 2){
		fprintf(stderr, "ERROR: usage: %s <IPaddress>\n", argv[0]);
		return 1;
    }

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(8080);
	if ( (err=inet_pton(AF_INET, argv[1], &servaddr.sin_addr)) <= 0){
		fprintf(stderr,"inet_pton error for %s\n", argv[1]);
		return 1;
    }

	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"connect error : %s \n", strerror(errno));
		return 1;
    }

    printf("Connected to server.\n");
    print_help();

    int game_started = 0;

    
    while(1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);
        max_sd = sockfd;

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        
        if (FD_ISSET(sockfd, &readfds)) {
            if (handle_server_message(sockfd, &game_started) == -1) {
                break;
            }
        }

        
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char input_line[MAX_INPUT];
            
            
            if (!get_input(input_line, sizeof(input_line))) {
                break; 
            }
            
            if (strlen(input_line) == 0) continue;

            if (game_started) {
                if (strlen(input_line) == 1) {
                    Guess g;
                    g.letter = input_line[0];
                    send_packet(sockfd, MSG_GUESS, &g, sizeof(g));
                } else {
                    Word w;
                    strncpy(w.word, input_line, 31);
                    w.word[31] = '\0';
                    send_packet(sockfd, MSG_WORD, &w, sizeof(w));
                }
            } 
            else {
                char command[32], arg1[32], arg2[32];
                int parsed = sscanf(input_line, "%31s %31s %31s", command, arg1, arg2);

                if (parsed < 1) continue;

                memset(msg, 0, sizeof(msg));

                if (strcmp(command, "login") == 0) {
                    if (parsed < 3) { printf("Usage: login <user> <password>\n"); continue; }
                    Login *p = (Login*)msg;
                    strncpy(p->username, arg1, 31);
                    strncpy(p->password, arg2, 31);
                    send_packet(sockfd, MSG_LOGIN, msg, sizeof(Login));
                }
                else if (strcmp(command, "register") == 0) {
                    if (parsed < 3) { printf("Usage: register <user> <password>\n"); continue; }
                    Login *p = (Login*)msg;
                    strncpy(p->username, arg1, 31);
                    strncpy(p->password, arg2, 31);
                    send_packet(sockfd, MSG_REGISTER, msg, sizeof(Login));
                }
                else if (strcmp(command, "exit") == 0) {
                    send_packet(sockfd, MSG_EXIT, NULL, 0);
                    break;
                }
                else if (strcmp(command, "help") == 0) {
                    print_help();
                }
                else {
                    printf("Unknown command (type 'help' to see available commands).\n");
                }
            }
        }
    }

    close(sockfd);
    return 0;
}