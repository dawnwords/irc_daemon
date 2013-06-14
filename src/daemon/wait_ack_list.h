#ifndef __WAITING_ACK_LIST_H__
#define __WAITING_ACK_LIST_H__

#include "../common/csapp.h"
#include "lsa_list.h"
#include "../common/log.h"

typedef struct ack_list_struct{
	time_t last_send;
	struct sockaddr_in target_addr;
    LSA package;

    struct ack_list_struct* prev;
    struct ack_list_struct* next;
} wait_ack_list;

void init_wait_ack_list();
void add_to_wait_ack_list(LSA const*package,struct sockaddr_in const*target_addr);

/* 	
 *	remove all wait_ack_node with same addr and senderID 
 *	but seq_num less or equal than give package
 */
void remove_from_wait_ack_list(LSA const *package,struct sockaddr_in const *target_addr);
int equal_addr(struct sockaddr_in const*addr1,struct sockaddr_in const*addr2);

#endif