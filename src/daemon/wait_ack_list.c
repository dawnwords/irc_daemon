#include "wait_ack_list.h"

wait_ack_list *wait_header, *wait_footer;

/* init wait ack list, should be invoked at the beginning of main */
void init_wait_ack_list(){
	wait_header = (wait_ack_list*) Calloc(1,sizeof(wait_ack_list));
    wait_footer = (wait_ack_list*) Calloc(1,sizeof(wait_ack_list));
    wait_header->next = wait_footer;
    wait_footer->prev = wait_header;   
}

/* debug fucntion */
void printf_wait_list(){
	if(DEBUG){
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
}

/* 
 * traverse waiting_ack_list to find the pointer to waiting_ack_node 
 * with the given packge and target address
 */
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

/*
 * insert the given package and target_addr into waiting_ack_list on not existence
 * if such waiting_ack_node exits, setting last_send to the present and package and
 * target_addr as given
 */
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

/* 
 * remove the waiting_ack_node with the given package 
 * and target_addr from waiting_ack_list 
 */
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

/* 
 * return 1 when addr1 equals to addr2 
 *        0 when not
 */
int equal_addr(struct sockaddr_in const *addr1,struct sockaddr_in const*addr2){
	if(addr1 && addr2 && addr1->sin_addr.s_addr == addr2->sin_addr.s_addr &&
		addr1->sin_port == addr2->sin_port)
		return 1;
	return 0;
}
