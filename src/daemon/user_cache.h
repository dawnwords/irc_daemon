#ifndef __USER_CACHE_H__
#define __USER_CACHE_H__

#include "routing_table.h"
#include "lsa_list.h"
#include "srouted.h"

typedef struct user_cache_item {
	char nickname[MAX_NAME_LENGTH];
	u_long next_hop;
	int distance;
} user_cache_item_t;

typedef struct user_cache_list {
	user_cache_item_t user_item;
	struct user_cache_list *prev;
	struct user_cache_list *next;
} user_cache_list_t;

void init_user_cache();
user_cache_list_t * insert_user_cache_item(char *nickname);
void discard_user_cache();
user_cache_list_t * find_user_cache_item(char *nickname);
#endif