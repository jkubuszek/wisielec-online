#define _GNU_SOURCE

#include <stdio.h> // snprintf
#include <stdlib.h> // atoi, exit
#include <string.h> // memset, str...
#include <unistd.h>  // close
#include <sys/epoll.h> // epoll_ctl
#include <ctype.h> // tolower
#include <netinet/in.h> // sockaddr, inet
#include <arpa/inet.h> // htons, htonl, inet_
#include <errno.h> // errno 
#include <syslog.h> // syslog 
#include <sys/socket.h> // sockets
#include <string.h> // memset
#include "server_utils.h"
#include "net.h"
#include "auth.h"



int socket_to_room[MAX_FD];

char socket_to_name[MAX_FD][MAX_NAME_LEN];

int save_points(const char *username, const int points) {
    FILE *file;
    FILE *tmp;
    char line[256];
    char *token;
    int find = 0;
    long position = 0;
    char *m_points;
    int m_i_point;

    file = fopen(SC_FILE, "r");
    tmp = fopen(TMP_FILE, "w");
    if (tmp == NULL) {
        if (file) 
        fclose(file);

        if (!file && !tmp) {
        syslog(LOG_ERR, "Scoreboard system failure: cannot open %s or %s", SC_FILE, TMP_FILE);
        return 1;
        }
        
        return 0;
    }

    // check for user
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            char line_copy[256];
            strcpy(line_copy, line);
            line_copy[strcspn(line_copy, "\n")] = 0;
            token = strtok(line_copy, ";");
            
            
            if (token != NULL) {
                if (strcmp(token, username) == 0) {
                    m_points = strtok(NULL, ";\n");
                    m_i_point  = atoi(m_points);
                    m_i_point += points;
                    //add points
                    fprintf(tmp, "%s;%d\n", username, m_i_point);
                    find = 1;
                    //return 1; 
                }else{
                    fprintf(tmp, "%s\n", line);
                }
                
                
            }
        }
        if (find==0){            
            fprintf(tmp, "%s;%d\n", username, points);
        }

        fclose(file);
        fclose(tmp);

        remove(SC_FILE);
        rename(TMP_FILE, SC_FILE);
        return 1;
        
    }    
}

int read_serv_points(PlayerScore *scores, const int size) {
    // memset(scores, '\0', 32);

    FILE *file = fopen(SC_FILE, "r");
    if (file == NULL) {
        syslog(LOG_ERR, "Couldn't open/find users scoreboard");
        return -1; 
    }

    char line[256];
    int count = 0;

    while (fgets(line, sizeof(line), file) && count < size) {
        // delete \n if found
        line[strcspn(line, "\n")] = 0;  

        if(strlen(line) == 0) 
            continue;

        char *token_user = strtok(line, ";");
        char *token_score = strtok(NULL, ";");

        if (token_user != NULL && token_score != NULL) {
            strncpy(scores[count].username, token_user, MAX_NAME_LEN);
            scores[count].username[MAX_NAME_LEN - 1] = '\0';
            scores[count].score = atoi(token_score);
            count++;
        }
    }

    fclose(file);
    return count;
}

int read_player_points(const char *username){
    FILE *file = fopen(SC_FILE, "r");
    if (file == NULL) {
        syslog(LOG_ERR, "Couldn't open/find users scoreboard");
        return -1; 
    }

    char line[256];
    int m_points = 0;

    

    while (fgets(line, sizeof(line), file)) {
        // delete \n if found
        line[strcspn(line, "\n")] = 0;
        // getting username
        char *token_user = strtok(line, ";");
        // getting score
        char *token_score = strtok(NULL, ";\n");

        if (token_user != NULL && token_score != NULL) {
            if (strcmp(username, token_user) == 0) {
                m_points = atoi(token_score);
                break; 
            }
        }
    }

    fclose(file);
    return m_points;
}

void check_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    
    if (f != NULL) {
        fclose(f);
        return;
    } else {
        f = fopen(filename, "w"); 
        if (f != NULL) {
            syslog(LOG_INFO, "Created missing database file: %s", filename);
            fclose(f);
            return;
        } else {
            syslog(LOG_ERR, "Cannot create/access file: %s", filename);
            closelog();
            exit(EXIT_FAILURE);
        }
    }
}

