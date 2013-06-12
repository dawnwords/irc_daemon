#ifndef __WAITING_ACK_LIST_H__
#define __WAITING_ACK_LIST_H__

#include "../common/csapp.h"
#include "udp.h"

typedef struct ack_list_struct{
	time_t last_send;
	struct sockaddr_in target_addr;
    LSA package;

    struct ack_list_struct* prev;
    struct ack_list_struct* next;
} wait_ack_list;

void init_wait_ack_list();
void add_to_wait_ack_list(LSA* package,struct sockaddr_in target_addr);

/* 	
 *	remove all wait_ack_node with same addr and senderID 
 *	but seq_num less or equal than give package
 */
void remove_from_wait_ack_list(LSA* package,struct sockaddr_in target_addr);
int equal_addr(struct sockaddr_in* addr1,struct sockaddr_in* addr2);

#endif