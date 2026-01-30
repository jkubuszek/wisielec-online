#define _GNU_SOURCE
#include        <sys/types.h> 
#include        <sys/socket.h> // sockets
#include        <sys/time.h> // select, FD
#include        <netinet/in.h> // sockaddr
#include        <arpa/inet.h> // INET_ADDRSTRLEN, inet_ntoa
#include        <errno.h> // errno
#include        <stdio.h> // printf, snprintf
#include        <string.h> // str...
#include 	    <unistd.h> // close, STDIN_FILENO
#include        <sys/select.h> // select, fd_set
#include        <netdb.h> // getaddrinfo, freeaddrinfo
#include        "client_utils.h"
#include        "net.h"

#define MAX_INPUT 256
#define MAXLINE 1024
#define SA      struct sockaddr


int main(int argc, char **argv)
{
    int					sockfd, err;
	struct sockaddr_in	servaddr;
    char msg[1024];
    char server_ip[32];
    
    fd_set readfds;
    int max_sd;
    
    if (argc >= 2) {
        strncpy(server_ip, argv[1], sizeof(server_ip) - 1);
        server_ip[sizeof(server_ip) - 1] = '\0';
    } else {
        if (!find_server(server_ip)) {
            fprintf(stderr, "Could not find server automatically. Usage: %s <IPaddress> <Port (optional)>\n", argv[0]);
            return 1;
        }
    }

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       
    hints.ai_socktype = SOCK_STREAM; 

    char *port_str;
    char default_port[6];

    if (argc == 3) {
        port_str = argv[2];
    } else {
        snprintf(default_port, sizeof(default_port), "%d", PORT);
        port_str = default_port;
    }

    int s = getaddrinfo(server_ip, port_str, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return 1;
    }


    // iterate through all the results from getaddrinfo
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1)
            continue; // address doesn't work, check the next one

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; // break if managed to connect
        

        close(sockfd); 
    }

    freeaddrinfo(result); 

    if (rp == NULL) {               
        fprintf(stderr, "Could not connect to server %s\n", server_ip);
        return 1;
    }

    printf("Connected to server.\n");
    hangman_title();
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
            if (handle_server_message(sockfd, &game_started) != 0) {
                printf("Error: could not read message from server\n");
                break;
            }
        }

        
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char input_line[MAX_INPUT];
            
            
            if (!get_input(input_line, sizeof(input_line))) {
                printf("Error: could not collect the input");
                break; 
            }
            
            if (strlen(input_line) == 0) continue;
            
            if(input_line[0] == '/'){
                char command[32], arg1[32], arg2[32];
                int parsed = sscanf(input_line, "%31s %31s %31s", command, arg1, arg2);
                if (parsed < 1) continue;

                memset(msg, 0, sizeof(msg));

                if (strcmp(command, "/login") == 0) { // to review
                    if (parsed < 3) { printf("Usage: login <user> <password>\n"); continue; }
                    Login *p = (Login*)msg;
                    strncpy(p->username, arg1, 31);
                    strncpy(p->password, arg2, 31);
                    if(send_packet(sockfd, MSG_LOGIN, msg, sizeof(Login)))
                        printf("Error: could not send packet");
                } else if (strcmp(command, "/register") == 0) { // to review
                    if (parsed < 3) { printf("Usage: register <user> <password>\n"); continue; }
                    Login *p = (Login*)msg;
                    strncpy(p->username, arg1, 31);
                    strncpy(p->password, arg2, 31);
                    if(send_packet(sockfd, MSG_REGISTER, msg, sizeof(Login)))
                        printf("Error: could not send packet");
                } else if (strcmp(command, "/help") == 0) {
                    print_help();
                } else if (strcmp(command, "/scoreboard") == 0) {
                    if(send_packet(sockfd, MSG_SCOREBOARD, NULL, 0))
                        printf("Error: could not send packet");
                } else {
                    printf("Unknown command (type '/help' to see available commands).\n");
                }
            } else if (game_started) {
                if (strlen(input_line) == 1) {
                    Guess g;
                    g.letter = input_line[0];
                    if(send_packet(sockfd, MSG_GUESS, &g, sizeof(g)))
                        printf("Error: could not send packet");
                } else {
                    Word w;
                    strncpy(w.word, input_line, 31);
                    w.word[31] = '\0';
                    if(send_packet(sockfd, MSG_WORD, &w, sizeof(w)))
                        printf("Error: could not send packet");
                }
            }
        }
    }

    close(sockfd);
    return 0;
}