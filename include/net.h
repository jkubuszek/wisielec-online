/**
 * @file net.h
 * @brief this file handles all the functions and structures used for communcation
 * between server and client
 * @date 2026-01-17
 */

#pragma once
#include <stdint.h> // for uint

#define MAX_NAME_LEN 32

#define PORT 8080
#define MULTICAST_IP "239.255.255.250" 
#define MULTICAST_PORT 8888            
#define DISCOVERY_MSG "DISCOVERY_REQUEST" 
#define DISCOVERY_RESPONSE "HANGMAN_SERVER" 

typedef struct {
    uint8_t type;      
    uint16_t length;   
} PacketHeader;

#pragma pack(push, 1) /** we need this, so the compiler won't do any padding  on the structs, 
so there is no risk that communicaton between different architecture  machines will fall apart
*/

typedef struct {
    char username[MAX_NAME_LEN];
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
    char letters[32];
} GameState;

typedef struct { 
    char username[MAX_NAME_LEN];
    int score;
}PlayerScore;


#pragma pack(pop)

/// @brief all of the message types that can be sent or received
typedef enum {
    MSG_LOGIN = 1,
    MSG_REGISTER = 2,
    MSG_WORD = 3,
    MSG_GUESS = 4,
    MSG_GAME_STATE = 5,  
    MSG_TEXT = 6,        
    MSG_PROMPT = 7,
    MSG_ERROR = 8,
    MSG_GAME_START = 9,
    MSG_SCOREBOARD = 10,
} MessageType;


/**
 * @brief function for sending a header, and then a packet with data
 * 
 * @param sock destination socket
 * @param type message type (see net.h for all the message types)
 * @param data data to send
 * @param data_len length of the data to send
 * @return int error code: 0 - success, 1 - error
 */
int send_packet(int sock, uint8_t type, const void *data, uint16_t data_len);

/**
 * @brief function for sending a text message with header.type = MSG_TEXT
 * 
 * @param sock destination socket
 * @param msg text message to send
 * @return int error code: 0 - success, 1 - error 
 */
int send_text(int sock, const char *msg);

/**
 * @brief function for receiving packet header, then data
 * 
 * @param sock destination socket
 * @param buffer buffer to store the data
 * @param buffer_size size of the allocated buffer
 * @param out_len lenght of the received data
 * @return int that corresponds to header type (see net.h for all headers, numbers 1-12), -1 - error
 */
int recv_packet(int sock, void *buffer, uint16_t buffer_size, int *out_len);