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
#include 	    <unistd.h>
#include        <signal.h>
#include        <wait.h>
#include        "protocol.h"
#include        "server.h"

#define DB_FILE "users.csv"
#define LISTENQ 10
#define MAXLINE 1024

void sig_chld(int signo){
	pid_t	pid;
	int		stat;
	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);
	return;
}

void sig_pipe(int signo){
	printf("Server received SIGPIPE - Default action is exit \n");
	exit(1);
}

int register_player(const char *username, const char *password) {
    FILE *file;
    char line[256];
    char *token;

    //reject ';' and '\n'
    if (strchr(username, ';') || strchr(password, ';') || 
        strchr(username, '\n') || strchr(password, '\n')) {
        return -1;      
    }
    file = fopen(DB_FILE, "r");

    //check for duplicates
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            char line_copy[256];
            strcpy(line_copy, line);
           
            token = strtok(line_copy, ";");
            
            if (token != NULL) {
                if (strcmp(token, username) == 0) {
                    fclose(file);
                    return 0; 
                }
            }
        }
        fclose(file);
    }

    file = fopen(DB_FILE, "a");
    if (file == NULL) {
        perror("Couldn't write to users database");
        return -2;
    }

    fprintf(file, "%s;%s\n", username, password);

    fflush(file);
    fclose(file);
    return 1; 
}

int authenticate_player(const char *username, const char *password) {
    FILE *file = fopen(DB_FILE, "r");
    if (file == NULL) {
        perror("ERROR: couldn't open/find users database");
        return 0; 
    }

    char line[256];
    int auth_success = 0;

    while (fgets(line, sizeof(line), file)) {

        // delete \n if found
        line[strcspn(line, "\n")] = 0;
        // getting login
        char *token_user = strtok(line, ";");
        // getting password
        char *token_pass = strtok(NULL, ";");

        // authentication
        if (token_user != NULL && token_pass != NULL) {
            if (strcmp(username, token_user) == 0 && strcmp(password, token_pass) == 0) {
                auth_success = 1;
                break; 
            }
        }
    }

    fclose(file);
    return auth_success;
}

int process_guess(GameSession session, char letter){

}

void handle_client(int connfd) {
    GameMessage msg;
    
    printf("Waiting for data from client...\n");

    while (recv(connfd, &msg, sizeof(msg), 0) > 0) {
        GameMessage response;
        memset(&response, 0, sizeof(response));

        if (msg.type == MSG_LOGIN) {
            printf("Logging in: %s\n", msg.username);
            if (authenticate_player(msg.username, msg.password)) {
                response.type = MSG_RESPONSE;
                strcpy(response.message_text, "Logged in successfully.");
               //game here
            } else {
                response.type = MSG_ERROR;
                strcpy(response.message_text, "Incorrect login details.");
                printf("Login of player %s failed.\n", msg.username);
            }
        } 
        else if (msg.type == MSG_REGISTER) {
            printf("Registration: %s\n", msg.username);
            int res = register_player(msg.username, msg.password);
            if (res == 1) {
                response.type = MSG_RESPONSE;
                strcpy(response.message_text, "Account created, now you can log in.");
            } else if (res == 0) {
                response.type = MSG_ERROR;
                strcpy(response.message_text, "This username is already taken, try a different one.");
            } else {
                response.type = MSG_ERROR;
                strcpy(response.message_text, "Registration error, you probably used ';' or '\n', try without them.");
            }
        }
        else if (msg.type == MSG_EXIT) {
            printf("Player %s disconnected.\n", msg.username);
            break;
        }

        
        send(connfd, &response, sizeof(response), 0);
    }
    
    close(connfd);
}

int main(int argc, char **argv)
{
	int				listenfd, connfd;
    char				buff[MAXLINE], str[INET_ADDRSTRLEN+1];
	socklen_t			len;
	struct sockaddr_in	servaddr, cliaddr;


        if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                fprintf(stderr,"socket error : %s\n", strerror(errno));
                return 1;
        }

        int on = 1;               
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
            fprintf(stderr,"SO_REUSEADDR setsockopt error : %s\n", strerror(errno));
        }
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr   = htonl(INADDR_ANY);
	servaddr.sin_port   = htons(8080);	

        if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
                fprintf(stderr,"bind error : %s\n", strerror(errno));
                return 1;
        }

        if ( listen(listenfd, LISTENQ) < 0){
                fprintf(stderr,"listen error : %s\n", strerror(errno));
                return 1;
        }

    signal(SIGCHLD, sig_chld);
	signal(SIGPIPE, sig_pipe);

	fprintf(stderr,"Waiting for clients ... \n");
	for ( ; ; ) {
		len = sizeof(cliaddr);
        	if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0){
                	fprintf(stderr,"accept error : %s\n", strerror(errno));
                	continue;
        	}

		bzero(str, sizeof(str));
	   	inet_ntop(AF_INET, (struct sockaddr  *) &cliaddr.sin_addr,  str, sizeof(str));
		printf("Connection from %s\n", str);

        pid_t pid = fork();

        if (pid == -1) {
            perror("ERROR: fork error");
            close(connfd);
        } 
        else if (pid == 0) {
            close(listenfd);
            handle_client(connfd);
            close(connfd);
            exit(0); 
        } 
        else {
            close(connfd);
        }
    }

    return 0;
}
