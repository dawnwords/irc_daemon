#ifndef __ROUTING_TABLE_H__
#define __ROUTING_TABLE_H__

#include "lsa_list.h"
#include "../common/log.h"

typedef struct routing_table_struct{
	u_long dst_id;
	int length;
	u_long path[32];

	struct routing_table_struct* prev;
	struct routing_table_struct* next;
} routing_table;

/*
 * return next_hop for dst in the path with starting at src
 * return -1 on fail
 */
void init_routing_table();
u_long find_next_hop(u_long src,u_long dst,u_long cur);
void find_next_hop_with_distance(u_long cur,u_long dst,u_long* next_hop, int* distance);
void discard_tree();

#endif