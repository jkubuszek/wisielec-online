#include <string.h> 
#include <stdio.h> // file operations
#include <syslog.h> // system logs
#include "auth.h"


#define DB_FILE "users.csv"
// #define DB_FILE "/var/lib/wisielec/users.csv"

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

    // check for duplicates
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
        syslog(LOG_ERR, "Couldn't write to users database");
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
        syslog(LOG_ERR, "Couldn't open/find users database");
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