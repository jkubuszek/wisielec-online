#pragma once


int find_server(char *server_ip_out);

void print_help();

int get_input(char *buffer, int size);

int handle_server_message(int sockfd, int *game_started);