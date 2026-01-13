/**
 * @file protocol.h
 * @author JK 
 * @brief this file handles message types, game states and game message types 
 * @date 2026-01-13
 */

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_LEN 32 

/// @brief all of the message types that can be sent or received
typedef enum {
    MSG_LOGIN,      
    MSG_GUESS,      
    MSG_RESPONSE,   
    MSG_ERROR,      
    MSG_EXIT        
} MessageType;

/// @brief all of the game states
typedef enum {
    GAME_IN_PROGRESS,
    GAME_WON,
    GAME_LOST
} GameState;

/// @brief all of the game messages
typedef struct {
    MessageType type;
    char username[32];      
    char password[32];      
    char guess;             
    char visible_word[64];  
    int lives_remaining;
    GameState state;
    char message_text[128]; 
} GameMessage;