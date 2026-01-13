/**
 * @file server.h
 * @author JK 
 * @brief this file handles the sever logic
 * @date 2026-01-13
 */



typedef struct{
    char word[32];
    char mask[32]
}GameSession;

/// @brief sigchild handler
/// @param signo signal number, passed by linux kernel
void sig_chld(int signo);

/// @brief sigpipe handler
/// @param signo signal number, passed by linux kernel
void sig_pipe(int signo);

/// @brief function used for checking the guess
/// @param session GameSession struct that defines current session's word and word mask
/// @param letter a letter to process
/// @return returns 1 if comepleted succesfully, -1 if encountered an error
int process_guess(GameSession session, char letter);

/// @brief function used for authentication
/// @param username 32 byte char array used for storing the username
/// @param password 32 byte char array used for storing the password
/// @return error code: 1 if completed succesfully, 0 if authentication failed, -1 if an error occured
int authenticate_player(const char username, char password);

/// @brief function used for taking care of 1 player
/// @param player_socket socket number the player is connected to
void handle_client(int client_socket);
