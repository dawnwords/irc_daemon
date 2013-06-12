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
        if(LSA_entry->package)
            Free(LSA_entry->package);
        Free(LSA_entry);
    }
}

LSA_list* find_LSA_list(unsigned long sender_id){
    LSA_list* temp;
    for(temp = lsa_header; temp != lsa_footer; temp = temp->next)
        if(temp->package->sender_id == sender_id)
            return temp;
    return NULL;
}

int insert_LSA_list(LSA* new_package,LSA_list* LSA_to_send){
	LSA_list* old_LSA = find_LSA_list(new_package->sender_id);
	int flag = new_package->seq_num - old_LSA->package->seq_num;

	if(old_LSA){
		if(flag == 0)
			return DISCARD;
		if(flag < 0){
			LSA_to_send = old_LSA;
			return SEND_BACK;
		}			
		remove_LSA_list(old_LSA);
	} 
	
	LSA_list* new_LSA = (LSA_list*) Calloc(1,sizeof(LSA_list));
	new_LSA->package = new_package;
	ctime(&new_LSA->receive_time);

	new_LSA->prev = lsa_footer->prev;
	new_LSA->next = lsa_footer;
	lsa_footer->prev->next = new_LSA;
	lsa_footer->prev = new_LSA;

	LSA_to_send = new_LSA;
	return CONTINUE_FLOODING;		
}

void remove_LSA_list(LSA_list* LSA_entry){
	LSA_entry->prev->next = LSA_entry->next;
	LSA_entry->next->prev = LSA_entry->prev;
	free_LSA_list(LSA_entry);
}

void delete_lsa_by_sender(unsigned long sender_id){
	LSA_list* LSA = find_LSA_list(sender_id);
	if(LSA)
		remove_LSA_list(LSA);
}