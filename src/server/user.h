#ifndef __USER_H__
#define __USER_H__

#include "../common/csapp.h"
#include "channel.h"
#include "../common/util.h"
#include "../common/socket.h"

typedef struct user_struct{
    char* user_name;
    char* nick_name;
    char* host_name;
    char* server_name;
    char* real_name;
    channel* located_channel;
} user;

void init_user();
void free_user(user* u);
int show_user(char* result,int max,user* u);
int nick_valid(char* name);
user* check_register(int connfd);
int find_user(char* name, int connfd);

#endif