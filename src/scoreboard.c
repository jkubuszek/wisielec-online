#include <string.h> 
#include <stdio.h> // file operations
#include <syslog.h> // system logs
#include "scoreboard.h"


#define SC_FILE "scoreboard.csv"
#define TMP_FILE "temp.csv"
// #define DB_FILE "/var/lib/wisielec/scoreboard.csv"

int save_points(const char *username, const int *points) {
    FILE *file;
    FILE *tmp;
    char line[256];
    char *token;
    int find = 0;
    long position = 0;
    char *m_points;
    int m_i_point;

    file = fopen(SC_FILE, "r");
    tmp = fopen(TMP_FILE, "w");
    if (tmp == NULL) {
        if (file) 
        fclose(file);
        
        if (!file && !tmp) {
        syslog(LOG_ERR, "Scoreboard system failure: cannot open %s or %s", SC_FILE, TMP_FILE);
        return 0;
}
        
        return 0;
    }

    // check for user
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            char line_copy[256];
            strcpy(line_copy, line);
            line_copy[strcspn(line_copy, "\n")] = 0;
            token = strtok(line_copy, ";");
            
            
            if (token != NULL) {
                if (strcmp(token, username) == 0) {
                    m_points = strtok(NULL, ";\n");
                    m_i_point  = atoi(m_points);
                    m_i_point += *points;
                    //add points
                    fprintf(tmp, "%s;%d\n", username, m_i_point);
                    find = 1;
                    //return 1; 
                }else{
                    fprintf(tmp, "%s\n", line_copy);
                }
                
                
            }
        }
        if (find==0){            
            fprintf(tmp, "%s;%d\n", username, *points);
        }

        fclose(file);
        fclose(tmp);

        remove(SC_FILE);
        rename(TMP_FILE, SC_FILE);
        return 1;
        
    }

    //here maybe could be return or/and syslog()???
    

     
}

int read_serv_points(char *string, int const size) {
    string[0]="\0";
    FILE *file = fopen(SC_FILE, "r");
    if (file == NULL) {
        syslog(LOG_ERR, "Couldn't open/find users database");
        return 0; 
    }

    char line[256];

    

    while (fgets(line, sizeof(line), file)) {

        // delete \n if found
        line[strcspn(line, "\n")] = 0;
        if ((strlen(string)+strlen(line))<=size){
        sprintf(string, "%s|%s", string, line);
        }
        
    }

    fclose(file);
    sprintf(string, "%s\n", string);
    return 1;
}

int read_player_points(const char *username){
    FILE *file = fopen(SC_FILE, "r");
    if (file == NULL) {
        syslog(LOG_ERR, "Couldn't open/find users database");
        return -1; 
    }

    char line[256];
    int m_points = 0;

    

    while (fgets(line, sizeof(line), file)) {

        // delete \n if found
        line[strcspn(line, "\n")] = 0;
        // getting username
        char *token_user = strtok(line, ";");
        // getting score
        char *token_score = strtok(NULL, ";\n");

        // authentication
        if (token_user != NULL && token_score != NULL) {
            if (strcmp(username, token_user) == 0) {
                m_points = atoi(token_score);
                break; 
            }
        }
    }

    fclose(file);
    return m_points;
}