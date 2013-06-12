#include "srouted.h"

/* Global variables */
extern u_long curr_nodeID;
extern rt_config_file_t   curr_node_config_file;  /* The config_file  for this node */
extern rt_config_entry_t *curr_node_config_entry; /* The config_entry for this node */
extern rt_args_t args;
extern LSA_list *lsa_header, *lsa_footer;  //header and foot of LAS list not waiting header & footer of waiting_ack_list
extern wait_ack_list *wait_header, *wait_footer;

LSA self_lsa;
user_routing_entry user_routing_table[FD_SETSIZE];
channel_routing_entry channel_routing_table[FD_SETSIZE];

char *command[] = {
    "ADDUSER",
    "REMOVEUSER",
    "ADDCHAN",
    "REMOVECHAN",
    "USERTABLE",
    "CHANTABLE",
    "NEXTHOP",
    "NEXTHOPS"
};

#define N_CMD (sizeof(command)/sizeof(command[0]))  

void (*handler[])(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num) = {
    &handle_ADDUSER,
    &handle_REMOVEUSER,
    &handle_ADDCHAN,
    &handle_REMOVECHAN,
    &handle_USERTABLE,
    &handle_CHANTABLE,
    &handle_NEXTHOP,
    &handle_NEXTHOPS
};

/* Main */
int main( int argc, char *argv[] ) {
    int listen_server_fd, udp_fd, nready,maxfd;
    fd_set read_set;
    struct timeval timeout;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    time_t last_time = 0L;
    rio_t rio;              //rio buffer to store data from server

    //init variable
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    ctime(&last_time);

    // init rio
    rio.rio_fd = 0;

    rt_init(argc, argv);  //must call at beginning
    init_daemon( argc, argv ); // parse command line and fill global variable
    
    listen_server_fd = init_unblocking_server_socket(curr_node_config_entry->local_port);
    udp_fd = init_udp_server_socket(curr_node_config_entry->routing_port);

    /* initialize lsa list */
    init_LSA_list();
    init_wait_ack_list();


    FD_ZERO(&read_set);
    maxfd = listen_server_fd > udp_fd ? listen_server_fd:udp_fd;
    while (1) {
        
        //advertise_cycle_time is up?
        if( is_time_to_advertise(&last_time) ){
            broadcast_neightbor(udp_fd,&self_lsa,NULL);  
        }

        //lsa_timeout & neighbor_timeout is up?
        remove_expired_lsa_and_neighbor(udp_fd);

        //retransmission_timeout is up?
        retransmit_ack(udp_fd);

        FD_SET(listen_server_fd,&read_set);
        FD_SET(udp_fd,&read_set);
        if(rio.rio_fd){
            FD_SET(rio.rio_fd,&read_set);
        }

        if( (nready = Select(maxfd+1, &read_set, NULL, NULL, &timeout)) < 0){
            unix_error("select error in srouted\n");
        }else if(nready > 0){
            //listen_server_fd selected, server ask for a tcp socket connection
            if( FD_ISSET(listen_server_fd,&read_set) ){
                Rio_readinitb(&rio,Accept(listen_server_fd, (SA *)&clientaddr, &clientlen));
                maxfd = maxfd > rio.rio_fd ? maxfd : rio.rio_fd;
            }
            //new command from server INCOMMING_SERVER_CMD
            if(rio.rio_fd && FD_ISSET(rio.rio_fd,&read_set)){
                if(process_server_cmd(&rio)){
                    /* Server EOF */
                    FD_CLR(rio.rio_fd, &read_set);
                    rio.rio_fd = 0;
                }
            }
            //LSA from daemon INCOMMING_ADVERTISEMENT
            if(FD_ISSET(udp_fd,&read_set)){
                process_incoming_lsa(udp_fd);
            }
        }

    }

    return 0;
}

