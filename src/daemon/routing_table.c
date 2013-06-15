#include "routing_table.h"
#include "user_cache.h"
routing_table *confirmed_header,*confirmed_footer, *tentative_header, *tentative_footer;


void init_routing_table(){
    confirmed_header = (routing_table*) Calloc(1,sizeof(routing_table));
    confirmed_footer = (routing_table*) Calloc(1,sizeof(routing_table));
    confirmed_header->next = confirmed_footer;
    confirmed_footer->prev = confirmed_header;

    tentative_header = (routing_table*) Calloc(1,sizeof(routing_table));
    tentative_footer = (routing_table*) Calloc(1,sizeof(routing_table));
    tentative_header->next = tentative_footer;
    tentative_footer->prev = tentative_header;    
}

void print_routing_table(){
	routing_table *temp;
	int i;
	write_log("$$$$$$$$$$ routing_table $$$$$$$$$$\n");	
	for(temp = confirmed_header->next;temp != confirmed_footer;temp = temp->next){
		write_log("dst:%lu path[%d]:",temp->dst_id,temp->length);
		for(i = 0;i < temp->length;i++){
			if(i != temp->length - 1)
				write_log("%lu -> ",temp->path[i]);
			else
				write_log("%lu\n",temp->path[i]);
		}
	}
	write_log("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
}

routing_table *insert_before_routing_table(u_long dst_id,int length,u_long path[32],routing_table *element){
	routing_table *new_entry = (routing_table *)Malloc(sizeof(routing_table));
	new_entry->dst_id = dst_id;
	new_entry->length = length;
	int i;
	for(i = 0;i < new_entry->length;i++)
		new_entry->path[i] = path[i];

	new_entry->prev = element->prev;
	new_entry->next = element;
	element->prev->next = new_entry;
	element->prev = new_entry;

	return new_entry;
}

void delete_routing_table(routing_table *element){
	if(element){
		element->prev->next = element->next;
		element->next->prev = element->prev;
		Free(element);	
	}
	
}

void discard_tree(){
	routing_table *temp = confirmed_header->next;
	while(temp != confirmed_footer){
		temp = temp->next;
		Free(temp->prev);
	}
	confirmed_header->next = confirmed_footer;
    confirmed_footer->prev = confirmed_header;
    discard_user_cache();
}

routing_table *find_in_confirmed(u_long src,u_long dst){
	routing_table *temp;
	for(temp = confirmed_header->next;temp != confirmed_footer;temp = temp->next)
		if(temp->dst_id == dst && temp->path[0] == src)
			return temp;
	return NULL;
}

routing_table *find_in_tentative(u_long src,u_long dst){
	routing_table *temp;
	for(temp = tentative_header->next;temp != tentative_footer;temp = temp->next)
		if(temp->dst_id == dst && temp->path[0] == src)
			return temp;
	return NULL;	
}

routing_table *find_min_length_in_tentative(){
	routing_table *temp;
	routing_table *result = NULL;
	int min_length = 32;
	for(temp = tentative_header->next;temp != tentative_footer;temp = temp->next){
		if(temp->length < min_length){
			result = temp;
			min_length = temp->length;
		}
	}
		
	return result;
}

int has_built(u_long src){
	routing_table *temp;
	for(temp = confirmed_header->next;temp != confirmed_footer;temp = temp->next)
		if(temp->path[0] == src)
			return 1;	
	return 0;
}

void build_shortest_path_tree(u_long src){
	/* if the tree starting at src has not been built, start dijkstra algorithm */
	if(!has_built(src)){		
		routing_table *next, *tentative_entry;
		LSA_list* lsa;
		int i, j, new_len;
		u_long neighbor;

		// 1) 2) set next as src
		u_long path[32];
		path[0] = src;
		next = insert_before_routing_table(src,1,path,confirmed_footer);

		while(1){
			// 3) for all neighbors
			lsa = find_LSA_list(next->dst_id);
			for(i = 0;i < lsa->package->num_link_entries;i++){
				neighbor = lsa->package->link_entries[i];
				if(!find_in_confirmed(src,neighbor)){
					next->path[next->length] = neighbor;
					new_len = next->length + 1;
					// a) if neighbor in neither confirmed nor tentative, add to tentative
					if(!(tentative_entry = find_in_tentative(src,neighbor)))
						insert_before_routing_table(neighbor,new_len,next->path,tentative_footer);
					// b) if neighbor in tentative with longer length than new_len, replace it
					else if(tentative_entry->length > new_len){
						tentative_entry->length = new_len;
						for(j = 0;j < new_len;j++)
							tentative_entry->path[j] = next->path[j];
					}								
				}
			}
			// 4) tentative is empty, stop loop
			if(tentative_header->next == tentative_footer)
				break;
			// else add min-length entry to confirmed
			else{
				tentative_entry = find_min_length_in_tentative();
				next = insert_before_routing_table(tentative_entry->dst_id,tentative_entry->length, tentative_entry->path,confirmed_footer);
				delete_routing_table(tentative_entry);
			}
		}
	}
}

u_long find_next_hop(u_long src,u_long dst,u_long cur){
	build_shortest_path_tree(src);

	/* traverse to find next hop to dst*/
	routing_table *temp;
	int i;
	for(temp = confirmed_header->next;temp != confirmed_footer;temp = temp->next)
		if(temp->dst_id == dst && temp->path[0] == src)
			for(i = 0;i < temp->length;i++)
				if(temp->path[i] == cur)
					return temp->path[i+1];
	return 0;
}


void find_next_hop_with_distance(u_long cur,u_long dst,u_long* next_hop, int* distance){
	build_shortest_path_tree(cur);

	print_routing_table();

	/* traverse to find next hop and distance to dst*/	
	routing_table *temp;
	for(temp = confirmed_header->next;temp != confirmed_footer;temp = temp->next){
		if(temp->dst_id == dst && temp->path[0] == cur){
			*next_hop = temp->length > 1 ? temp->path[1] : 0;
			*distance = temp->length - 1;
			return;
		}
	}

	*next_hop = 0;
}