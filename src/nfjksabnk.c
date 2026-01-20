#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "../include/protocol.h"
#include "../include/net_utils.h"

// Deklaracje z auth.c
int authenticate_player(const char *username, const char *password);
int register_player(const char *username, const char *password);

#define MAX_ROOMS 50
#define MAX_EVENTS 20
#define MAX_FD 2048

// Struktura stanu gry
typedef struct {
    char word[32];
    char mask[32];
    int lives;
} GameSession;

// Struktura pokoju (2 graczy)
typedef struct {
    int id;
    int players[2];      // Socket gracza 0 i 1
    GameSession game;
    int current_setter;  // Kto układa hasło (0 lub 1)
    int state;           // 0=Czekanie, 1=Ustawianie, 2=Zgadywanie
} GameRoom;

// --- ZMIENNE GLOBALNE ---
GameRoom rooms[MAX_ROOMS];
int socket_to_room[MAX_FD]; // Mapowanie: socket -> id pokoju

void init_server() {
    for(int i=0; i<MAX_ROOMS; i++) {
        rooms[i].id = i;
        rooms[i].players[0] = 0;
        rooms[i].players[1] = 0;
        rooms[i].state = 0;
    }
    for(int i=0; i<MAX_FD; i++) socket_to_room[i] = -1;
}

// Resetuje rundę w pokoju
void reset_round(GameRoom *room) {
    memset(room->game.word, 0, 32);
    memset(room->game.mask, 0, 32);
    room->game.lives = 6;
    room->state = 1; // Powrót do ustawiania hasła
}

// Pomocnicza: wyślij tekst
void send_text(int sock, const char *msg) {
    send_packet(sock, MSG_TEXT, msg, strlen(msg) + 1);
}

// --- LOGIKA GRY ---
int handle_client_message(int sock) {
    char buffer[1024];
    int type = recv_packet(sock, buffer, sizeof(buffer));

    if (type <= 0) return -1; // Rozłączenie

    int room_idx = socket_to_room[sock];
    
    // --- AUTH ---
    if (type == MSG_LOGIN) {
        PayloadLogin *p = (PayloadLogin*)buffer;
        p->username[31] = 0; p->password[31] = 0;
        if (authenticate_player(p->username, p->password)) 
            send_text(sock, "Zalogowano.");
        else 
            send_text(sock, "Blad logowania.");
        return 0;
    }
    if (type == MSG_REGISTER) {
        PayloadLogin *p = (PayloadLogin*)buffer;
        p->username[31] = 0; p->password[31] = 0;
        if (register_player(p->username, p->password) == 1)
            send_text(sock, "Zarejestrowano.");
        else
            send_text(sock, "Blad rejestracji.");
        return 0;
    }

    // --- GAME LOGIC ---
    if (room_idx == -1) return 0; // Gracz nie jest w pokoju
    GameRoom *room = &rooms[room_idx];

    // Ustalanie kto jest kim
    int p_idx = (room->players[0] == sock) ? 0 : 1;
    int opp_sock = room->players[(p_idx == 0) ? 1 : 0];

    // Jeśli brak przeciwnika -> czekamy
    if (opp_sock == 0) {
        send_text(sock, "Czekanie na drugiego gracza...");
        return 0;
    }

    // FAZA 1: Ustawianie hasła
    if (room->state == 1 && type == MSG_SET_WORD) {
        if (p_idx == room->current_setter) {
            PayloadSetWord *p = (PayloadSetWord*)buffer;
            strcpy(room->game.word, p->word);
            
            // Inicjalizacja maski
            int len = strlen(room->game.word);
            for(int i=0; i<len; i++) room->game.mask[i] = '_';
            room->game.mask[len] = 0;
            room->game.lives = 6;
            
            room->state = 2; // Start gry
            
            send_text(sock, "Haslo przyjete. Przeciwnik zgaduje.");
            
            // Info dla zgadującego
            PayloadGameState state;
            strcpy(state.visible_word, room->game.mask);
            state.lives = 6; state.max_lives = 6;
            send_packet(opp_sock, MSG_GAME_STATE, &state, sizeof(state));
            send_text(opp_sock, "Start! Zgaduj haslo.");
        }
    }
    // FAZA 2: Zgadywanie
    else if (room->state == 2 && type == MSG_GUESS) {
        if (p_idx != room->current_setter) {
            PayloadGuess *p = (PayloadGuess*)buffer;
            char g = p->letter;
            int hit = 0;

            for(int i=0; i<strlen(room->game.word); i++) {
                if (room->game.word[i] == g) {
                    room->game.mask[i] = g;
                    hit = 1;
                }
            }
            if (!hit) room->game.lives--;

            // Sprawdzenie końca
            int won = (strcmp(room->game.word, room->game.mask) == 0);
            int lost = (room->game.lives <= 0);

            if (won || lost) {
                // Koniec rundy
                if (won) {
                    send_text(sock, "WYGRANA! Zgadles haslo.");
                    char msg[64]; sprintf(msg, "PRZEGRANA! Haslo to: %s", room->game.word);
                    send_text(opp_sock, msg);
                } else {
                    char msg[64]; sprintf(msg, "PRZEGRANA! Haslo to: %s", room->game.word);
                    send_text(sock, msg);
                    send_text(opp_sock, "WYGRANA! Przeciwnik stracil zycia.");
                }

                // Reset i zamiana
                reset_round(room);
                room->current_setter = (room->current_setter == 0) ? 1 : 0;
                
                int new_setter = room->players[room->current_setter];
                int new_guesser = room->players[(room->current_setter == 0) ? 1 : 0];

                send_packet(new_setter, MSG_PROMPT, NULL, 0); // "Podaj hasło"
                send_text(new_setter, "Twoja kolej na ustawienie hasla.");
                send_text(new_guesser, "Przeciwnik ustawia haslo...");

            } else {
                // Gra trwa
                PayloadGameState state;
                strcpy(state.visible_word, room->game.mask);
                state.lives = room->game.lives;
                state.max_lives = 6;
                send_packet(sock, MSG_GAME_STATE, &state, sizeof(state)); // Do zgadującego
                
                char msg[64]; sprintf(msg, "Przeciwnik zgaduje: %c (%s)", g, hit ? "Traf" : "Pudlo");
                send_text(opp_sock, msg);
            }
        }
    }

    return 0;
}

