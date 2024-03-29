#ifndef __SROUTED_H__
#define __SROUTED_H__

#include "../common/rtlib.h"
#include "../common/csapp.h"
#include "../common/util.h"
#include "../common/log.h"
#include "../common/socket.h"
#include "rtgrading.h"
#include "udp.h"
#include "rtgrading.h"
#include "lsa_list.h"
#include "wait_ack_list.h"
#include "user_cache.h"
#include <stdarg.h>
#include "channel_cache.h"

typedef struct user_struct{
	char name[MAX_NAME_LENGTH];	
	u_long next_hop;
	int distance;
} user_routing_entry;

typedef struct channel_struct{
	char name[MAX_NAME_LENGTH];
	u_long source_node;
	int hop_num;
	u_long next_hops[MAX_LINK_ENTRIES];
} channel_routing_entry;

void init_self_lsa();
void process_incoming_lsa(int udp_fd);

void handle_ADDUSER(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_REMOVEUSER(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_ADDCHAN(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_REMOVECHAN(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_USERTABLE(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_CHANTABLE(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_NEXTHOP(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_NEXTHOPS(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);

void handle_command(char *msg, int connfd, int udp_fd);
void retransmit_ack(int udp_fd);
int is_neighbor( u_long nodeID );
void remove_expired_lsa_and_neighbor(int udp_fd);
void broadcast_neighbor(int sock_fd, LSA *package_to_broadcast, struct sockaddr_in *except_addr);
int is_time_to_advertise(time_t *last_time);
int process_server_cmd(rio_t* rio, int udp_fd);
void broadcast_self(int udp_fd);
#endif /*__SROUTED_H__*/