void remove_expired_lsa_and_neighbor(int udp_fd){
    time_t cur_time;
    LSA_list *cur_lsa_p;
    u_long lsa_timeout = args.lsa_timeout;
    u_long neighbor_timeout = args.neighbor_timeout;
    long elapsed_time;

    ctime(&cur_time);

    for(cur_lsa_p = lsa_header->next; cur_lsa_p != lsa_footer; cur_lsa_p = cur_lsa_p->next){
        elapsed_time = cur_time - cur_lsa_p->receive_time;
        if( elapsed_time >= lsa_timeout ){
            remove_LSA_list(cur_lsa_p);      
        }
        if( elapsed_time >= neighbor_timeout && is_neighbor(cur_lsa_p->package->sender_id) ){
            cur_lsa_p->package->ttl = 1;
            broadcast_neightbor(udp_fd, cur_lsa_p->package,NULL);
        }
    }
    
}

int is_neighbor( u_long nodeID ){
    int i;
    int num_link_entries = self_lsa.num_link_entries;
    for( i = 0; i < num_link_entries; i++){
        if( nodeID == self_lsa.link_entries[i]){
            return 1;
        }
    }
    return 0;
}

void retransmit_ack(int udp_fd){
    time_t cur_time;
    wait_ack_list *cur_lsa_p;
    u_long retransmission_timeout = args.retransmission_timeout;
    long elapsed_time;
    ctime(&cur_time);
    for(cur_lsa_p = wait_header->next; cur_lsa_p != wait_header; cur_lsa_p = cur_lsa_p->next ){
        elapsed_time = cur_time - cur_lsa_p->last_send;
        if(elapsed_time >= retransmission_timeout){
            rt_sendto(udp_fd, &cur_lsa_p->package, sizeof(LSA), 0, (SA *)&cur_lsa_p->target_addr, sizeof(struct sockaddr_in));
            ctime(&cur_lsa_p->last_send);
        }
    }
}

void broadcast_neightbor( int udp_sock, LSA *package_to_broadcast, struct sockaddr_in *except_addr){
    int i;
    struct sockaddr_in target_addr;
    int result;
    for(i = 0; i < self_lsa.num_link_entries; i++){
        result = get_addr_by_nodeID(self_lsa.link_entries[i],&target_addr);
        if(result && !except_addr && equal_addr(&target_addr,except_addr)){
            send_to(udp_sock, package_to_broadcast,&target_addr);
        }
    }
}

int get_addr_by_nodeID(int nodeID, struct sockaddr_in *target_addr){
    int i;
    int size = curr_node_config_file.size;
    rt_config_entry_t temp;
    for (i = 0; i < size; i++){
        temp = curr_node_config_file.entries[i];
        if(nodeID == temp.nodeID){
            bzero(target_addr, sizeof(struct sockaddr_in));
            target_addr->sin_family = AF_INET;
            target_addr->sin_port = temp.routing_port;
            target_addr->sin_addr.s_addr = temp.ipaddr;
            return 1;
        }
    }
    return 0;
}


void init_self_lsa(int node_id){
    self_lsa.version = 1;
    self_lsa.ttl = 32;
    self_lsa.type = 0;
    
    self_lsa.sender_id = node_id;
    self_lsa.seq_num = 0;

    self_lsa.num_link_entries = 0;
    self_lsa.num_user_entries = 0;
    self_lsa.num_channel_entries = 0;

    LSA_list* LSA_to_send = NULL;
    insert_LSA_list(&self_lsa, LSA_to_send);
}

