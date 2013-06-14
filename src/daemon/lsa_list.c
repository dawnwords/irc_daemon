#include "lsa_list.h"

LSA_list *lsa_header, *lsa_footer;

void init_LSA_list(){
    lsa_header = (LSA_list*) Calloc(1,sizeof(LSA_list));
    lsa_footer = (LSA_list*) Calloc(1,sizeof(LSA_list));
    lsa_header->next = lsa_footer;
    lsa_footer->prev = lsa_header;    
}

void free_LSA_list(LSA_list* LSA_entry){
    if(LSA_entry){
        if(LSA_entry->package){	
            Free(LSA_entry->package);
        }
        
        Free(LSA_entry);
    }
}

LSA_list* find_LSA_list(unsigned long sender_id){
    LSA_list* temp;
    for(temp = lsa_header->next; temp != lsa_footer; temp = temp->next)
        if(temp->package->sender_id == sender_id)
            return temp;
    return NULL;
}

int insert_LSA_list(LSA* new_package,LSA* LSA_to_send){
	LSA_list* lsa = find_LSA_list(new_package->sender_id);
	int flag;
	if(lsa){
		flag = new_package->seq_num - lsa->package->seq_num;
		if(flag == 0)
			return DISCARD;
		if(flag < 0){
			LSA_to_send = lsa->package;
			return SEND_BACK;
		}	
	}else{
		lsa = (LSA_list*) Calloc(1,sizeof(LSA_list));
		discard_tree();
	} 
	lsa->package = new_package;
	lsa->receive_time = time(NULL);

	lsa->prev = lsa_footer->prev;
	lsa->next = lsa_footer;
	lsa_footer->prev->next = lsa;
	lsa_footer->prev = lsa;

	LSA_to_send = lsa->package;

	return CONTINUE_FLOODING;		
}

void remove_LSA_list(LSA_list* LSA_entry){
	LSA_entry->prev->next = LSA_entry->next;
	LSA_entry->next->prev = LSA_entry->prev;
	free_LSA_list(LSA_entry);
	discard_tree();
}

void delete_lsa_by_sender(unsigned long sender_id){
	LSA_list* LSA = find_LSA_list(sender_id);
	if(LSA)
		remove_LSA_list(LSA);
}

u_long find_nodeID_by_nickname(char *nickname){
	LSA_list * cur_lsa_p;
	int num_user_entries,i;
	for(cur_lsa_p = lsa_header->next; cur_lsa_p != lsa_footer; cur_lsa_p = cur_lsa_p->next){
		num_user_entries = cur_lsa_p->package->num_user_entries;
		
		for(i = 0; i < num_user_entries; i++){
			if(!strncmp(cur_lsa_p->package->user_entries[i],nickname,MAX_NAME_LENGTH)){
				return cur_lsa_p->package->sender_id;
			} 
		}
	}

	return 0;
}

void print_package_as_string(LSA *package){
    char buf[MAX_MSG_LEN];
    int length = 0;
    length += snprintf(buf + length, MAX_MSG_LEN - length, "{ ttl:%d, type:%s, sender_id:%lu, seq_num:%d, link_entries[",package->ttl, package->type ? "ACK":"LSA", package->sender_id, package->seq_num);
    int i;
    for(i = 0; i < package->num_link_entries; i++){
        length += snprintf(buf + length, MAX_MSG_LEN - length, "%lu,",package->link_entries[i]);
    }  
    length += snprintf(buf + length, MAX_MSG_LEN - length, "], user_entries[");
    for(i = 0; i < package->num_user_entries; i++){
        length += snprintf(buf + length, MAX_MSG_LEN - length, "%s,",package->user_entries[i]);
    }    
    length += snprintf(buf + length, MAX_MSG_LEN - length, "], channel_entries[");
    for(i = 0; i < package->num_channel_entries; i++){
        length += snprintf(buf + length, MAX_MSG_LEN - length, "%s,",package->channel_entries[i]);
    }
    length += snprintf(buf + length, MAX_MSG_LEN - length, "] }\n");

    write_log(buf);
}   