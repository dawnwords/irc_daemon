#include "wait_ack_list.h"

wait_ack_list *wait_header, *wait_footer;

void init_wait_ack_list(){
	wait_header = (wait_ack_list*) Calloc(1,sizeof(wait_ack_list));
    wait_footer = (wait_ack_list*) Calloc(1,sizeof(wait_ack_list));
    wait_header->next = wait_footer;
    wait_footer->prev = wait_header;   
}
void printf_wait_list(){
	int i = 0;
	wait_ack_list* temp;
	write_log("====================\n");
	for (temp = wait_header->next; temp != wait_footer; temp = temp->next) {
		write_log("wait_ack_list package %d:\n",i++);
		write_log("target port:%d\n",ntohs(temp->target_addr.sin_port));
		print_package_as_string(&temp->package);
	}
	if(!i)
		write_log("wait_ack_list empty!\n");
	write_log("====================\n");
}

void add_to_wait_ack_list(LSA* package,struct sockaddr_in *target_addr){
	wait_ack_list* new_wait_ack = (wait_ack_list*) Calloc(1,sizeof(wait_ack_list));
	new_wait_ack->last_send = time(NULL);
	// memcpy(&new_wait_ack->target_addr,target_addr,sizeof(struct sockaddr_in));
	// memcpy(&new_wait_ack->package,package,sizeof(LSA));
	new_wait_ack->target_addr = *target_addr;
	new_wait_ack->package = *package;

	new_wait_ack->prev = wait_footer->prev;
	new_wait_ack->next = wait_footer;
	wait_footer->prev->next = new_wait_ack;
	wait_footer->prev = new_wait_ack;

	write_log("add_to_wait_ack_list\n");
	printf_wait_list();
}

void remove_one_frome_wait_ack_list(wait_ack_list* wait_ack_node){
	if(wait_ack_node){		
		wait_ack_node->prev->next = wait_ack_node->next;
		wait_ack_node->next->prev = wait_ack_node->prev;
		Free(wait_ack_node);	
	}
	write_log("remove_from_wait_ack_list\n");
	printf_wait_list();
}

void remove_from_wait_ack_list(LSA* package,struct sockaddr_in *target_addr){
	wait_ack_list* temp;
	for (temp = wait_header->next; temp != wait_footer; temp = temp->next) {
		if(package->sender_id == temp->package.sender_id &&
			package->seq_num >= temp->package.seq_num &&
			equal_addr(target_addr,&temp->target_addr))
			remove_one_frome_wait_ack_list(temp);
	}
	write_log("end remove_from_wait_ack_list\n");
}


int equal_addr(struct sockaddr_in* addr1,struct sockaddr_in* addr2){
	if(addr1 && addr2 && addr1->sin_addr.s_addr == addr2->sin_addr.s_addr &&
		addr1->sin_port == addr2->sin_port)
		return 1;
	return 0;
}