#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>                /* timespec{} for pselect() */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>               /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include 	<unistd.h>
#include    "protocol.h"
#include    "server.h"

int authenticate_player(const char username, const char password){

    FILE *file = fopen("users.txt", "r");
    
    if (file == NULL) {
        perror("ERROR: couldn't open/find users.txt");
        return 0; 
    }

    char file_user[MAX_LEN];
    char file_pass[MAX_LEN];
    int auth_success = 0; 

    while (fscanf(file, "%31s %31s", file_user, file_pass) == 2) {
        if (strcmp(username, file_user) == 0 && strcmp(password, file_pass) == 0) {
            auth_success = 1;
            break; 
        }
    }

    fclose(file);
    return auth_success;
}
   


int main(){
    return 0;
}