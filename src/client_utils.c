#include <stdio.h> // printf, fgets
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // inet_addr, inet_ntoa
#include <sys/time.h> // timeval
#include <string.h> // str, memset
#include <unistd.h> // close
#include <stdlib.h> // system
#include "net.h"
#include "client_utils.h"


int find_server(char *server_ip_out) {
    int sock;
    struct sockaddr_in mcast_addr;
    char buffer[256];

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
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
        return 1;
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

void hangman_title(){
    system("clear");
    printf("H      H       A     N      NNN   GGGG    MMM       MM        A     N      NNN\n");
    printf("HH     HH     AAAA  NNNN     NN GGG       MMMMM    MMMM      AAAA  NNNN     NN\n");
    printf("HHHHHHHHH   AAA  AA NN  NNN  NN GG   GGG  MMM MMM MM MMM   AAA  AA  NN  NNN  NN\n");
    printf(" HH    HH  AAAAAAAA NN    NNN N GG     GG  MM   MMM   MM  AAAAAAAA  NN    NNN N\n");
    printf(" HH    HH AA      AA NN     NNN  GGGGGGG  MM         MMM AA      AA  NN     NNN\n");
    printf("\n");
    printf(" HHHHHHHHH AAAAAAAAA  NNNNNNNN   GGGGGGGGG    MMMMMMMMM   AAAAAAAA    NNNNNNNN\n");
    printf("                             |\n");
    printf("                             O\n");
    printf("                            /|\\\n");
    printf("                            / \\\n");
}

void draw_hangman(int lives) {
    int errors = 8 - lives;
    printf("                            %s\n", (errors >= 2 ? "████████████████████████████":""));
    printf("                           %s\n", (errors >= 2 ? "██████████████████████████████":""));
    printf("                             %s     %s           %s\n", (errors >= 1 ? "███":""), (errors >= 2 ? "████":""), (errors >= 3 ? "█":""));
    printf("                             %s   %s            %s\n", (errors >= 1 ? "███":""), (errors >= 2 ? "█████":""), (errors >= 3 ? "█":""));
    printf("                             %s %s              %s\n", (errors >= 1 ? "███":""), (errors >= 2 ? "█████":""), (errors >= 3 ? "█":""));
    printf("                             %s%s             %s\n", (errors >= 1 ? "███":""), (errors >= 2 ? "████":""), (errors >= 3 ? "███████":""));
    printf("                             %s%s             %s\n", (errors >= 1 ? "███":""), (errors >= 2 ? "██":""), (errors >= 3 ? "████   ████":""));
    printf("                             %s              %s\n", (errors >= 1 ? "███":""), (errors >= 3 ? "███       ███":""));
    printf("                             %s             %s\n", (errors >= 1 ? "███":""), (errors >= 3 ? "███         ███":""));
    printf("                             %s              %s\n", (errors >= 1 ? "███":""), (errors >= 3 ? "███       ███":""));
    printf("                             %s               %s\n", (errors >= 1 ? "███":""), (errors >= 3 ? "████   ████":""));
    printf("                             %s                 %s\n", (errors >= 1 ? "███":""), (errors >= 3 ? "███████":""));
    printf("                             %s                   %s\n", (errors >= 1 ? "███":""), (errors >= 4 ? "████":""));
    printf("                             %s                 %s%s%s\n", (errors >= 1 ? "███":""), (errors >= 5 ? "██":"  "), (errors >= 4 ? "████":""), (errors >= 6 ? "██":""));
    printf("                             %s               %s%s%s\n", (errors >= 1 ? "███":""), (errors >= 5 ? "████":"    "), (errors >= 4 ? "████":""), (errors >= 6 ? "████":""));
    printf("                             %s              %s  %s  %s\n", (errors >= 1 ? "███":""), (errors >= 5 ? "███":"   "), (errors >= 4 ? "████":""), (errors >= 6 ? "███":""));
    printf("                             %s             %s    %s    %s\n", (errors >= 1 ? "███":""), (errors >= 5 ? "██":"  "), (errors >= 4 ? "████":""), (errors >= 6 ? "██":""));
    printf("                             %s                   %s\n", (errors >= 1 ? "███":""), (errors >= 4 ? "████":""));
    printf("                             %s                   %s\n", (errors >= 1 ? "███":""), (errors >= 4 ? "████":""));
    printf("                             %s                  %s%s%s\n", (errors >= 1 ? "███":""), (errors >= 7 ? "██":"  "), (errors >= 4 ? "██":""), (errors >= 8 ? "██":""));
    printf("                             %s                 %s  %s\n", (errors >= 1 ? "███":""), (errors >= 7 ? "███":""), (errors >= 8 ? "███":""));
    printf("                             %s                %s    %s\n", (errors >= 1 ? "███":""), (errors >= 7 ? "███":""), (errors >= 8 ? "███":""));
    printf("                             %s               %s      %s\n", (errors >= 1 ? "███":""), (errors >= 7 ? "███":""), (errors >= 8 ? "███":""));
    printf("                             %s              %s        %s\n", (errors >= 1 ? "███":""), (errors >= 7 ? "███":""), (errors >= 8 ? "███":""));
    printf("                             %s\n", (errors >= 1 ? "███":""));
    printf("                             %s\n", (errors >= 1 ? "███":""));
    printf("                       ██████████████████████████████████████████\n");
    printf("                       ██████████████████████████████████████████\n");
}

void print_help() {
    printf("\nAvailable actions:\n");
    printf("  /login <username> <password>    - Log in to the game\n");
    printf("  /register <username> <password> - Create an account\n");
    // printf("  exit                           - Exit the game\n");
    printf("  /help                           - Display this help page\n");
    printf("  /scoreboard                           - Display the scoreboard\n");
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
    char response[2048];
    int data_len;
    int type = recv_packet(sockfd, response, sizeof(response), &data_len);
    if (type <= 0) {
        printf("Server disconnected or error.\n");
        return 1;
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
            printf("\n                              --- GAME STATUS ---\n");
            draw_hangman(state->lives);
            printf("Secret word: ");
            //char st = state->word_mask;
            for(int i=0; i < strlen(state->word_mask); i++){
                printf("%c ",state->word_mask[i]);
            }
            printf("\n");
            printf("Lives left: %d / %d\n", state->lives, state->max_lives);
            printf("-------------------\n");
            printf("letters already used : ");
            for(int i = 0; i < strlen(state->letters); i++){
                printf("%c,", state->letters[i]);
                // (i == strlen(state->letters)) ? printf("\n") : NULL;
            }
            printf("\n");
            break;
        case MSG_SCOREBOARD:
            int count = data_len / sizeof(PlayerScore);
            PlayerScore *scores = (PlayerScore*)response;
            printf("-----------SCOREBOARD-------------\n");
            for(int i = 0; i < count; i++){
                printf("%s: %d pts\n", scores[i].username, scores[i].score);
            }
            printf("----------------------------------\n");
            break;            
        
        case MSG_PROMPT:
            break;
    }
    return 0;
}