void init_server(){
    check_file(DB_FILE);
    check_file(SC_FILE);
    memset(socket_to_name, 0, sizeof(socket_to_name));
    for (int i = 0; i < MAX_ROOMS; i++) {
        rooms[i].id = i;
        rooms[i].players[0] = 0;
        rooms[i].players[1] = 0;
        rooms[i].state = 0;
    }
    // -1 means no socket is mapped to the room
    for (int i = 0; i < MAX_FD; i++) {
        socket_to_room[i] = -1;
    }
}
void reset_round(GameRoom *room){
    memset(room->game.word, 0, 32);
    memset(room->game.mask, 0, 32);
    memset(room->letters_used, 0, 32);
    room->game.lives = 8;
    room->state = 1; 
}
int assign_to_room(int sock) {
    char msg[256];
    int joined_room = -1;
    int slot = -1;
    int p0 = -1;
    int p1 = -1;

    for(int i=0; i<MAX_ROOMS; i++) { // check all the rooms 
        p0 = rooms[i].players[0];
        p1 = rooms[i].players[1];

        // look for a waiting player
        if((p0 == 0 && p1 != 0) || (p0 != 0 && p1 ==0)){
            slot = (rooms[i].players[0] == 0) ? 0 : 1; // slot = 0 means first player in the room, setter
            rooms[i].players[slot] = sock;
            joined_room = i;
            break;
        }
    }
    if(joined_room == -1){
        for(int i=0; i<MAX_ROOMS; i++) { // check all the rooms 
            p0 = rooms[i].players[0];
            p1 = rooms[i].players[1];

            // look for an empty room
            if(p0 == 0 && p1 == 0){
                slot = 0;
                rooms[i].players[slot] = sock;
                joined_room = i;
                break;
            }
        }
    }
    
    
    if(joined_room != -1){
        socket_to_room[sock] = joined_room;     
        p0 = rooms[joined_room].players[0];
        p1 = rooms[joined_room].players[1];           
        syslog(LOG_INFO, "Player %s (socket %d) joined room %d (slot %d)\n", socket_to_name[sock], sock, joined_room, slot);

        snprintf(msg, sizeof(msg), "You have joined room %d, slot %d", joined_room, slot);
        if(send_text(sock, msg)){
            syslog(LOG_ERR, "Error: could not send text.");
            return 1;
        }
    
        /// start game if full and not already going
        if (p0 && p1 && (rooms[joined_room].state == 0)) {
            rooms[joined_room].state = 1;
            rooms[joined_room].current_setter = 0; //p0 is setter
            reset_round(&rooms[joined_room]);


            // char setter_name[32] = socket_to_name[rooms[i].players[setter_slot]];
            // char setter_name[32]; 
            // strncpy(setter_name, socket_to_name[p0], 31);
            // setter_name[31] = '\0';

            snprintf(msg, sizeof(msg), "Your room is full. Game starts now - %s vs %s .\n"
            "This round, the setter is %s.", socket_to_name[p0],
            socket_to_name[p1] , socket_to_name[p0]);

            if(send_text(p0, msg)){
                syslog(LOG_ERR, "Error: could not send text.");
                return 1;
            }
            if(send_text(p1, msg)){
                syslog(LOG_ERR, "Error: could not send text.");
                return 1;
            }
            if(send_packet(p0, MSG_GAME_START, NULL, 0)){
                syslog(LOG_ERR, "Error: could not send packet");
                return 1;
            }
            if(send_packet(p1, MSG_GAME_START, NULL, 0)){
                syslog(LOG_ERR, "Error: could not send packet");   
                return 1;
            }         
            if(send_text(p0, "Set the secret word now.")){
                syslog(LOG_ERR, "Error: could not send text");
                return 1;
            }
            if(send_packet(p0, MSG_PROMPT, NULL, 0)){
                syslog(LOG_ERR, "Error: could not send packet");
                return 1;
            }
            if(send_text(p1, "Wait for the opponent to set the secret word...")){
                syslog(LOG_ERR, "Error: could not send text");
                return 1;
            }
        } else {
            if(send_text(sock, "Waiting for another player to join...")){
                syslog(LOG_ERR, "Error: could not send text");
                return 1;
            }
        }
        return 0;
    } else {
    if(send_text(sock, "Server is full.")){
        syslog(LOG_ERR, "Error: could not send text");
        return 1;
    }
    return 2;
    close(sock);
    }
}

