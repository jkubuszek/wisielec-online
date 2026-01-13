/**
 * @file server.h
 * @author JK 
 * @brief this file handles the sever logic
 * @date 2026-01-13
 */

/// @brief function used for authentication
/// @param username 32 byte char array used for storing the username
/// @param password 32 byte char array used for storing the password
/// @return error code: 1 if completed succesfully, 0 if authentication failed, -1 if an error occured
int authenticate_player(const char username, char password);

/// @brief function used for taking care of 1 player
/// @param player_socket socket number the player is connected to
void handle_player(int player_socket);