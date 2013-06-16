#include "srouted.h"

/* Global variables */
extern u_long curr_nodeID;
extern rt_config_file_t   curr_node_config_file;  /* The config_file  for this node */
extern rt_config_entry_t *curr_node_config_entry; /* The config_entry for this node */
extern rt_args_t args;
extern LSA_list *lsa_header, *lsa_footer;         /* header and foot of LAS list */
extern wait_ack_list *wait_header, *wait_footer;  /* header and foot of waiting_ack_list*/

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

/* an array that stores pointer to function */
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
    init_routing_table();
    init_user_cache(); 
    init_channel_cache();
    init_self_lsa();

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

/*
 * given the global self_lsa a default value and insert
 * it into LSA_list for calculating routing table
 */
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
   
    // link entries are given in config_file.entries 
    for(i = 0, j = 0; i < curr_node_config_file.size; i++){
        if((nodeID = curr_node_config_file.entries[i].nodeID) != curr_nodeID){
             self_lsa.link_entries[j++] = nodeID;
        }  
    }

    insert_LSA_list(&self_lsa, NULL);
}

/*
 * remove expired lsa and neighbor
 */
void remove_expired_lsa_and_neighbor(int udp_fd){
    time_t cur_time;
    LSA_list *cur_lsa_p = lsa_header->next;
    u_long lsa_timeout = args.lsa_timeout;
    u_long neighbor_timeout = args.neighbor_timeout;
    long elapsed_time;

    cur_time = time(NULL);

    while(cur_lsa_p != lsa_footer){
        // calculate elapsed time
        elapsed_time = cur_time - cur_lsa_p->receive_time;
        // ignore current node
        if(cur_lsa_p->package->sender_id == curr_nodeID ){
            cur_lsa_p = cur_lsa_p->next;
        }
        // if a neighbor's lsa is not update for neighbor_timeout
        // remove its lsa and broadcast all neighbors to delete its LSA
        else if(elapsed_time >= neighbor_timeout && is_neighbor(cur_lsa_p->package->sender_id)){
            cur_lsa_p->package->ttl = 1;
            broadcast_neighbor(udp_fd, cur_lsa_p->package,NULL);
            cur_lsa_p = cur_lsa_p->next;
            remove_LSA_list(cur_lsa_p->prev);
        }
        // if a lsa is time out, just remove it out of lsa_list
        else if(elapsed_time >= lsa_timeout ){
            cur_lsa_p = cur_lsa_p->next;
            remove_LSA_list(cur_lsa_p->prev);
        }
        // else continue
        else{
            cur_lsa_p = cur_lsa_p->next;
        }
    }
}

/*
 *  return 1 if the given nodeID is a neighbor of current node
 *         0 if not
 */
int is_neighbor( u_long nodeID ){
    int i;
    int num_link_entries = self_lsa.num_link_entries;
    for( i = 0; i < num_link_entries; i++)
        if( nodeID == self_lsa.link_entries[i])
            return 1;
    return 0;
}

/*
 * retransmit all the LSA packages in waiting list,
 * whose ACKs are received in 'retransmission_timeout'
 */
void retransmit_ack(int udp_fd){
    time_t cur_time;
    wait_ack_list *cur_lsa_p;
    u_long retransmission_timeout = args.retransmission_timeout;
    long elapsed_time;
    //get current time
    cur_time = time(NULL);

    for(cur_lsa_p = wait_header->next; cur_lsa_p != wait_footer; cur_lsa_p = cur_lsa_p->next ){
        // calculate elapsed time
        elapsed_time = cur_time - cur_lsa_p->last_send;
        if(elapsed_time >= retransmission_timeout){
            // if ACK receiving is out of time, retransmit the package
            rt_sendto(udp_fd, &cur_lsa_p->package, sizeof(LSA), 0, (SA *)&cur_lsa_p->target_addr, sizeof(struct sockaddr_in));
            // set last send time as current time
            cur_lsa_p->last_send = cur_time;
        }
    }
}

