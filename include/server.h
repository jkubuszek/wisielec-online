/**
 * @file server.h
 * @author JK 
 * @brief this file handles the sever logic
 * @date 2026-01-13
 */

#include "game.h"

#pragma once

void print_help();

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

/// @brief function used for taking care of 1 player
/// @param player_socket socket number the player is connected to
void handle_game(int client_socket);
