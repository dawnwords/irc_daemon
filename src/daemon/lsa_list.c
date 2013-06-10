#include "lsa_list.h"

LSA_list* header,footer;

void init_LSA_list(){
    header = (LSA_list*) Calloc(1,sizeof(LSA_list));
    footer = (LSA_list*) Calloc(1,sizeof(LSA_list));
    header->next = footer;
    footer->prev = header;    
}

void free_LSA_list(LSA_list* LSA){
    if(LSA){
        if(c->package)
            Free(c->package);
        Free(LSA);
    }
}

LSA_list* find_LSA_list(int sender_id){
    LSA_list* temp;
    for(temp = header; temp != footer; temp = temp->next)
        if(temp->package->sender_id == sender_id)
            return temp;
    return NULL;
}

int insert_LSA_list(LSA* new_package){
	LSA_list* old_LSA = find_LSA_list(new_package->sender_id);
	int flag = new_package->seq_num - old_LSA->package->seq_num;

	if(old_LSA){
		if(flag == 0)
			return DISCARD;
		if(flag < 0)
			return SEND_BACK;
		remove_LSA_list(old_LSA);
	} 
	
	LSA_list* new_LSA = (LSA_list*) Calloc(1,sizeof(LSA_list));
	new_LSA->package = new_package;
	ctime(&new_LSA->receive_time);

	new_LSA->prev = footer->prev;
	new_LSA->next = footer;
	footer->prev->next = new_LSA;
	footer->prev = new_LSA;

	return CONTINUE_FLOODING;		
}

int remove_LSA_list(LSA_list* LSA_entry){
	LSA_entry->prev->next = LSA_entry->next;
	LSA_entry->next->prev = LSA_entry->prev;
	free_LSA_list(LSA_entry);
}
