#include <stdio.h> // printf, fgets
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // inet_addr, inet_ntoa
#include <sys/time.h> // timeval
#include <string.h> // str, memset
#include "net.h"
#include "client_utils.h"


int find_server(char *server_ip_out) {
    int sock;
    struct sockaddr_in mcast_addr;
    char buffer[256];

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return 0;
    }

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    memset(&mcast_addr, 0, sizeof(mcast_addr));
    mcast_addr.sin_family = AF_INET;
    mcast_addr.sin_addr.s_addr = inet_addr(MULTICAST_IP);
    mcast_addr.sin_port = htons(MULTICAST_PORT);

    printf("Looking for server on %s:%d...\n", MULTICAST_IP, MULTICAST_PORT);
    if (sendto(sock, DISCOVERY_MSG, strlen(DISCOVERY_MSG), 0, 
              (struct sockaddr *)&mcast_addr, sizeof(mcast_addr)) < 0) {
        perror("Sendto failed");
        close(sock);
        return 0;
    }

    struct sockaddr_in server_addr;
    socklen_t len = sizeof(server_addr);
    
    int n = recvfrom(sock, buffer, sizeof(buffer), 0, 
                     (struct sockaddr *)&server_addr, &len);

    if (n > 0) {
        buffer[n] = 0;
        if (strstr(buffer, DISCOVERY_RESPONSE)){
            strcpy(server_ip_out, inet_ntoa(server_addr.sin_addr));
            printf("Found server at: %s\n", server_ip_out);
            close(sock);
            return 1; 
        }
    }

    printf("No server found (timeout).\n");
    close(sock);
    return 0; 
}

void print_help() {
    printf("\nAvailable actions:\n");
    printf("  login <username> <password>    - Log in to the game\n");
    printf("  register <username> <password> - Create an account\n");
    printf("  exit                           - Exit the game\n");
    printf("  help                           - Display this help page\n");
}

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
            printf("letters already used : ");
            for(int i = 0; i < strlen(state->letters); i++){
                printf("%c,", state->letters[i]);
                // (i == strlen(state->letters)) ? printf("\n") : NULL;
            }
            printf("\n");
            break;
            
        case MSG_PROMPT:
            break;
    }
    return 0;
}