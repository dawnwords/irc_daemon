#include "lsa_list.h"

LSA_list *lsa_header, *lsa_footer;

/* init lsa_list, should be invoked at the beginning of main */
void init_LSA_list(){
    lsa_header = (LSA_list*) Calloc(1,sizeof(LSA_list));
    lsa_footer = (LSA_list*) Calloc(1,sizeof(LSA_list));
    lsa_header->next = lsa_footer;
    lsa_footer->prev = lsa_header;    
}

/* free an given LSA_list entry*/
void free_LSA_list(LSA_list* LSA_entry){
    if(LSA_entry){
        if(LSA_entry->package){	
            Free(LSA_entry->package);
        }
        
        Free(LSA_entry);
    }
}

/* 
 * return the LSA_list_entry pointer to the given sender_id 
 * return NULL on no such an LSA_entry in the list
 */
LSA_list* find_LSA_list(unsigned long sender_id){
    LSA_list* temp;
    for(temp = lsa_header->next; temp != lsa_footer; temp = temp->next)
        if(temp->package->sender_id == sender_id)
            return temp;
    return NULL;
}

/*
 * return DISCARD 
 *		when given new_package has the same seq_num with that in LSA_list
 * return SEND_BACK 
 * 		when given new_package has a lower seq_num with that in LSA_list
 *			and setting LSA_to_send as the package in LSA_list
 * return CONTINUE_FLOODING when else
 * 		if no such package is found, insert a new item in LSA_list
 *			and setting LSA_to_send as the new_package
 */
int insert_LSA_list(LSA* new_package,LSA** LSA_to_send){
	LSA_list* lsa = find_LSA_list(new_package->sender_id);
	int flag;
	if(lsa){
		flag = new_package->seq_num - lsa->package->seq_num;
		if(flag == 0)
			return DISCARD;
		if(flag < 0){
			if(LSA_to_send)
				*LSA_to_send = lsa->package;
			return SEND_BACK;
		}	
	}else{
		lsa = (LSA_list*) Calloc(1,sizeof(LSA_list));

		lsa->prev = lsa_footer->prev;
		lsa->next = lsa_footer;
		lsa_footer->prev->next = lsa;
		lsa_footer->prev = lsa;
	}
	// invoke discard tree since LSA_list is modified
	discard_tree();	

	lsa->package = new_package;
	lsa->receive_time = time(NULL);

	if(LSA_to_send)
		*LSA_to_send = lsa->package;

	print_lsa_list();
	return CONTINUE_FLOODING;		
}

/* remove an LSA entry in the list */
void remove_LSA_list(LSA_list* LSA_entry){
	LSA_entry->prev->next = LSA_entry->next;
	LSA_entry->next->prev = LSA_entry->prev;
	free_LSA_list(LSA_entry);
	// invoke discard tree since LSA_list is modified
	discard_tree();
	print_lsa_list();
}

/* remove the LSA entry with the given sender_id in the list */
void delete_lsa_by_sender(unsigned long sender_id){
	LSA_list* LSA = find_LSA_list(sender_id);
	if(LSA)
		remove_LSA_list(LSA);
}

/* 
 * return the node id containing the nick name if exists 
 * return 0 if not
 */
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

/****************************************
 *			debug function 				*
 ****************************************/
void format_package(LSA *package, char *buf){
	int length = 0;
    length += snprintf(buf + length, MAX_MSG_LEN - length, "{ ttl:%d, type:%s, sender_id:%lu, seq_num:%d, link_entries[",package->ttl, package->type ? "ACK":"LSA", package->sender_id, package->seq_num);
    int i;
    for(i = 0; i < package->num_link_entries; i++){
        length += snprintf(buf + length, MAX_MSG_LEN - length, "%lu,",package->link_entries[i]);
    }  
    length += snprintf(buf + length, MAX_MSG_LEN - length, "%s","], user_entries[");
    for(i = 0; i < package->num_user_entries; i++){
        length += snprintf(buf + length, MAX_MSG_LEN - length, "%s,",package->user_entries[i]);
    }    
    length += snprintf(buf + length, MAX_MSG_LEN - length, "%s", "], channel_entries[");
    for(i = 0; i < package->num_channel_entries; i++){
        length += snprintf(buf + length, MAX_MSG_LEN - length, "%s,",package->channel_entries[i]);
    }
    length += snprintf(buf + length, MAX_MSG_LEN - length, "%s", "] }");
}

void print_package_as_string(LSA *package){
	if(DEBUG){
	    char buf[MAX_MSG_LEN];
	    format_package(package, buf);
	    write_log("%s\n", buf);	
	}
}

void print_lsa_list(){ 
	if(DEBUG){		
		char pack_buf[MAX_MSG_LEN];
		LSA_list *cur_list_p;
		int i = 0;

		write_log("*********lsa_list*********\n");
		for(cur_list_p = lsa_header->next; cur_list_p != lsa_footer; cur_list_p = cur_list_p->next){
			i++;
			format_package(cur_list_p->package,pack_buf);
			write_log("%s|lsa_%d:%s|\n",ctime(&cur_list_p->receive_time),i,pack_buf);
		}
		write_log("**************************\n");	
	}
}   