void process_incoming_lsa(int udp_fd){
    LSA* package_in = (LSA*) Calloc(1,sizeof(LSA));
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    /* receive a udp package from other daemon */
    rt_recvfrom(udp_fd, package_in,sizeof(LSA), 0, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
    
    /* if the package is an ack */
    if(package_in->type){
        remove_from_wait_ack_list(package_in, &cli_addr);
        return;
    }

    /* send ack back */
    LSA ack;
    ack.type = 1;
    ack.sender_id = package_in->sender_id;
    ack.seq_num = package_in->seq_num;
    send_to(udp_fd,&ack, &cli_addr);

    /* 
     * TTL = 0 and sender is not self, 
     * delete corresponding LSA and broadcast this package to neighbor 
     */
    if(package_in->ttl == 0 && package_in->sender_id != curr_nodeID){
        delete_lsa_by_sender(package_in->sender_id);
        package_in->ttl++;
        broadcast_neightbor(udp_fd, package_in, &cli_addr);
        return;
    }

    /* 
     * receiving a package sent by self only happens when recovering from down 
     * we need to replace self-LSA with the package received
     */
    if(package_in->sender_id == curr_nodeID){
        package_in->ttl = 32;
        memcpy(&self_lsa,package_in,sizeof(LSA));
    }

    /* insert package_in */
    LSA_list* LSA_to_send = NULL;
    switch(insert_LSA_list(package_in,LSA_to_send)){
        case CONTINUE_FLOODING:
            broadcast_neightbor(udp_fd,LSA_to_send->package, &cli_addr);            
            break;
        case DISCARD:
            break;
        case SEND_BACK:
            send_to(udp_fd,LSA_to_send->package, &cli_addr);
            break;
    }   
}

int process_server_cmd(rio_t* rio){
    char buf[MAXLINE];      //current command line

    if(Rio_readlineb(rio,buf,MAXLINE) > 0){
        get_msg(buf,buf);
        handle_command(buf,rio->rio_fd);
        while( rio->rio_cnt > 0 ){
            Rio_readlineb(rio,buf,MAXLINE);
            get_msg(buf,buf);
            handle_command(buf,rio->rio_fd);
        }
        return 0;
    }else{
        /* EOF detected.*/     
        close(rio->rio_fd);
        return 1;
    }
}

int is_time_to_advertise(time_t *last_time){
    time_t cur_time;
    ctime(&cur_time);
    long elapsed_time = cur_time - *last_time;
    if(args.advertisement_cycle_time >= elapsed_time){
        *last_time = cur_time;
        return 1;
    }
    return 0;
}

void reply(int connfd, char const * const message){
    char reply[MAX_MSG_LEN];
    sprintf(reply, "%s\n", message);
    Rio_writen(connfd,reply, strlen(reply));
}


/******************************************
 *         handle server command
 ******************************************/
void handle_command(char *msg, int connfd){
    char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1];
    int tokens_num;
    char *cmd;

    tokens_num = tokenize(msg, tokens);
    cmd = tokens[0];
    //looking for command
    int i;
    for (i = 0; i < N_CMD; ++i){
        if(strcasecmp(cmd, command[i]) == 0){
            //if it is a valide command, use corresponding handler to handle it.
            (handler[i])(connfd, tokens, tokens_num); //an array that stores pointer to function
        }
    }
}

void handle_ADDUSER(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    strncpy(self_lsa.user_entries[self_lsa.num_user_entries++],tokens[1],MAX_NAME_LENGTH);
    reply(connfd,"OK");
}

void handle_REMOVEUSER(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    int i;

    for(i = 0;i<self_lsa.num_user_entries;i++){
        if(!strcmp(self_lsa.user_entries[i],tokens[1])){
            strncpy(self_lsa.user_entries[i],
                self_lsa.user_entries[--self_lsa.num_user_entries],
                MAX_NAME_LENGTH);
            break;
        }
    }

    reply(connfd,"OK");
}

void handle_ADDCHAN(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    reply(connfd,"OK");
}

void handle_REMOVECHAN(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    reply(connfd,"OK");
}

void handle_USERTABLE(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    reply(connfd,"OK 0");
}

void handle_CHANTABLE(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    reply(connfd,"OK 0");
}

void handle_NEXTHOP(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    reply(connfd,"OK");
}

void handle_NEXTHOPS(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    reply(connfd,"OK");   
}


