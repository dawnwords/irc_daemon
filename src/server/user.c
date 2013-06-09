#include "user.h"

/* Extern Global variables */
extern pool p;

/* the user table with users locates at its 'clientfd' position */
user* user_table[FD_SETSIZE]; 

void free_user(user* u){
    if(u){
        if(u->host_name)
            Free(u->host_name);
        if(u->server_name)
            Free(u->server_name);
        if(u->user_name)
            Free(u->user_name);
        if(u->nick_name)
            Free(u->nick_name);
        if(u->real_name)
            Free(u->real_name);
        Free(u);
    }
}

int show_user(char* result,int max,user* u){
    int length = 0;
    if(u){
        length += snprintf(result + length, max - length, "[user:%s|",u->user_name);
        length += snprintf(result + length, max - length, "server:%s|",u->server_name);
        length += snprintf(result + length, max - length, "host:%s|",u->host_name);
        length += snprintf(result + length, max - length, "nick:%s|",u->nick_name);
        length += snprintf(result + length, max - length, "real:%s]\n",u->real_name);
    }
    return length;
}

int nick_valid_char(char c){
    if((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') ||
        c=='-' || c=='[' || c==']' || c=='\\' || c=='`' || c=='^' || c=='{' || c=='}')
        return 1;
    else
        return 0;
}

int nick_valid(char* name){
    int i;
    if(strlen(name)>MAX_NAME_LENGTH)
        return 0;
    for(i=0;i<strlen(name);i++)
        if(!nick_valid_char(name[i]))
            return 0;
    return 1;
}

user* check_register(int connfd){
    user* u = user_table[connfd];
    if(u->user_name && u->nick_name && u->real_name)
        return u;
    else        
        return 0; 
}

void init_user(){
    /* set user table as all NULL */
    int i;
    for (i = 0; i < FD_SETSIZE; ++i)
        user_table[i] = NULL;
}


int find_user(char* name, int connfd){
    int i;
    user *u;
    for (i = 0; i <= p.maxi; i++) {
        u = user_table[p.clientfd[i]];
        if(u && u->nick_name && p.clientfd[i] != connfd && !strcasecmp(name,u->nick_name))
            return p.clientfd[i];
    }
    return -1;
}