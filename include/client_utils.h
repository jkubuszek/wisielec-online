/**
 * @file client_utils.h
 * @brief this file handles all the functions used by the client
 * @date 2026-01-29
 */
#pragma once

/**
 * @brief function for finding server in the multicast group 239.255.255.250
 * 
 * @param server_ip_out buffer for storing the sever's unicast ip
 * @return int error code: 0 - success, 1 - error
 */
int find_server(char *server_ip_out);

/**
 * @brief function for drawing a hangman (based on lives left)
 * 
 */
void draw_hangman(int lives);

/**
 * @brief function for drawing the hangman title screen 
 * 
 */
void hangman_title();

/**
 * @brief function for printing all available commands
 * 
 */
void print_help();

/**
 * @brief function for getting user's input
 * 
 * @param buffer buffer for storing the input
 * @param size size of the input
 * @return int error code: 0 - success, 1 - error
 */
int get_input(char *buffer, int size);

/**
 * @brief function for handling a message received from server
 * 
 * @param sockfd source socket
 * @param game_started 1 if game has started, 0 otherwise
 * @return int error code: 0 - success, 1 - errror
 */
int handle_server_message(int sockfd, int *game_started);
