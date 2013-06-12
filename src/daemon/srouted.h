#ifndef __SROUTED_H__
#define __SROUTED_H__

#include "../common/rtlib.h"
#include "rtgrading.h"
#include "../common/csapp.h"
#include "../common/util.h"
#include "../common/socket.h"
#include "udp.h"
#include "rtgrading.h"
#include "lsa_list.h"
#include "wait_ack_list.h"


#define INCOMING_ADVERTISEMENT 			1
#define IT_IS_TIME_TO_ADVERTISE_ROUTES  2
#define INCOMMING_SERVER_CMD			3

typedef struct user_struct{
	char name[MAX_NAME_LENGTH];
	int source_node;
	int next_hoop;
} user_routing_entry;

typedef struct channel_struct{
	char name[MAX_NAME_LENGTH];
	int next_hoop;
	int distance;
} channel_routing_entry;

void init_self_lsa(int node_id);
void process_incoming_lsa(int udp_fd);

void handle_ADDUSER(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_REMOVEUSER(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_ADDCHAN(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_REMOVECHAN(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_USERTABLE(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_CHANTABLE(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_NEXTHOP(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);
void handle_NEXTHOPS(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num);

void handle_command(char *msg, int connfd);
void retransmit_ack(int udp_fd);
int is_neighbor( u_long nodeID );
void remove_expired_lsa_and_neighbor(int udp_fd);
void broadcast_neightbor(int sock_fd, LSA *package_to_broadcast, struct sockaddr_in *except_addr);
int get_addr_by_nodeID(int nodeID, struct sockaddr_in *target_addr);
int is_time_to_advertise(time_t *last_time);
#endif /*__SROUTED_H__*/