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

wait_ack_list* find_wait_ack_node(LSA const *package,struct sockaddr_in const *target_addr){
	wait_ack_list* temp;
	for(temp = wait_header->next;temp != wait_footer;temp = temp->next) {
		if(package->sender_id == temp->package.sender_id &&
			package->seq_num >= temp->package.seq_num &&
			equal_addr(target_addr,&temp->target_addr)){
			return temp;
		}
	}
	return NULL;
}

void add_to_wait_ack_list(LSA const*package,struct sockaddr_in const*target_addr){
	wait_ack_list* wait_ack_node = find_wait_ack_node(package,target_addr);

	if(!wait_ack_node){
		wait_ack_node = (wait_ack_list*) Calloc(1,sizeof(wait_ack_list));

		wait_ack_node->prev = wait_footer->prev;
		wait_ack_node->next = wait_footer;
		wait_footer->prev->next = wait_ack_node;
		wait_footer->prev = wait_ack_node;
	}	
	
	wait_ack_node->last_send = time(NULL);
	wait_ack_node->target_addr = *target_addr;
	wait_ack_node->package = *package;

	write_log("add_to_wait_ack_list\n");
	printf_wait_list();
}

void remove_from_wait_ack_list(LSA const *package,struct sockaddr_in const *target_addr){
	wait_ack_list* wait_ack_node = find_wait_ack_node(package,target_addr);
	if(wait_ack_node){
		wait_ack_node->prev->next = wait_ack_node->next;
		wait_ack_node->next->prev = wait_ack_node->prev;
		Free(wait_ack_node);	
	}
	write_log("remove_from_wait_ack_list\n");
	printf_wait_list();
}

int equal_addr(struct sockaddr_in const *addr1,struct sockaddr_in const*addr2){
	if(addr1 && addr2 && addr1->sin_addr.s_addr == addr2->sin_addr.s_addr &&
		addr1->sin_port == addr2->sin_port)
		return 1;
	return 0;
}

// void add_to_wait_ack_list(LSA const*package,struct sockaddr_in const*target_addr){
// 	wait_ack_list* new_wait_ack = (wait_ack_list*) Calloc(1,sizeof(wait_ack_list));
// 	new_wait_ack->last_send = time(NULL);
// 	// memcpy(&new_wait_ack->target_addr,target_addr,sizeof(struct sockaddr_in));
// 	// memcpy(&new_wait_ack->package,package,sizeof(LSA));
// 	new_wait_ack->target_addr = *target_addr;
// 	new_wait_ack->package = *package;

// 	new_wait_ack->prev = wait_footer->prev;
// 	new_wait_ack->next = wait_footer;
// 	wait_footer->prev->next = new_wait_ack;
// 	wait_footer->prev = new_wait_ack;

// 	write_log("add_to_wait_ack_list\n");
// 	printf_wait_list();
// }

// void remove_one_frome_wait_ack_list(wait_ack_list* wait_ack_node){
// 	if(wait_ack_node){
// 		wait_ack_node->prev->next = wait_ack_node->next;
// 		wait_ack_node->next->prev = wait_ack_node->prev;
// 		Free(wait_ack_node);	
// 	}
// 	write_log("remove_from_wait_ack_list\n");
// 	printf_wait_list();
// }

// void remove_from_wait_ack_list(LSA const *package,struct sockaddr_in const *target_addr){
// 	wait_ack_list* temp = wait_header->next;
// 	while (temp != wait_footer) {
// 		if(package->sender_id == temp->package.sender_id &&
// 			package->seq_num >= temp->package.seq_num &&
// 			equal_addr(target_addr,&temp->target_addr)){
// 			temp = temp->next;
// 			remove_one_frome_wait_ack_list(temp->prev);
// 		}
// 		else
// 			temp = temp->next;
// 	}
// 	write_log("remove_from_wait_ack_list finish\n");
// }