/*
 * broadcast the give LSA package to all neighbors of current node 
 * except the given 'except_addr'
 */
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
        // ignore current node
        if(temp.nodeID != curr_nodeID){
            bzero(&target_addr, sizeof(struct sockaddr_in));
            target_addr.sin_family = AF_INET;
            target_addr.sin_port = htons((unsigned short)temp.routing_port);
            target_addr.sin_addr.s_addr = htonl(temp.ipaddr);

            // if target_addr is not the except_addr, then send to the package to it
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

/*
 * broadcast self_lsa with seq_num increased by 1 to all neighbors
 */
void broadcast_self(int udp_fd){
    self_lsa.seq_num++;
    write_log("broadcast_self\n");
    broadcast_neighbor(udp_fd,&self_lsa,NULL);
}

/*
 * process a incoming lsa
 */
void process_incoming_lsa(int udp_fd){
    LSA* package_in = (LSA*) Calloc(1,sizeof(LSA));
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    /* receive a udp package from other daemon */
    rt_recvfrom(udp_fd, package_in,sizeof(LSA), 0, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
    //debug
    write_log("receive a lsa package\n");
    print_package_as_string(package_in);

    /* if the package is an ack, remove it from waiting_ack_list */
    if(package_in->type){
        //debug
        write_log("receive is a ack\n");
        remove_from_wait_ack_list(package_in, &cli_addr);
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
            write_log("CONTINUE_FLOODING\n");
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

/*
 * process incomming server cmd
 */
int process_server_cmd(rio_t* rio, int udp_fd){
    char buf[MAXLINE];      //current command line

    if(Rio_readlineb(rio,buf,MAXLINE) > 0){
        //debug
        write_log("daemon received command %s\n",buf);
        
        get_msg(buf,buf);
        handle_command(buf,rio->rio_fd,udp_fd);

        /* read the rest server cmd line and process them */
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

/*
 * return 1 if it is time to broadcast self lsa
 *        0 if not
 */
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

/*
 * reply server with message
 */
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
            (handler[i])(connfd, udp_fd, tokens, tokens_num); 
        }
    }
}

/* handler for ADDUSER cmd*/
void handle_ADDUSER(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    // add given nickname to user_entries of self_lsa and expand its length
    strncpy(self_lsa.user_entries[self_lsa.num_user_entries++],tokens[1],MAX_NAME_LENGTH);
    reply(connfd,"OK");
    // after adding, broadcast self_lsa
    broadcast_self(udp_fd);
}

/* handler for REMOVEUSER cmd*/
void handle_REMOVEUSER(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    int i;

    // for each user_entry of self_lsa
    for(i = 0;i<self_lsa.num_user_entries;i++){
        // if the given nickname is found
        if(!strcmp(self_lsa.user_entries[i],tokens[1])){
            // overwrite the current entry with the last entry and decrease num_user_entries by 1
            strncpy(self_lsa.user_entries[i], self_lsa.user_entries[--self_lsa.num_user_entries], MAX_NAME_LENGTH);
            reply(connfd,"OK");
            // after removing, braodcast self_lsa
            broadcast_self(udp_fd); 
            return;
        }
    }
}

/* handler for ADDCHAN cmd */
void handle_ADDCHAN(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    // add given channel name to channel_entries of self_lsa and expand its length
    strncpy(self_lsa.channel_entries[self_lsa.num_channel_entries++],tokens[1],MAX_NAME_LENGTH);
    reply(connfd,"OK");
    // after adding, broadcast self_lsa
    broadcast_self(udp_fd);
}

/* handler for REMOVECHAN cmd */
void handle_REMOVECHAN(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    int i;

    // for each channel_entry of self_lsa
    for(i = 0; i < self_lsa.num_channel_entries; i++){
        // if the given channel name is found
        if(!strcmp(self_lsa.channel_entries[i],tokens[1])){
            // overwrite the current entry with the last entry and decrease num_user_entries by 1
            strncpy(self_lsa.channel_entries[i], self_lsa.channel_entries[--self_lsa.num_channel_entries], MAX_NAME_LENGTH);
            reply(connfd,"OK");
            // after removing, braodcast self_lsa
            broadcast_self(udp_fd);
            return;
        }
    }
}

/* handler for USERTABLE cmd */
void handle_USERTABLE(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    LSA_list *cur_lsa_p;
    int i;
    int total_num_user = 0;
    int length = 0;
    char *nickname;
    char buf[MAX_MSG_LEN];
    char tmp_buf[MAX_MSG_LEN];
    user_cache_list_t * temp;
    LSA *package;

    // for each lsa in lsa_list
    for(cur_lsa_p = lsa_header->next ; cur_lsa_p != lsa_footer; cur_lsa_p = cur_lsa_p->next){
        package = cur_lsa_p->package;
        // ignore self_lsa
        if( package != &self_lsa){
            // for each user entry in lsa package
            for(i = 0; i < package->num_user_entries; i++){
                total_num_user++;
                nickname = package->user_entries[i];
                // try insert the nick name into user_cache_list to get an user_cache_item
                temp = insert_user_cache_item(nickname);
                // add entry to the reply result
                length += snprintf(buf + length, MAX_MSG_LEN - length, "%s %lu %d\n",nickname, temp->user_item.next_hop, temp->user_item.distance);
            }
        }
    }
    // prepend OK size to reply result
    snprintf(tmp_buf, MAX_MSG_LEN, "OK %d\n%s",total_num_user,buf);
    reply(connfd,tmp_buf);
}

/* handler for CHANTABLE cmd */
void handle_CHANTABLE(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    LSA_list *cur_lsa_channel_p;
    int i,j;
    int total_num_channel_item = 0;
    int length = 0;
    char *channel_name;
    char buf[MAX_MSG_LEN];
    char tmp_buf[MAX_MSG_LEN];
    channel_cache_list_t *temp;
    LSA *package;
    u_long sourceID;

    /**********************************************************
     * another version for CHANTABLE implementation which     *
     * satisfies the requirements in IRC_REF but not suitable *
     * for our test script of finalcheckpoint/script.rb       *
     **********************************************************/

    // LSA_list* cur_lsa_sender_p;
    // for(cur_lsa_channel_p = lsa_header->next; cur_lsa_channel_p != lsa_footer; cur_lsa_channel_p = cur_lsa_channel_p->next){
    //    if(cur_lsa_channel_p->package->sender_id != curr_nodeID){
    //        package = cur_lsa_channel_p->package;
    //        for( i = 0; i < package->num_channel_entries; i++){
    //             channel_name = package->channel_entries[i];
    //             for(cur_lsa_sender_p = lsa_header->next; cur_lsa_sender_p != lsa_footer; cur_lsa_sender_p = cur_lsa_sender_p->next){
    //                 sourceID = cur_lsa_sender_p->package->sender_id;
    //                 temp = insert_channel_cache_item(sourceID, channel_name);
    //                 if(temp->channel_item.size){
    //                     total_num_channel_item++;
    //                     length += snprintf(buf + length, MAX_MSG_LEN - length, "%s %lu", channel_name, sourceID);
    //                     for(j = 0; j < temp->channel_item.size; j++){   
    //                         length += snprintf(buf + length, MAX_MSG_LEN - length, " %lu",temp->channel_item.next_hops[j] );
    //                     }
    //                     length += snprintf(buf + length, MAX_MSG_LEN - length, "\n");
    //                 }
    //             }
    //         }
    //     }
    // }

    // for each lsa in lsa_list
    for(cur_lsa_channel_p = lsa_header->next; cur_lsa_channel_p != lsa_footer; cur_lsa_channel_p = cur_lsa_channel_p->next){
       package = cur_lsa_channel_p->package;
       // for each channel entry in lsa
       for( i = 0; i < package->num_channel_entries; i++){
            channel_name = package->channel_entries[i];                
            // source id is the channel container node
            sourceID = cur_lsa_channel_p->package->sender_id;
            // try insert channel_cache_list to get channel_cache_item
            temp = insert_channel_cache_item(sourceID, channel_name);                
            total_num_channel_item++;
            // each line starts with channel name and its container node id
            length += snprintf(buf + length, MAX_MSG_LEN - length, "%s %lu", channel_name, sourceID);
            // append all nexthops after each line
            for(j = 0; j < temp->channel_item.size; j++)
                length += snprintf(buf + length, MAX_MSG_LEN - length, " %lu",temp->channel_item.next_hops[j] );
            length += snprintf(buf + length, MAX_MSG_LEN - length, "\n");            
        }
    }

    // prepend OK size to reply result
    snprintf(tmp_buf, MAX_MSG_LEN, "OK %d\n%s",total_num_channel_item,buf);
    reply(connfd, tmp_buf);
}

/* handler for NEXTHOP cmd */
void handle_NEXTHOP(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    // try insert the nick name into user_cache_list to get an user_cache_item
    user_cache_list_t *temp = insert_user_cache_item(tokens[1]);
    char buf[MAX_MSG_LEN];
    
    // if there is a next_hop result for given nickname reply OK next_hop distance
    if(temp->user_item.next_hop)
        snprintf(buf, MAX_MSG_LEN, "OK %lu %d\n",temp->user_item.next_hop, temp->user_item.distance);
    // else reply NONE
    else
        snprintf(buf, MAX_MSG_LEN, "NONE\n");
    
    reply(connfd,buf);
}

/* handler for NEXTHOPS cmd */
void handle_NEXTHOPS(int connfd, int udp_fd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    u_long sourceID = strtoul(tokens[1],NULL,10);
    char *channel_name = tokens[2];
    int length = 0;
    int i;
    char buf[MAX_MSG_LEN];
    // try insert channel_cache_list to get channel_cache_item
    channel_cache_list_t *channel_cache_list_p = insert_channel_cache_item(sourceID, channel_name);
    
    // if there is a next_hops result for given channel name and source_ID
    if(channel_cache_list_p->channel_item.size){
        channel_cache_item_t channel_item = channel_cache_list_p->channel_item;
        // reply OK nexthop1 nexthop2 ...
        length += snprintf(buf + length, MAX_MSG_LEN - length, "OK");
        for( i = 0; i < channel_cache_list_p->channel_item.size; i ++)
            length += snprintf(buf + length, MAX_MSG_LEN - length, " %lu",channel_item.next_hops[i]);
        length += snprintf(buf + length, MAX_MSG_LEN - length, "\n");
    }
    // else reply NONE
    else
        snprintf(buf, MAX_MSG_LEN, "NONE");    

    reply(connfd, buf);
}