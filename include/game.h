/**
 * @file game.h
 * @author JK 
 * @brief this file handles message types, game states and game message types 
 * @date 2026-01-13
 */
#pragma once

#include <stdint.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_LEN 32 
#define MAX_ROOMS 50

/// @brief all of the message types that can be sent or received
typedef enum {
    MSG_LOGIN = 1,
    MSG_REGISTER = 2,
    MSG_WORD = 3,
    MSG_GUESS = 4,
    MSG_GAME_STATE = 5,  
    MSG_TEXT = 6,        
    MSG_PROMPT = 7,
    MSG_EXIT = 8,
    MSG_ERROR = 9,
    MSG_GAME_START = 10
} MessageType;

// this is similar to GameSate, but it's for server only
 typedef struct{
    char word[32];
    char mask[32];
    int lives;
} GameSession;

#pragma pack(push, 1) /** we need this, so the compiler won't do any padding  on the structs, 
so there is no risk that communicaton between different architecture  machines will fall apart
*/

typedef struct {
    char username[32];
    char password[32];
} Login;

typedef struct {
    char word[32];
} Word;

typedef struct {
    char letter;
} Guess;

//this struct we send to client
typedef struct {
    char word_mask[32];
    uint8_t lives;
    uint8_t max_lives;
} GameState;

#pragma pack(pop)

/** @brief game room struct, contains all the essential info 
 *  @param id room id
 *  @param players array of 2 players, their int value means: 0 - absent, 1 - present
 *  @param game GameSession struct containg the word, current mask and lives left
 *  @param current_setter int that hints at the player that is setting the word in this session
 *  @param state current game state: 0 - waiting, 1 - setting the word, 2 - guessing
 */
typedef struct {
    int id;                 // room id
    int players[2];  // player sockets in this room
    // int usernames[2][32]; // players' usernames
    GameSession game;    // game state in this room
    int current_setter;     // who is setting the word
    int state;        // 0 - waiting, 1 - setting the word, 2 - guessing
} GameRoom;

extern GameRoom rooms[MAX_ROOMS];



int handle_client_message(int sock);

int handle_server_message();

void reset_round(GameRoom *room);

/// @brief setting room objects to default
void init_server();

void assign_to_room(int sock);

void cleanup_socket(int sock, int efd);