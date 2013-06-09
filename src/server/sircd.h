#ifndef __SIRCD_H__
#define __SIRCD_H__

#include "../common/csapp.h"
#include "../common/rtlib.h"
#include "../common/util.h"
#include "../common/int_list.h"
#include "user.h"
#include "channel.h"
#include "../common/socket.h"

/* command */
#define USER_CMD 0
#define NICK_CMD 1
#define QUIT_CMD 2
#define JOIN_CMD 3
#define PART_CMD 4
#define LIST_CMD 5
#define WHO_CMD 6
#define PRIVMSG_CMD 7
#define UNKONWN_CMD -1

const int ARG_NUM[8] = {4,1,0,1,1,0,1,2};

void check_clients();
void send_command_to_daemon(char const *cmd, char const *param);
void invoke_channel(int connfd,char *msg, channel* c);

/* message handler */
void nick_command(int connfd,char* nick_name);
void user_command(int connfd,char* user_name, char* real_name);
void join_command(int connfd,char* channel_name);
void part_command(int connfd,char* channel_name,int need_writeback);
void quit_command(int index);
void list_command(int connfd);
void who_command(int connfd,char *match);
void privmsg_command(int connfd,char *target,char* msg);
void unknown_command(int connfd, char* cmd);

/* DEBUG FUNCTIONS */
int show_channel(char* result, int max,channel* c);
void debug_list_all_user();
void debug_list_all_channel();


#endif /*__SIRCD_H__*/