#include "net.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>


int send_packet(int sock, uint8_t type, const void *data, uint16_t data_len) {
    PacketHeader header;
    header.type = type;
    header.length = htons(data_len); 

    // send header
    if (send(sock, &header, sizeof(header), 0) != sizeof(header)){
        return -1;
    }
    // send data
    if (data_len > 0 && data != NULL) {
        if (send(sock, data, data_len, 0) != data_len){
            return -1;
        }
    }  
    return 1;
}

void send_text(int sock, const char *msg) {
    send_packet(sock, MSG_TEXT, msg, strlen(msg) + 1);
}

int recv_packet(int sock, void *buffer, uint16_t buffer_size) {
    PacketHeader header;

    int n = recv(sock, &header, sizeof(header), MSG_WAITALL); // using WAITALL to ensure we get all the data, but have to test if it's worth it
    if (n <= 0){
        return n; 
    }
    uint16_t msg_len = ntohs(header.length); 

    // if (msg_len > buffer_size) msg_len = buffer_size;

    if (msg_len > 0) {
        if (recv(sock, buffer, msg_len, MSG_WAITALL) <= 0){
            return -1;
        }
    }
    return header.type; 
}