int is_valid_ascii(const char *str) {
    while (*str) {
        unsigned char c = (unsigned char)*str;        
        if (c < 33 || c > 126) {
            return 0; 
        }
        str++;
    }
    return 1;
}


int cleanup_socket(int sock, int efd) {
    int r_idx = socket_to_room[sock];
    int opp_sock;
    int single = 0;
    if (r_idx != -1) {
        GameRoom *r = &rooms[r_idx];
        int p_idx = (r->players[0] == sock) ? 0 : 1;
        opp_sock = r->players[p_idx == 0 ? 1 : 0];
        r->players[p_idx] = 0;
        socket_to_room[sock] = -1;

        if (opp_sock) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Oponnent %s disconnected, waiting for a new player to join...", socket_to_name[sock]);
            if(send_text(opp_sock, msg)){
                syslog(LOG_ERR, "Error: could not send text");
                return 1;
            }
            single = opp_sock;
        } 
        r->state = 0;
    }
    if(epoll_ctl(efd, EPOLL_CTL_DEL, sock, NULL) < 0){
        syslog(LOG_ERR, "epoll_ctl() error : %s\n", strerror(errno));
        return 1;
    }
    close(sock);
    if(socket_to_name[sock][0] == '\0'){
        syslog(LOG_INFO, "Socket %d closed\n", sock);
    } else {
        syslog(LOG_INFO, "Player %s on socket %d disconnected, closed the socket.\n", socket_to_name[sock], sock);
        memset(socket_to_name[sock], 0, 32);
    }

    // int waiting_players[MAX_ROOMS];
    if(single){
        int match = 0;
        rooms[r_idx].players[0] = opp_sock;
        rooms[r_idx].players[1] = 0;

        // for(int i = 0; i < MAX_ROOMS; i++) { 
        for(int i = 0; i < MAX_ROOMS && !match; i++){// check all the rooms 
            //if there is a player waiting in a room
            if((rooms[i].players[1] == 0) && (rooms[i].players[0] != 0) && (rooms[i].players[0] != opp_sock)){
                // rooms[r_idx].players[1] == rooms[i].players[0];
                // socket_to_room[rooms[r_idx].players[1]] = r_idx;
                if(send_text(opp_sock, "We have found an opponent for you waiting in a different room!")){
                    syslog(LOG_ERR, "Error: could not send text.");
                    return 1;
                }
                if(send_text(rooms[i].players[0], "We are moving you to a different room with a waiting player.")){
                    syslog(LOG_ERR, "Error: could not send text.");
                    return 1;
                }
                if(assign_to_room(rooms[i].players[0])){
                    syslog(LOG_ERR, "Error: could assign player to a room.");
                    return 1;
                }
                rooms[i].players[0] = 0;
                match = 1;
            }
        }
    }
}


