#include "user_cache.h"

user_cache_list_t *user_cache_header, *user_cache_footer;

extern u_long curr_nodeID;

/* init user cache list, should be invoked at the beginning of main */
void init_user_cache(){
	user_cache_header = (user_cache_list_t*) Calloc(1,sizeof(user_cache_list_t));
    user_cache_footer = (user_cache_list_t*) Calloc(1,sizeof(user_cache_list_t));
    user_cache_header->next = user_cache_footer;
    user_cache_footer->prev = user_cache_header;  
}

/*
 * insert the given nick name into user_cache_list on not existence by 
 * setting nickname as given and invoking find_next_hop_with_distance()
 * to set next_hop and distance
 *
 * if such item exists just return it
 */
user_cache_list_t * insert_user_cache_item(char *nickname){
	user_cache_list_t *user_cache_item_p = NULL ;
	u_long dest;
	if(!(user_cache_item_p = find_user_cache_item(nickname))){
		user_cache_item_p = (user_cache_list_t *)Calloc(1,sizeof(user_cache_list_t));
		memcpy(user_cache_item_p->user_item.nickname, nickname, MAX_NAME_LENGTH);
		dest = find_nodeID_by_nickname(nickname);
		find_next_hop_with_distance(curr_nodeID, dest, &user_cache_item_p->user_item.next_hop, &user_cache_item_p->user_item.distance);
	}

	return user_cache_item_p;
}

/* delete all user_cache_item in user_cache_list */
void discard_user_cache(){
	user_cache_list_t *temp = user_cache_header->next;
	while(temp != user_cache_footer){
		temp = temp->next;
		Free(temp->prev);
	}

	user_cache_header->next = user_cache_footer;
	user_cache_footer->prev = user_cache_header;
}

/* traverse user_cache_list to find a user_cache_item with the given nick name */
user_cache_list_t * find_user_cache_item(char *nickname) {
	user_cache_list_t *user_cache_item_p;
	for(user_cache_item_p = user_cache_header->next; user_cache_item_p != user_cache_footer; user_cache_item_p = user_cache_item_p->next)
		if(!strncmp(user_cache_item_p->user_item.nickname,nickname,MAX_NAME_LENGTH))
			return user_cache_item_p; 
	return NULL;
}
