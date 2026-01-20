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

int send_packet(int sock, uint8_t type, const void *data, uint16_t data_len);

void send_text(int sock, const char *msg);

int recv_packet(int sock, void *buffer, uint16_t buffer_size);