int handle_client_message(int sock) {

    char msg[256];
    char buffer[1024];
    int type = recv_packet(sock, buffer, sizeof(buffer), NULL);

    if (type <= 0){
        return 1; 
    }
    
    int room_idx = socket_to_room[sock];
    
    if (room_idx == -1 && type != MSG_LOGIN && type != MSG_REGISTER && type != MSG_SCOREBOARD) {
        if(send_text(sock, "Error: You are not in a room yet.")){
            syslog(LOG_ERR, "Error: could not send text.");
            return 1;
        }
        return 0;
    }
    
    // authorisation
    switch(type){
        case MSG_LOGIN:{
            Login *p = (Login*)buffer;
            p->username[31] = 0; p->password[31] = 0;
            for(int i = 0; i < MAX_FD; i++){
                if(strcmp(socket_to_name[i], p->username) == 0){
                    if(send_text(sock, "Player already logged in.")){
                        syslog(LOG_ERR, "Error: could not send text.");
                        return 1;
                    }
                    return 0;
                }
            }
            if(socket_to_name[sock][0] != '\0'){
                snprintf(msg, sizeof(msg), "You are already logged in as %s", socket_to_name[sock]);
                if(send_text(sock, msg)){
                    syslog(LOG_ERR, "Error: could not send text.");
                    return 1;
                }
                return 0;
            }
            if (authenticate_player(p->username, p->password)){
                strncpy(socket_to_name[sock], p->username, 31);
                socket_to_name[sock][31] = '\0'; 
                snprintf(msg, sizeof(msg), "Logged in as %s.", p->username);
                if(send_text(sock, msg)){
                    syslog(LOG_ERR, "Error: could not send text.");
                    return 1;
                }
                syslog(LOG_INFO, "Player %s logged in\n", p->username);
                assign_to_room(sock);
            } else {
                if(send_text(sock, "Login error.")){
                    syslog(LOG_ERR, "Error: could not send text.");
                    return 1;
                }
            }
        return 0;
        }

        case MSG_REGISTER:{
            Login *p = (Login*)buffer;
            p->username[31] = 0; p->password[31] = 0;
            if(socket_to_name[sock][0] != '\0'){
                snprintf(msg, sizeof(msg), "You are already logged in as %s", socket_to_name[sock]);
                if(send_text(sock, msg)){
                    syslog(LOG_ERR, "Error: could not send text.");
                    return 1;
                }
                return 0;
            }
            if (register_player(p->username, p->password) == 1){
                snprintf(msg, sizeof(msg), "Registered as %s.", p->username);
                if(send_text(sock, msg)){
                    syslog(LOG_ERR, "Error: could not send text.");
                    return 1;
                }
                syslog(LOG_INFO, "Registered a new player: %s", p->username);
            } else { 
                if(send_text(sock, "Registration error")){
                    syslog(LOG_ERR, "Error: could not send text.");
                    return 1;
                }
            }
        return 0; 
        }

        case MSG_SCOREBOARD:{
            PlayerScore scoreboard[MAX_PLAYERS];
            int count = read_serv_points(scoreboard, MAX_PLAYERS);
            int data_len = count * sizeof(PlayerScore);
            if(send_packet(sock, MSG_SCOREBOARD, &scoreboard, data_len)){
                syslog(LOG_ERR, "Error: could not send packet");
                return 1;
            }
        return 0; 
        }
    }

    if (room_idx == -1) {
        if(send_text(sock, "Please login first.")){
            syslog(LOG_ERR, "Error: could not send text.");
            return 1;
        }
        return 0;
    }

    GameRoom *room = &rooms[room_idx];
    int p_idx = (room->players[0] == sock) ? 0 : 1;
    int opp_sock = room->players[(p_idx == 0) ? 1 : 0];

     if (opp_sock == 0) {
        if(send_text(sock, "Waiting for another player to join...")){
            syslog(LOG_ERR, "Error: could not send text.");
            return 1;
        }
        return 0;
    }
    
    if (room->state == 1 && type == MSG_WORD) {
        if (p_idx == room->current_setter) {
            Word *p = (Word*)buffer;
            int len = strlen(p->word);
            
            for(int i = 0; i < len; i++){
                if(!isalpha((unsigned char)p->word[i])){
                    if(send_text(sock, "Invalid characters in secret word, word rejected. Set a new word:")){
                        syslog(LOG_ERR, "Error: could not send text");
                        return 1;
                    }
                    if(send_packet(sock, MSG_PROMPT, NULL, 0)){
                        syslog(LOG_ERR, "Error: could not send packet");
                        return 1;
                    }
                    return 0;
                }
            }

            for(int i = 0; i < len; i++){
                 room->game.word[i] = tolower(p->word[i]);
            }

            room->game.word[len] = '\0';
            // strcpy(room->game.word, p->word);
            
            
            int word_len = strlen(room->game.word);
            for(int i=0; i<word_len; i++){
                 room->game.mask[i] = '_';
            }
            room->game.mask[word_len] = 0; //null terminator
            room->game.lives = 8;
            
            room->state = 2; 
            
            
            // info for the guesser
            GameState state;
            strcpy(state.word_mask, room->game.mask);
            state.lives = 8; 
            state.max_lives = 8;
            strcpy(state.letters, room->letters_used);

            if(send_packet(sock, MSG_GAME_STATE, &state, sizeof(state))){
                syslog(LOG_ERR, "Error: could not send packet");
                return 1;
            }
            if(send_text(sock, "Word accepted, now, the opponent starts guessing.")){
                syslog(LOG_ERR, "Error: could not send text");
                return 1;
            }
            if(send_packet(opp_sock, MSG_GAME_STATE, &state, sizeof(state))){
                syslog(LOG_ERR, "Error: could not send packet");
                return 1;
            }
            if(send_text(opp_sock, "Your opponent has set the word. Now, start guessing!")){
                syslog(LOG_ERR, "Error: could not send text");
                return 1;
            }
        }
    } else if(room->state == 2 && type == MSG_GUESS) {
        if (p_idx != room->current_setter) {
            Guess *p = (Guess*)buffer;
            char g = p->letter;
            int hit = 0;

            for(int i=0; i<strlen(room->game.word); i++) {
                if (room->game.word[i] == g) {
                    room->game.mask[i] = g;
                    hit = 1;
                }
            }
            if (!hit){
                room->game.lives--;
            }

            GameState state;
            if(strlen(room->letters_used) < 31){
                room->letters_used[strlen(room->letters_used)] = g;
            }
            strcpy(state.word_mask, room->game.mask);
            state.lives = room->game.lives;
            state.max_lives = 8;
            strcpy(state.letters, room->letters_used);
            if(send_packet(sock, MSG_GAME_STATE, &state, sizeof(state))){
                syslog(LOG_ERR, "Error: could not send packet");
                return 1;
            }
            if(send_packet(opp_sock, MSG_GAME_STATE, &state, sizeof(state))){
                syslog(LOG_ERR, "Error: could not send packet");
                return 1;
            }

            int won = (strcmp(room->game.word, room->game.mask) == 0);
            int lost = (room->game.lives <= 0);

            if (won || lost) {
                if (won) {
                    if(send_text(sock, "YOU WON!\nYou guessed the word!")){
                        syslog(LOG_ERR, "Error: could not send text.");
                        return 1;
                    }
                    char msg[64];
                    // sprintf(msg, "You lost :(\nThe password was: %s", room->game.word);
                    snprintf(msg, sizeof(msg), "You lost :(\nThe password was: %s", room->game.word);
                    if(send_text(opp_sock, msg)){
                        syslog(LOG_ERR, "Error: could not send text.");
                        return 1;
                    }
                    if(!save_points(socket_to_name[opp_sock], 1)){
                        syslog(LOG_ERR, "Could not save player's %s points", socket_to_name[opp_sock]);
                        return 1;
                    }
                } else {
                    char msg[64];
                    // sprintf(msg, "You lost :(\nThe password was: %s", room->game.word);
                    snprintf(msg, sizeof(msg), "You lost :(\nThe password was: %s", room->game.word);
                    if(send_text(sock, msg)){
                        syslog(LOG_ERR, "Error: could not send text.");
                        return 1;
                    }
                    if(send_text(opp_sock, "YOU WON!\nYour opponent couldn't guess your secret word.")){
                        syslog(LOG_ERR, "Error: could not send text.");
                        return 1;
                    }
                    if(!save_points(socket_to_name[opp_sock], 1)){
                        syslog(LOG_ERR, "Could not save player's %s points", socket_to_name[opp_sock]);
                        return 1;
                    }

                }

                // reset and change sides
                reset_round(room);
                room->current_setter = (room->current_setter == 0) ? 1 : 0;
                
                int new_setter = room->players[room->current_setter];
                int new_guesser = room->players[(room->current_setter == 0) ? 1 : 0];

                if(send_text(new_setter, "Now it is your turn to set the secret word:")){
                    syslog(LOG_ERR, "Error: could not send text");
                    return 1;
                }
                if(send_packet(new_setter, MSG_PROMPT, NULL, 0)){
                    syslog(LOG_ERR, "Error: could not send packet");
                    return 1;
                }
                if(send_text(new_guesser, "Wait for the opponent to set the secret word...")){
                    syslog(LOG_ERR, "Error: could not send packet");
                    return 1;
                }
            } 
        }
    }

    return 0;
}




