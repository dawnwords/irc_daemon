#include "channel_cache.h"

channel_cache_list_t *channel_cache_header, *channel_cache_footer;

extern LSA_list *lsa_header, *lsa_footer;
extern u_long curr_nodeID;

/* init channel_cache_list, should be invoked at the beginning of main */
void init_channel_cache(){
	channel_cache_header = (channel_cache_list_t*) Calloc(1,sizeof(channel_cache_list_t));
    channel_cache_footer = (channel_cache_list_t*) Calloc(1,sizeof(channel_cache_list_t));
    channel_cache_header->next = channel_cache_footer;
    channel_cache_footer->prev = channel_cache_header;
}

/* return the channel_cache_item with the given channelname and source_id */
channel_cache_list_t *find_channel_cache_item(u_long source_id,char *channelname){
	channel_cache_list_t *temp;
	for(temp = channel_cache_header->next;temp != channel_cache_footer;temp = temp->next)
		if(temp->channel_item.source_id == source_id && !strcmp(channelname,temp->channel_item.channelname))
			return temp;
	return NULL;
}

/* 
 * try to insert channel_cache_item with the given channelname and source_id
 * and return it 
 */
channel_cache_list_t * insert_channel_cache_item(u_long source_id,char *channelname){
	// seaching in cache list
	channel_cache_list_t *channel_cache_item_p = find_channel_cache_item(source_id,channelname);
	
	// if such an item found, return it directly
	if(channel_cache_item_p)
		return channel_cache_item_p;

	// if not, insert an new one
	channel_cache_item_p = (channel_cache_list_t*)Calloc(1,sizeof(channel_cache_list_t));

	LSA_list* temp;
	int i;
	u_long next_hop;
	// for each LSA_entry in the LSA_list
	for(temp = lsa_header->next;temp != lsa_footer; temp = temp->next){
		// ignore self_lsa
		if(temp->package->sender_id != curr_nodeID){
			// for each channel storing in the LSA_engty
			for(i = 0;i < temp->package->num_channel_entries;i++){
				// if it has the same name as the given 
				if(!strcmp(temp->package->channel_entries[i],channelname)){
					// invoke find_next_hop with src = source_id and dst = lsa_sender
					if((next_hop = find_next_hop(source_id,temp->package->sender_id,curr_nodeID)))
						// if there is such an next-hop, insert the next-hop into next-hops array
						channel_cache_item_p->channel_item.next_hops[channel_cache_item_p->channel_item.size++] = next_hop;
					break;
				}
			}
		}
	}
	return channel_cache_item_p;
}

/* delete all channel_cache_item in channel_cache_list */
void discard_channel_cache(){
	channel_cache_list_t *temp = channel_cache_header->next;
	while(temp != channel_cache_footer){
		temp = temp->next;
		Free(temp->prev);
	}

	channel_cache_header->next = channel_cache_footer;
	channel_cache_footer->prev = channel_cache_header;
}

