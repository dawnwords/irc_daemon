#ifndef __CHANNEL_CACHE_H__
#define __CHANNEL_CACHE_H__ 

#include "routing_table.h"
#include "lsa_list.h"
#include "srouted.h"

typedef struct channel_cache_item {
	char channelname[MAX_NAME_LENGTH];
	u_long source_id;
	u_long next_hops[MAX_LINK_ENTRIES];
} channel_cache_item_t;

typedef struct channel_cache_list {
	channel_cache_item_t user_item;
	struct channel_cache_list *prev;
	struct channel_cache_list *next;
} channel_cache_list_t;

void init_channel_cache();
channel_cache_list_t * insert_channel_cache_item(u_long sourceID, char *channelname);
void discard_channel_cache();
channel_cache_list_t * find_channel_cache_item(u_long sourceID, char *channelname);

#endif