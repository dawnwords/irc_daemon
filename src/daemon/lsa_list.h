#ifndef __LSA_LIST_H__
#define __LSA_LIST_H__

#include "../common/csapp.h"
#include "udp.h"

#define CONTINUE_FLOODING 	1
#define DISCARD				0
#define SEND_BACK			-1

typedef struct LSA_list_struct{
	time_t receive_time;
    LSA* package;    
    struct LSA_list_struct* prev;
    struct LSA_list_struct* next;
} LSA_list;

void init_LSA_list();
void free_LSA_list(LSA_list* LSA);
LSA_list* find_LSA_list(int sender_id);
int insert_LSA_list(LSA* new_package,LSA_list* LSA_to_send);
void remove_LSA_list(LSA_list* LSA_entry);

#endif /*__LSA_LIST_H__*/