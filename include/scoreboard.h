/**
 * @file auth.h
 * @author SM 
 * @brief this file handles scoreboard functions
 * @date 2026-01-27
 */

#pragma once

/// @brief function used to register a new player
/// @param username 32 byte char array used for storing the username
/// @param password 32 byte char array used for storing the password
/// @return error code: 1 if success, 0 if user already exists, -1 if unsupported characters used, -2 if server error
int save_points(const char *username, const int *points);

/// @brief function used for authentication
/// @param username 32 byte char array used for storing the username
/// @param password 32 byte char array used for storing the password
/// @return error code: 1 if completed succesfully, 0 if authentication failed, -1 if an error occured
int read_serv_points(char *string, int const size);

/**
 * @brief 
 * 
 * @param username 
 * @param password 
 * @return int 
 */
int read_player_points(const char *username);