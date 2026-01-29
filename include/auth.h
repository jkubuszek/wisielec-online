/**
 * @file auth.h
 * @brief this file handles aithentication functions
 * @date 2026-01-17
 */

#pragma once

/// @brief function used to register a new player
/// @param username 32 byte char array used for storing the username
/// @param password 32 byte char array used for storing the password
/// @return error code: 1 if success, 0 if user already exists, -1 if unsupported characters used, -2 if server error
int register_player(const char *username, const char *password);

/// @brief function used for authentication
/// @param username 32 byte char array used for storing the username
/// @param password 32 byte char array used for storing the password
/// @return error code: 1 if completed succesfully, 0 if authentication failed, -1 if an error occured
int authenticate_player(const char *username, const char *password);