// Dołączanie do pokoju
void assign_to_room(int sock) {
    for(int i=0; i<MAX_ROOMS; i++) {
        if (rooms[i].players[0] == 0 || rooms[i].players[1] == 0) {
            int slot = (rooms[i].players[0] == 0) ? 0 : 1;
            rooms[i].players[slot] = sock;
            socket_to_room[sock] = i;
            
            printf("Socket %d dolaczyl do pokoju %d (slot %d)\n", sock, i, slot);
            
            // Jeśli pełny -> start
            if (rooms[i].players[0] && rooms[i].players[1]) {
                rooms[i].state = 1;
                rooms[i].current_setter = 0;
                reset_round(&rooms[i]);
                
                send_packet(rooms[i].players[0], MSG_PROMPT, NULL, 0);
                send_text(rooms[i].players[0], "Start gry! Ustaw haslo.");
                send_text(rooms[i].players[1], "Start gry! Czekaj na haslo.");
            } else {
                send_text(sock, "Czekanie na gracza...");
            }
            return;
        }
    }
    send_text(sock, "Serwer pelny.");
    close(sock);
}

void cleanup_socket(int sock, int epoll_fd) {
    int r_idx = socket_to_room[sock];
    if (r_idx != -1) {
        GameRoom *r = &rooms[r_idx];
        int p_idx = (r->players[0] == sock) ? 0 : 1;
        int opp_sock = r->players[p_idx == 0 ? 1 : 0];
        
        r->players[p_idx] = 0;
        socket_to_room[sock] = -1;
        
        if (opp_sock) {
            send_text(opp_sock, "Przeciwnik rozlaczyl sie. Szukam nowego...");
            r->state = 0;
        } else {
            r->state = 0;
        }
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);
    close(sock);
    printf("Socket %d zamkniety\n", sock);
}

int main() {
    int master_sock, epoll_fd;
    struct sockaddr_in addr;
    struct epoll_event ev, events[MAX_EVENTS];

    init_server();

    // Socket Setup
    master_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt=1; setsockopt(master_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    addr.sin_family=AF_INET; addr.sin_addr.s_addr=INADDR_ANY; addr.sin_port=htons(PORT);
    bind(master_sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(master_sock, 10);

    // Epoll Setup
    epoll_fd = epoll_create1(0);
    ev.events = EPOLLIN;
    ev.data.fd = master_sock;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, master_sock, &ev);

    printf("SERWER START: Port %d, Epoll Active.\n", PORT);

    while(1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for(int i=0; i<n; i++) {
            int curr_fd = events[i].data.fd;

            if (curr_fd == master_sock) {
                int new_sock = accept(master_sock, NULL, NULL);
                ev.events = EPOLLIN;
                ev.data.fd = new_sock;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &ev) == 0) {
                    assign_to_room(new_sock);
                }
            } else {
                if (handle_client_message(curr_fd) == -1) {
                    cleanup_socket(curr_fd, epoll_fd);
                }
            }
        }
    }
    return 0;
}