#include <stdint.h>
#include <stdio.h>
#include "net.h"


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