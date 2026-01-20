#include "game.h"
#include "net.h"
#include "auth.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

#define MAX_FD 1024


int socket_to_room[MAX_FD];

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
void reset_room(GameRoom *room){
    memset(room->game.word, 0, 32);
    memset(room->game.mask, 0, 32);
    room->game.lives = 6;
    room->state = 1; 
}
void assign_to_room(int sock) {
    for(int i=0; i<MAX_ROOMS; i++) { // check all the rooms 
        //if there is a player witing in a room
        if (rooms[i].players[0] == 0 || rooms[i].players[1] == 0) {
            int slot = (rooms[i].players[0] == 0) ? 0 : 1; //slot = 0 means first player in the room, setter
            rooms[i].players[slot] = sock;
            socket_to_room[sock] = i;
            
            
            printf("Socket %d joined room %d (slot %d)\n", sock, i, slot);
            
            /// start game if full
            if (rooms[i].players[0] && rooms[i].players[1]) {
                rooms[i].state = 1;
                rooms[i].current_setter = 0;
                reset_round(&rooms[i]);
                
                send_packet(rooms[i].players[0], MSG_PROMPT, NULL, 0);
                send_text(rooms[i].players[0], "Start! Set the secret word now.");
                send_text(rooms[i].players[1], "Start! Wait for the other player to set the secret word.");
            } else {
                send_text(sock, "Waiting for another player to join...");
            }
            return;
        }
    }
    send_text(sock, "Server is full.");
    close(sock);
}

void cleanup_socket(int sock, int epollfd) {
    int r_idx = socket_to_room[sock];
    if (r_idx != -1) {
        GameRoom *r = &rooms[r_idx];
        int p_idx = (r->players[0] == sock) ? 0 : 1;
        int opp_sock = r->players[p_idx == 0 ? 1 : 0];
        
        r->players[p_idx] = 0;
        socket_to_room[sock] = -1;
        
        if (opp_sock) {
            send_text(opp_sock, "Oponnent disconnected, waiting for a new player to join...");
            r->state = 0;
        } else {
            r->state = 0;
        }
    }
    epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, NULL);
    close(sock);
    printf("Socket %d closed\n", sock);
}

int handle_client_message(int sock) {

    char msg[256];
    char buffer[1024];
    int type = recv_packet(sock, buffer, sizeof(buffer));

    if (type <= 0){
        return -1; // sending error detected
    }
    
    int room_idx = socket_to_room[sock];
    
    // authorisation
    if (type == MSG_LOGIN) {
        Login *p = (Login*)buffer;
        p->username[31] = 0; p->password[31] = 0;
        if (authenticate_player(p->username, p->password)){
            snprintf(msg, sizeof(msg), "Logged in as %s.", p->username);
            send_text(sock, msg);
        } else {
            send_text(sock, "Login error.");
        }
            return 0;
    }
    if (type == MSG_REGISTER) {
        Login *p = (Login*)buffer;
        p->username[31] = 0; p->password[31] = 0;
        if (register_player(p->username, p->password) == 1){
            snprintf(msg, sizeof(msg), "Registered as %s.", p->username);
            send_text(sock, msg);
        } else { 
            send_text(sock, "Registration error");
        }
        return 0;
    }

    

    return 0;
}

void reset_round(GameRoom *room) {
    memset(room->game.word, 0, 32);
    memset(room->game.mask, 0, 32);
    room->game.lives = 6;
    room->state = 1; // Powrót do ustawiania hasła
}

int process_guess(GameSession session, char letter){

}