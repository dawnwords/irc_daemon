#include "user_cache.h"

user_cache_list_t *user_cache_header;
user_cache_list_t *user_cache_footer;

extern u_long curr_nodeID;

void init_user_cache(){
	user_cache_header = (user_cache_list_t*) Calloc(1,sizeof(user_cache_list_t));
    user_cache_footer = (user_cache_list_t*) Calloc(1,sizeof(user_cache_list_t));
    user_cache_header->next = user_cache_footer;
    user_cache_footer->prev = user_cache_header;  
}

void insert_user_cache_item(user_cache_list_t *user_cache_item_p, char *nickname){
	user_cache_item_p = (user_cache_list_t *)Calloc(1,sizeof(user_cache_list_t));
	memcpy(user_cache_item_p->user_item.nickname, nickname, MAX_NAME_LEN);
	find_next_hop(curr_nodeID, dest, &user_cache_item_p->user_item.next_hop, &user_cache_item_p->user_item.distance);

}

void discard_user_cache(){
	user_cache_list_t *temp = user_cache_header->next;
	while(temp != user_cache_footer){
		temp = temp->next;
		Free(temp->prev);
	}

	user_cache_header->next = user_cache_footer;
	user_cache_footer->prev = user_cache_header;

}

void find_user_cache_item(char *nickname) {
	user_cache_list_t *user_cache_item_p;
	for(user_cache_item_p = user_cache_header->next; user_cache_item_p != user_cache_footer; user_cache_item_p = user_cache_item_p->next){
		if(user_cache_item_p->user_item){

		}
	}
}



