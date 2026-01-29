#pragma once

#include "net.h"

///max number of rooms
#define MAX_ROOMS 50
///max number of file descriptors
#define MAX_FD 1024
///max number of players on scoreboard
#define MAX_PLAYERS 50

///scoreboard database file
#define SC_FILE "scoreboard.csv"
///temporary file for writing to scoreboard
#define TMP_FILE "temp.csv"
///users database, containing usernames and paasswords
#define DB_FILE "users.csv"

/// @brief structure that holds parameters of the current game
typedef struct{
    char word[32]; ///secret word
    char mask[32]; ///current mask on word
    int lives; ///current lives left
} GameSession; // this is similar to GameSate, but it's for server only

/// @brief game room struct, contains all the essential info about a game room
typedef struct {
    int id;                 ///room id
    int players[2];  ///array of 2 players, their int value means: 0 - absent, 1 - present
    GameSession game;    ///GameSession struct containg the word, current mask and lives left
    int current_setter;     ///int that hints at the player that is setting the word in this session
    int state;        ///current game state: 0 - waiting, 1 - setting the word, 2 - guessing
    char letters_used[32]; ///letters already used by the guesser
} GameRoom;

extern GameRoom rooms[MAX_ROOMS];

/**
 * @brief function that includes the player in the scoreboard if they are
 * not there already and increases player's score by points value 
 * 
 * @param username username of the player
 * @param points value of points to add to their score
 * @return returns error code: 0 - success, 1 - error
 */
int save_points(const char *username, const int points);

/**
 * @brief function for reading the whole scoreboard
 * 
 * @param scores buffer to write the scoreboard (an array of PlayerScore structs)
 * @param size number of players to read
 * @return int number of players read; -1 - error
 */
int read_serv_points(PlayerScore *scores, const int size);

/**
 * @brief function for creating a multicast socket for finding the server
 * 
 * @return int file descriptor number
 */
int multicast_socket();

/**
 * @brief function for handling a message from the client
 * 
 * @param sock source socket
 * @return int error code: 0 - success, 1 - error
 */
int handle_client_message(int sock);

/**
 * @brief function for clearing a round (state, word, mask, letters used, lives left)
 * 
 * @param room GameRoom struct to clear
 */
void reset_round(GameRoom *room);

/**
 * @brief function for setting all the server game rooms to default 
 * 
 */
void init_server();

/**
 * @brief function for assigning a player to a room with a vacant slot, 
 * this function prioritizes matching the player to a room with a player that is already waiting
 * 
 * @param sock socket of the player to assign
 * @return int error code: 0 - success, 1 - error, 2 - server full;
 */
int assign_to_room(int sock);

/**
 * @brief function for disconnecting a player, clearing their game info, 
 * notifying the other player and finding a new room for them to play
 * 
 * @param sock socket of the player to remove
 * @param efd epoll file descriptor
 * @return int error code: 0 - success, 1 - error
 */
int cleanup_socket(int sock, int efd);

/**
 * @brief function for checking if each character of str is >= 33 and <= 127, 
 * this is a security feature so users don't use problematic characters in usernames or password
 * @param str array of chars to check
 * @return int logic result: 0 - contains an invalid char, 1 - is fine
 */
int is_valid_ascii(const char *str);