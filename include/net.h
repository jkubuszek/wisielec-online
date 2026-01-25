/**
 * @file net.h
 * @author JK 
 * @brief 
 * @date 2026-01-17
 */

#pragma once
#include <stdint.h>

typedef struct {
    uint8_t type;      
    uint16_t length;   
} PacketHeader;

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
    char letters[32];
} GameState;

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
    MSG_EXIT = 8,
    MSG_ERROR = 9,
    MSG_GAME_START = 10
} MessageType;

int send_packet(int sock, uint8_t type, const void *data, uint16_t data_len);

void send_text(int sock, const char *msg);

int recv_packet(int sock, void *buffer, uint16_t buffer_size);