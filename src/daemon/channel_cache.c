#include "channel_cache.h"

channel_cache_list_t *channel_cache_header, *channel_cache_footer;

extern LSA_list *lsa_header, *lsa_footer;
extern u_long curr_nodeID;

void init_channel_cache(){
	channel_cache_header = (channel_cache_list_t*) Calloc(1,sizeof(channel_cache_list_t));
    channel_cache_footer = (channel_cache_list_t*) Calloc(1,sizeof(channel_cache_list_t));
    channel_cache_header->next = channel_cache_footer;
    channel_cache_footer->prev = channel_cache_header;
}

channel_cache_list_t *find_channel_cache_item(u_long source_id,char *channelname){
	channel_cache_list_t *temp;
	for(temp = channel_cache_header->next;temp != channel_cache_footer;temp = temp->next)
		if(temp->source_id == source_id && !strcmp(channelname,temp->channelname))
			return temp;
	return NULL;
}

channel_cache_list_t * insert_channel_cache_item(u_long source_id,char *channelname){
	channel_cache_list_t *channel_cache_item_p = find_channel_cache_item(source_id,channelname);
	
	if(channel_cache_item_p)
		return channel_cache_item_p;

	channel_cache_item_p = (channel_cache_list_t*)Calloc(1,sizeof(channel_cache_list_t));

	LSA_list* temp;
	int i;
	for(temp = lsa_header->next;temp != lsa_footer; temp = temp->next){
		for(i = 0;i < temp->num_channel_entries;i++){
			if(!strcmp(temp->channel_entries[i],channelname)){
				channel_cache_item_p->next_hops[channel_cache_item_p->length++] = find_next_hop(source_id,temp->sender_id,curr_nodeID);
				break;
			}
		}
	}
	return channel_cache_item_p;
}

void discard_channel_cache(){
	channel_cache_list_t *temp = channel_cache_header->next;
	while(temp != channel_cache_footer){
		temp = temp->next;
		Free(temp->prev);
	}

	channel_cache_header->next = channel_cache_footer;
	channel_cache_footer->prev = channel_cache_header;
}

