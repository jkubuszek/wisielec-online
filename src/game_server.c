#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <ctype.h>
#include "game_server.h"
#include "net.h"
#include "auth.h"

#define MAX_FD 1024



int socket_to_room[MAX_FD];

char socket_to_name[MAX_FD][32];

void init_server(){
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
void assign_to_room(int sock) {
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
        printf("Player %s (socket %d) joined room %d (slot %d)\n", socket_to_name[sock], sock, joined_room, slot);

        snprintf(msg, sizeof(msg), "You have joined room %d, slot %d", joined_room, slot);
        send_text(sock, msg);
    
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

            send_text(p0, msg);
            send_text(p1, msg);

            send_packet(p0, MSG_GAME_START, NULL, 0);
            send_packet(p1, MSG_GAME_START, NULL, 0);
            
            send_text(p0, "Set the secret word now.");
            send_packet(p0, MSG_PROMPT, NULL, 0);
            send_text(p1, "Wait for the opponent to set the secret word...");
        } else {
            send_text(sock, "Waiting for another player to join...");
        }
    } else {
    send_text(sock, "Server is full.");
    close(sock);
    }
}


void cleanup_socket(int sock, int efd) {
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
            send_text(opp_sock, msg);
            single = opp_sock;
        } 
        r->state = 0;
    }
    epoll_ctl(efd, EPOLL_CTL_DEL, sock, NULL);
    close(sock);
    if(socket_to_name[sock][0] == '\0'){
        printf("Socket %d closed\n", sock);
    } else {
        printf("Player %s on socket %d disconnected, closed the socket.\n", socket_to_name[sock], sock);
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
                send_text(opp_sock, "We have found an opponent for you waiting in a different room!");
                send_text(rooms[i].players[0], "We are moving you to a different room with a waiting player.");
                assign_to_room(rooms[i].players[0]);
                rooms[i].players[0] = 0;
                match = 1;
            }
        }
    }
}

int handle_client_message(int sock) {

    char msg[256];
    char buffer[1024];
    int type = recv_packet(sock, buffer, sizeof(buffer));

    if (type <= 0){
        return -1; // sending error detected
    }
    
    int room_idx = socket_to_room[sock];
    
    if (room_idx == -1 && type != MSG_LOGIN && type != MSG_REGISTER && type != MSG_EXIT) {
        send_text(sock, "Error: You are not in a room yet.");
        return 0;
    }
    
    // authorisation
    switch(type){
        case MSG_LOGIN:{
            Login *p = (Login*)buffer;
            p->username[31] = 0; p->password[31] = 0;
            if (authenticate_player(p->username, p->password)){
                strncpy(socket_to_name[sock], p->username, 31);
                socket_to_name[sock][31] = '\0'; 
                snprintf(msg, sizeof(msg), "Logged in as %s.", p->username);
                send_text(sock, msg);
                printf("Player %s logged in\n", p->username);
                assign_to_room(sock);
            } else {
                send_text(sock, "Login error.");
            }
        return 0;
        }

        case MSG_REGISTER:{
            Login *p = (Login*)buffer;
            p->username[31] = 0; p->password[31] = 0;
            if (register_player(p->username, p->password) == 1){
                snprintf(msg, sizeof(msg), "Registered as %s.", p->username);
                send_text(sock, msg);
                printf("Registered a new player: %s", p->username);
            } else { 
                send_text(sock, "Registration error");
            }
        return 0; 
        }

        case MSG_EXIT:
        return -1;
    }

    if (room_idx == -1) {
        send_text(sock, "Please login first.");
        return 0;
    }

    GameRoom *room = &rooms[room_idx];
    int p_idx = (room->players[0] == sock) ? 0 : 1;
    int opp_sock = room->players[(p_idx == 0) ? 1 : 0];

     if (opp_sock == 0) {
        send_text(sock, "Waiting for another player to join...");
        return 0;
    }
    
    if (room->state == 1 && type == MSG_WORD) {
        if (p_idx == room->current_setter) {
            Word *p = (Word*)buffer;
            
            int len = strlen(p->word);
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

            send_packet(sock, MSG_GAME_STATE, &state, sizeof(state));
            send_text(sock, "Word accepted, now, the opponent starts guessing.");
            send_packet(opp_sock, MSG_GAME_STATE, &state, sizeof(state));
            send_text(opp_sock, "Your opponent has set the word. Now, start guessing!");
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
            send_packet(sock, MSG_GAME_STATE, &state, sizeof(state));
            send_packet(opp_sock, MSG_GAME_STATE, &state, sizeof(state));

            int won = (strcmp(room->game.word, room->game.mask) == 0);
            int lost = (room->game.lives <= 0);

            if (won || lost) {
                if (won) {
                    send_text(sock, "YOU WON!\nYou guessed the word!");
                    char msg[64];
                    sprintf(msg, "You lost :(\nThe password was: %s", room->game.word);
                    send_text(opp_sock, msg);
                } else {
                    char msg[64]; sprintf(msg, "You lost :(\nThe password was: %s", room->game.word);
                    send_text(sock, msg);
                    send_text(opp_sock, "YOU WON!\nYour opponent couldn't guess your secret word.");
                }

                // reset and changing sides
                reset_round(room);
                room->current_setter = (room->current_setter == 0) ? 1 : 0;
                
                int new_setter = room->players[room->current_setter];
                int new_guesser = room->players[(room->current_setter == 0) ? 1 : 0];

                // send_packet(rooms[i].players[0], MSG_GAME_START, NULL, 0);
                // send_packet(rooms[i].players[1], MSG_GAME_START, NULL, 0);

                send_text(new_setter, "Now it is your turn to set the secret word:");
                send_packet(new_setter, MSG_PROMPT, NULL, 0); 
                send_text(new_guesser, "Wait for the opponent to set the secret word...");
                

            } 
        }
    }

    return 0;
}




