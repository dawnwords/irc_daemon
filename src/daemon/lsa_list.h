#ifndef __LSA_LIST_H__
#define __LSA_LIST_H__

#include "../common/csapp.h"
#include "../common/rtlib.h"
#include "../common/util.h"
#include "../common/log.h"
#include "routing_table.h"


#define CONTINUE_FLOODING 	1
#define DISCARD				0
#define SEND_BACK			-1

#define MAX_LINK_ENTRIES MAX_CONFIG_FILE_LINES

typedef struct LSA_t {
	char version;
	char ttl;
	short type;

	unsigned long sender_id;
	int seq_num;

	int num_link_entries;
	int num_user_entries;
	int num_channel_entries;

	unsigned long link_entries[MAX_LINK_ENTRIES];
	char user_entries[FD_SETSIZE][MAX_NAME_LENGTH];
	char channel_entries[FD_SETSIZE][MAX_NAME_LENGTH];
} LSA;

typedef struct LSA_list_struct{
	time_t receive_time;
    LSA* package;    
    struct LSA_list_struct* prev;
    struct LSA_list_struct* next;
} LSA_list;

void init_LSA_list();
void free_LSA_list(LSA_list* LSA);
LSA_list* find_LSA_list(unsigned long sender_id);
int insert_LSA_list(LSA* new_package,LSA** LSA_to_send);
void remove_LSA_list(LSA_list* LSA_entry);
void delete_lsa_by_sender(unsigned long sender_id);
u_long find_nodeID_by_nickname(char *nickname);

void print_package_as_string(LSA *package);

#endif /*__LSA_LIST_H__*/