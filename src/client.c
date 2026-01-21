#include        <sys/types.h>
#include        <sys/socket.h>
#include        <sys/time.h>
#include        <netinet/in.h>
#include        <arpa/inet.h>
#include        <errno.h>
#include        <fcntl.h>
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include 	    <unistd.h>
#include        <sys/select.h>
#include        "server.h"
#include        "game.h"
#include        "net.h"

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

// Zmieniono typ zwracany na int (1 - sukces, 0 - błąd/EOF)
int get_input(char *buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        return 1;
    }
    return 0;
}

int handle_server_message(int sockfd, int *game_started) {
    char response[1024];
    int type = recv_packet(sockfd, response, sizeof(response));
    
    if (type <= 0) {
        printf("Server disconnected or error.\n");
        return -1;
    }

    switch(type){
        case MSG_TEXT:
            printf("server: %s\n", response);
            break;
        
        case MSG_GAME_START:
            *game_started = 1;
            break;

        case MSG_GAME_STATE: 
            GameState *state = (GameState*)response;
            system("clear");
            printf("\n--- GAME STATUS ---\n");
            printf("Secret word: %s\n", state->word_mask);
            printf("Lives left: %d / %d\n", state->lives, state->max_lives);
            printf("-------------------\n");
            
            // if (state->lives > 0) {
            //     printf("Guess: ");
            // }
            break;
            
        case MSG_PROMPT:
            break;
    }
    return 0;
}

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