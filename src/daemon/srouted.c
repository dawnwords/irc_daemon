#include "srouted.h"

/* Global variables */
extern u_long curr_nodeID;
extern rt_config_file_t   curr_node_config_file;  /* The config_file  for this node */
extern rt_config_entry_t *curr_node_config_entry; /* The config_entry for this node */
extern rt_args_t args;
extern LSA_list *lsa_header, *lsa_footer;

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
    handle_ADDCHAN,
    &handle_REMOVECHAN,
    &handle_USERTABLE,
    &handle_CHANTABLE,
    &handle_NEXTHOP,
    &handle_NEXTHOPS
};

/* Main */
int main( int argc, char *argv[] ) {
    int listen_server_fd, udp_fd, nready,maxfd;
    int send_to_server_fd = -1;
    fd_set read_set;
    int is_connect_server = 0;  //whether has connected to server
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    time_t last_time;
    rio_t rio;              //rio buffer to store data from server
    char buf[MAXLINE];      //current command line


    rt_init(argc, argv);  //must call at beginning
    init_daemon( argc, argv ); // parse command line and fill global variable
    
    listen_server_fd = init_unblocking_server_socket(curr_node_config_entry->local_port);
    udp_fd = init_udp_server_socket(curr_node_config_entry->routing_port);

    /* initialize lsa list */
    init_LSA_list();

    FD_ZERO(&read_set);
    maxfd = listen_server_fd > udp_fd ? listen_server_fd:udp_fd;
    while (1) {
        
        //IT_IS_TIME_TO_ADVERTISEMENT
        if( is_time_to_advertise(&last_time) ){
            //advertise_all_routes_to_all_neighbors();
            //check_for_down_neighbors();
            //expire_old_routes();
            //delete_very_old_routes();  
        }

        FD_SET(listen_server_fd,&read_set);
        FD_SET(udp_fd,&read_set);
        if(is_connect_server){
            FD_SET(send_to_server_fd,&read_set);
        }

        if( (nready = Select(maxfd+1, &read_set, NULL, NULL, &timeout)) < 0){
            unix_error("select error in srouted\n");
        }else if(nready > 0){
            //listen_server_fd selected, server ask for a tcp socket connection
            if( FD_ISSET(listen_server_fd,&read_set) ){
                send_to_server_fd = Accept(listen_server_fd, (SA *)&clientaddr, &clientlen);
                Rio_readinitb(&rio,send_to_server_fd);
                is_connect_server = 1;
                maxfd = maxfd > send_to_server_fd ? maxfd : send_to_server_fd;
            }
            //new command from server INCOMMING_SERVER_CMD
            if(is_connect_server && FD_ISSET(send_to_server_fd,&read_set)){
                if(Rio_readlineb(&rio,buf,MAXLINE) > 0){
                    get_msg(buf,buf);
                    handle_command(buf,rio.rio_fd);
                    while( rio.rio_cnt > 0 ){
                        Rio_readlineb(&rio,buf,MAXLINE);
                        get_msg(buf,buf);
                        handle_command(buf,rio.rio_fd);
                    }
                }else{/* EOF detected.*/
                    close(send_to_server_fd);
                    FD_CLR(send_to_server_fd, &read_set);
                    is_connect_server = 0;
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

    rt_recvfrom(udp_fd, package_in,sizeof(LSA), 0, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
    
    LSA_list* LSA_to_send = NULL;
    switch(insert_LSA_list(package_in,LSA_to_send)){
        case CONTINUE_FLOODING:{   
            int i;
            rt_config_entry_t config_entry;
            struct sockaddr_in target_addr;
            unsigned short target_port;
            unsigned long target_ip;

            for (i = 0; i < curr_node_config_file.size; i++){
                config_entry = curr_node_config_file.entries[i];
                // ignore self
                if(config_entry.nodeID == curr_nodeID)
                    continue;

                target_port = htons(config_entry.routing_port);
                target_ip = htonl(config_entry.ipaddr);

                // ignore sender
                if(target_ip == cli_addr.sin_addr.s_addr && 
                    target_port == cli_addr.sin_port)                    
                    continue;

                memset(&target_addr, '\0', sizeof(target_addr));
                target_addr.sin_family = AF_INET;
                target_addr.sin_addr.s_addr = target_ip;
                target_addr.sin_port = target_port;
                rt_sendto(udp_fd,LSA_to_send,sizeof(LSA),0,(struct sockaddr *)&cli_addr, clilen);
            }
        }
            break;
        case DISCARD:
            break;
        case SEND_BACK:
            // send the LSA with higher seq_num back to the daemon connected
            rt_sendto(udp_fd,LSA_to_send,sizeof(LSA),0,(struct sockaddr *)&cli_addr, clilen);
            break;
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

