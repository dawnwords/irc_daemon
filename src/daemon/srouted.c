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

void (*handler[])(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num) = {
    &handle_ADDUSER,
    &handle_REMOVEUSER,
    &handle_ADDCHAN,
    &handle_REMOVECHAN,
    &handle_USERTABLE,
    &handle_CHANTABLE,
    &handle_NEXTHOP,
    &handle_NEXTHOPS
};

/******************************************
 *         debug function
 ******************************************/

/* Main */
int main( int argc, char *argv[] ) {
    int listen_server_fd, udp_fd, maxfd;
    fd_set read_set;
    struct timeval timeout;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    time_t last_time = 0L;
    rio_t rio;              //rio buffer to store data from server

    //init variable
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // init rio
    rio.rio_fd = 0;

    rt_init(argc, argv);  //must call at beginning
    init_daemon( argc, argv ); // parse command line and fill global variable
    
    //debug open log file
    init_log();

    listen_server_fd = init_unblocking_server_socket(curr_node_config_entry->local_port);
    udp_fd = init_udp_server_socket(curr_node_config_entry->routing_port);

    /* initialize lsa list */
    init_LSA_list();
    init_wait_ack_list();
    init_user_cache();
    init_routing_table();
    init_self_lsa();
    init_channel_cache();


    maxfd = listen_server_fd > udp_fd ? listen_server_fd:udp_fd;
    
    while (1) {

        //advertise_cycle_time is up?
        if( is_time_to_advertise(&last_time) ){
            //debug
            write_log("time to advertise\n");
            broadcast_self(udp_fd);
        }
        //lsa_timeout & neighbor_timeout is up?
        remove_expired_lsa_and_neighbor(udp_fd);

        retransmit_ack(udp_fd);

        FD_ZERO(&read_set);
        FD_SET(listen_server_fd,&read_set);
        FD_SET(udp_fd,&read_set);
        if(rio.rio_fd){
            FD_SET(rio.rio_fd,&read_set);
        }
        
        if(Select(maxfd+1, &read_set, NULL, NULL, &timeout) > 0){
            //listen_server_fd selected, server ask for a tcp socket connection
            if( FD_ISSET(listen_server_fd,&read_set) ){
                Rio_readinitb(&rio,Accept(listen_server_fd, (SA *)&clientaddr, &clientlen));
                if(maxfd < rio.rio_fd)
                    maxfd = rio.rio_fd;
                //debug
                write_log("server connect at fd:%d, connect fd is %d, maxfd is %d\n",listen_server_fd, rio.rio_fd,maxfd);
            }
            //new command from server INCOMMING_SERVER_CMD
            if(FD_ISSET(rio.rio_fd,&read_set)){
                write_log("server send cmd at fd:%d\n",rio.rio_fd);
                if(process_server_cmd(&rio,udp_fd)){
                    //debug
                    write_log("server EOF\n");
                    
                    /* Server EOF */
                    FD_CLR(rio.rio_fd, &read_set);
                    rio.rio_fd = 0;
                }
            }
            //LSA from daemon INCOMMING_ADVERTISEMENT
            if(FD_ISSET(udp_fd,&read_set)){
                //debug
                write_log("@@@@ process_incoming_lsa @@@@\n");
                process_incoming_lsa(udp_fd);                
                write_log("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
            }
        }
    }

    return 0;
}

void remove_expired_lsa_and_neighbor(int udp_fd){
    time_t cur_time;
    LSA_list *cur_lsa_p = lsa_header->next;
    u_long lsa_timeout = args.lsa_timeout;
    u_long neighbor_timeout = args.neighbor_timeout;
    long elapsed_time;

    cur_time = time(NULL);

    while(cur_lsa_p != lsa_footer){
        elapsed_time = cur_time - cur_lsa_p->receive_time;
        if(cur_lsa_p->package->sender_id == curr_nodeID ){
            cur_lsa_p = cur_lsa_p->next;
        }else if(elapsed_time >= neighbor_timeout && is_neighbor(cur_lsa_p->package->sender_id)){
            cur_lsa_p->package->ttl = 1;
            broadcast_neighbor(udp_fd, cur_lsa_p->package,NULL);
            cur_lsa_p = cur_lsa_p->next;
            remove_LSA_list(cur_lsa_p->prev);
        }else if(elapsed_time >= lsa_timeout ){
            cur_lsa_p = cur_lsa_p->next;
            remove_LSA_list(cur_lsa_p->prev);
        }else{
            cur_lsa_p = cur_lsa_p->next;
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
    cur_time = time(NULL);
    for(cur_lsa_p = wait_header->next; cur_lsa_p != wait_footer; cur_lsa_p = cur_lsa_p->next ){
        elapsed_time = cur_time - cur_lsa_p->last_send;
        if(elapsed_time >= retransmission_timeout){
            rt_sendto(udp_fd, &cur_lsa_p->package, sizeof(LSA), 0, (SA *)&cur_lsa_p->target_addr, sizeof(struct sockaddr_in));
            cur_lsa_p->last_send = time(NULL);
        }
    }
}

void broadcast_neighbor( int udp_sock, LSA *package_to_broadcast, struct sockaddr_in *except_addr){
    int i;
    struct sockaddr_in target_addr;
    int size = curr_node_config_file.size;
    rt_config_entry_t temp;

    //debug log
    write_log("+++++ broadcast_neighbor\n");
    write_log("num_link_entries is %d\n",self_lsa.num_link_entries);

    for(i = 0; i < size; i++){
        temp = curr_node_config_file.entries[i];
        if(temp.nodeID != curr_nodeID){
            bzero(&target_addr, sizeof(struct sockaddr_in));
            target_addr.sin_family = AF_INET;
            target_addr.sin_port = htons((unsigned short)temp.routing_port);
            target_addr.sin_addr.s_addr = htonl(temp.ipaddr);

            if(!equal_addr(&target_addr,except_addr)){
                //debug
                write_log("address to send is ip:%s port:%d\n",inet_ntoa(target_addr.sin_addr), ntohs(target_addr.sin_port));
                write_log("package to send is\n");            
                print_package_as_string(package_to_broadcast);
                send_to(udp_sock, package_to_broadcast, &target_addr);
            }
        }  
    }

    //debug log
    write_log("----- end broadcast_neighbor\n");
}

void broadcast_self(int udp_fd){
    self_lsa.seq_num++;
    write_log("broadcast_self\n");
    broadcast_neighbor(udp_fd,&self_lsa,NULL);
}

void init_self_lsa(){
    int i,j;
    u_long nodeID;

    self_lsa.version = 1;
    self_lsa.ttl = 32;
    self_lsa.type = 0;
    
    self_lsa.sender_id = curr_nodeID;
    self_lsa.seq_num = 0;

    self_lsa.num_link_entries = curr_node_config_file.size-1;
    self_lsa.num_user_entries = 0;
    self_lsa.num_channel_entries = 0;
   
    for(i = 0, j = 0; i < curr_node_config_file.size; i++){
        if((nodeID = curr_node_config_file.entries[i].nodeID) != curr_nodeID){
             self_lsa.link_entries[j++] = nodeID;
        }  
    }

    insert_LSA_list(&self_lsa, NULL);
}

void process_incoming_lsa(int udp_fd){
    LSA* package_in = (LSA*) Calloc(1,sizeof(LSA));
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    /* receive a udp package from other daemon */
    rt_recvfrom(udp_fd, package_in,sizeof(LSA), 0, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
    //debug
    write_log("receive a lsa package\n");
    print_package_as_string(package_in);

    /* if the package is an ack */
    if(package_in->type){
        //debug
        write_log("receive is a ack\n");
        remove_from_wait_ack_list(package_in, &cli_addr);
        write_log("after remove_from_wait_ack_list\n");
        return;
    }

    /* send ack back */
    LSA ack;
    ack.ttl = 32;
    ack.type = 1;
    ack.sender_id = package_in->sender_id;
    ack.seq_num = package_in->seq_num;
     //debug
    write_log("send ack back\n");
    print_package_as_string(&ack);
    rt_sendto(udp_fd, &ack, sizeof(LSA), 0, (SA *)&cli_addr, sizeof(struct sockaddr_in));

    /* 
     * TTL = 0 and sender is not self, 
     * delete corresponding LSA and broadcast this package to neighbor 
     */
    if(package_in->ttl == 0 && package_in->sender_id != curr_nodeID){
        delete_lsa_by_sender(package_in->sender_id);
        package_in->ttl++;
        broadcast_neighbor(udp_fd, package_in, &cli_addr);
        return;
    }

    /* 
     * receiving a package sent by self only happens when recovering from down 
     * we need to replace self-LSA with the package received
     */
    if(package_in->sender_id == curr_nodeID){
        package_in->ttl = 32;
        // memcpy(&self_lsa,package_in,sizeof(LSA));
        self_lsa = *package_in;
    }

    /* insert package_in */
    LSA* LSA_to_send = NULL;
    switch(insert_LSA_list(package_in,&LSA_to_send)){
        case CONTINUE_FLOODING:            
            write_log("CONTINUE_FLOODING LSA_to_send:%p\n",LSA_to_send);
            print_package_as_string(package_in);
            broadcast_neighbor(udp_fd,LSA_to_send, &cli_addr);            
            break;
        case DISCARD:
            write_log("DISCARD\n");
            break;
        case SEND_BACK:
            write_log("SEND_BACK\n");
            send_to(udp_fd,LSA_to_send, &cli_addr);
            break;
    }
}

int process_server_cmd(rio_t* rio, int udp_fd){
    char buf[MAXLINE];      //current command line

    if(Rio_readlineb(rio,buf,MAXLINE) > 0){
        get_msg(buf,buf);

        //debug
        write_log("daemon received command %s\n",buf);
    
        handle_command(buf,rio->rio_fd,udp_fd);
        while( rio->rio_cnt > 0 ){
            Rio_readlineb(rio,buf,MAXLINE);
            get_msg(buf,buf);
            handle_command(buf,rio->rio_fd, udp_fd);
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
    cur_time = time(NULL);
    long elapsed_time = cur_time - *last_time;
    if(args.advertisement_cycle_time <= elapsed_time){
        *last_time = cur_time;
        return 1;
    }
    return 0;
}

void reply(int connfd, char const * const message){
    char reply[MAX_MSG_LEN];
    sprintf(reply, "%s\n", message);
    write_log("reply to server:%s\n",message);
    Rio_writen(connfd,reply, strlen(reply));
}


/******************************************
 *         handle server command
 ******************************************/
void handle_command(char *msg, int connfd, int udp_fd){
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
            (handler[i])(connfd, udp_fd, tokens, tokens_num); //an array that stores pointer to function
        }
    }
}

void handle_ADDUSER(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    strncpy(self_lsa.user_entries[self_lsa.num_user_entries++],tokens[1],MAX_NAME_LENGTH);
    reply(connfd,"OK");
    broadcast_self(udp_fd);
}

void handle_REMOVEUSER(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
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
    broadcast_self(udp_fd);  
}

void handle_ADDCHAN(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    strncpy(self_lsa.channel_entries[self_lsa.num_channel_entries++],tokens[1],MAX_NAME_LENGTH);
    reply(connfd,"OK");
    broadcast_self(udp_fd);
}

void handle_REMOVECHAN(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    int i;
    for(i = 0; i < self_lsa.num_channel_entries; i++){
        if(!strcmp(self_lsa.channel_entries[i],tokens[1])){
            strncpy(self_lsa.channel_entries[i], 
                self_lsa.channel_entries[--self_lsa.num_channel_entries], 
                MAX_NAME_LENGTH);
            break;
        }
    }
    reply(connfd,"OK");
    broadcast_self(udp_fd);
}

void handle_USERTABLE(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    LSA_list *cur_lsa_p = lsa_header->next;
    int i;
    int total_num_user = 0;
    int length = 0;
    char *nickname;
    char buf[MAX_MSG_LEN];
    char tmp_buf[MAX_MSG_LEN];
    int tmp_length = 0;
    user_cache_list_t * temp;
    LSA * package;

    for(; cur_lsa_p != lsa_footer; cur_lsa_p = cur_lsa_p->next){
        package = cur_lsa_p->package;
        if( package != &self_lsa){
            for(i = 0; i < package->num_user_entries; i++){
                total_num_user++;
                nickname = package->user_entries[i];
                temp = insert_user_cache_item(nickname);
                length += snprintf(buf + length, MAX_MSG_LEN - length, "%s %lu %d\n",nickname, temp->user_item.next_hop, temp->user_item.distance);
            }
        }
    }
    tmp_length += snprintf(tmp_buf+tmp_length, MAX_MSG_LEN - tmp_length, "OK %d\n%s",total_num_user,buf);
    reply(connfd,tmp_buf);
}

void handle_CHANTABLE(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    LSA_list *cur_lsa_channel_p, *cur_lsa_sender_p;
    int i,j;
    int total_num_channel_item = 0;
    int length = 0;
    char *channel_name;
    char buf[MAX_MSG_LEN];
    char tmp_buf[MAX_MSG_LEN];
    int tmp_length = 0;
    channel_cache_list_t *temp;
    LSA *package;
    u_long sourceID;

    for(cur_lsa_channel_p = lsa_header->next; cur_lsa_channel_p != lsa_footer; cur_lsa_channel_p = cur_lsa_channel_p->next){
       package = cur_lsa_channel_p->package;
       for( i = 0; i < package->num_channel_entries; i++){
            channel_name = package->channel_entries[i];
            for(cur_lsa_sender_p = lsa_header->next; cur_lsa_sender_p != lsa_footer; cur_lsa_sender_p = cur_lsa_sender_p->next){
                sourceID = cur_lsa_sender_p->package->sender_id;
                temp = insert_channel_cache_item(sourceID, channel_name);
                if(temp->channel_item.next_hops[0]){
                    total_num_channel_item++;
                    length += snprintf(buf + length, MAX_MSG_LEN - length, "%s %lu", channel_name, sourceID);
                    for(j = 0; j < temp->channel_item.size; j++){   
                        length += snprintf(buf + length, MAX_MSG_LEN - length, " %lu",temp->channel_item.next_hops[j] );
                    }
                    length += snprintf(buf + length, MAX_MSG_LEN - length, "\n");
                }
            }
       }
    }

    tmp_length += snprintf(tmp_buf+tmp_length, MAX_MSG_LEN - tmp_length, "OK %d\n%s",total_num_channel_item,buf);
    reply(connfd, tmp_buf);
}

void handle_NEXTHOP(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    char *nickname = tokens[1];
    user_cache_list_t * temp = insert_user_cache_item(nickname);
    int length = 0;
    char buf[MAX_MSG_LEN];
    
    if(temp->user_item.next_hop){
        length += snprintf(buf + length, MAX_MSG_LEN - length, "OK %lu %d\n",temp->user_item.next_hop, temp->user_item.distance);
    }else{
        length += snprintf(buf + length, MAX_MSG_LEN - length, "NONE\n");
    }
    
    reply(connfd,buf);
}

void handle_NEXTHOPS(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    u_long sourceID = strtoul(tokens[1],NULL,10);
    char *channel_name = tokens[2];
    channel_cache_list_t *channel_cache_list_p;
    int length = 0;
    int i;
    char buf[MAX_MSG_LEN];

    channel_cache_list_p = insert_channel_cache_item(sourceID, channel_name);

    if(channel_cache_list_p->channel_item.next_hops[0]){
        channel_cache_item_t channel_item = channel_cache_list_p->channel_item;
        length += snprintf(buf + length, MAX_MSG_LEN - length, "OK");
        for( i = 0; i < channel_cache_list_p->channel_item.size; i ++){
            length += snprintf(buf + length, MAX_MSG_LEN - length, " %lu",channel_item.next_hops[i]);
        }
        length += snprintf(buf + length, MAX_MSG_LEN - length, "\n");
    }else{
        length += snprintf(buf + length, MAX_MSG_LEN - length, "NONE");
    }

    reply(connfd, buf);
}

