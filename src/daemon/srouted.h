#ifndef __SROUTED_H__
#define __SROUTED_H__

#include "../common/rtlib.h"
#include "rtgrading.h"
#include "../common/csapp.h"
#include "../common/util.h"
#include "../common/socket.h"


#define INCOMING_ADVERTISEMENT 			1
#define IT_IS_TIME_TO_ADVERTISE_ROUTES  2
#define INCOMMING_SERVER_CMD			3

typedef struct input_param{
	int listen_server_fd;
	int udp_fd;
} input_param_t;

typedef union param{
	input_param_t input; /*input parameter*/
} param_t;

void handle_ADDUSER(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_REMOVEUSER(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_ADDCHAN(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_REMOVECHAN(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_USERTABLE(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_CHANTABLE(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);

void handle_command(char *msg, int connfd);

int wait_for_event(param_t *paramp);
void check_clients();
#endif /*__SROUTED_H__*/