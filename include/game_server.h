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



// this is similar to GameSate, but it's for server only
 typedef struct{
    char word[32];
    char mask[32];
    int lives;
} GameSession;



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
    char letters_used[32]; // letters already used by the guesser
} GameRoom;

extern GameRoom rooms[MAX_ROOMS];

int handle_client_message(int sock);

void reset_round(GameRoom *room);

/// @brief setting rooms to default
void init_server();

void assign_to_room(int sock);

void cleanup_socket(int sock, int efd);