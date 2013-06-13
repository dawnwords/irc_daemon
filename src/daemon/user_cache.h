#ifndef __USER_CACHE_H__
#define __USER_CACHE_H__

#include "routing_table.h"
#include "lsa_list.h"
#include "srouted.h"

typedef struct user_cache_item {
	char nickname[MAX_NAME_LEN];
	u_long next_hop;
	int distance;
} user_cache_item_t;

typedef struct user_cache_list {
	user_cache_item_t user_item;
	struct user_cache_list *prev;
	struct user_cache_list *next;
} user_cache_list_t;



#endif