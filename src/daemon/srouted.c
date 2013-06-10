#include "srouted.h"
#include "rtgrading.h"

/* Global variables */
extern u_long curr_nodeID;
extern rt_config_file_t   curr_node_config_file;  /* The config_file  for this node */
extern rt_config_entry_t *curr_node_config_entry; /* The config_entry for this node */
extern rt_args_t args;

// pool p;


char *command[] = {
    "ADDUSER",
    "REMOVEUSER",
    "ADDCHAN",
    "REMOVECHAN",
    "USERTABLE",
    "CHANTABLE"
};

#define N_CMD (sizeof(command)/sizeof(command[0]))  

void (*handler[])(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num) = {
    &handle_ADDUSER,
    &handle_REMOVEUSER,
    handle_ADDCHAN,
    &handle_REMOVECHAN,
    &handle_USERTABLE,
    &handle_CHANTABLE
};

/* Main */
int main( int argc, char *argv[] ) {
    int listen_server_fd, send_to_server_fd, udp_fd, nready,maxfd;
    fd_set read_set;
    int is_connect_server = 0;  //whether has connected to server
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    time_t last_time;

    rt_init(argc, argv);  //must call at beginning
    init_daemon( argc, argv ); // parse command line and fill global variable
    
    listen_server_fd = init_unblocking_server_socket(curr_node_config_entry->local_port);
    udp_fd = init_udp_server_socket(curr_node_config_entry->routing_port);

    FD_ZERO(&read_set);
    maxfd = max(listen_server_fd,udp_fd);
    while (1) {
        
        //IT_IS_TIME_TO_ADVERTISEMENT
        if( is_time_to_advertise(&last_time) ){
            //advertise_all_routes_to_all_neighbors();         //check_for_down_neighbors();
            //expire_old_routes();
            //delete_very_old_routes();  
        }

        FD_SET(listen_server_fd,&read_set);
        FD_SET(udp_fd,&read_set);
        if(is_connect_server){
            FD_SET(send_to_server_fd,&read_set);
        }

        int nready;
        if( (nready = Select(maxfd+1, &read_set, NULL, NULL, &timeout)) < 0){
            unix_error("select error in srouted\n");
        }else if(nready > 0){
            //listen_server_fd selected, server ask for a tcp socket connection
            if( FD_ISSET(listen_server_fd,&read_set) ){
                send_to_server_fd = Accept(listen_server_fd, (SA *)clientaddr, &clientlen);
                is_connect_server = 1;
            }
            //new command from server INCOMMING_SERVER_CMD
            if(is_connect_server & FD_ISSET(send_to_server_fd,&read_set)){
                
            }
            //LSA from daemon INCOMMING_ADVERTISEMENT
            if(FD_ISSET(udp_fd,&read_set)){
                //process_incoming_advertisements_from_neighbor();
            }
        }

    }

    return 0;
}

int is_time_to_advertise(time_t *last_time){
    time_t cur_time;
    ctime(&cur_time);
    long elapsed_time = cur_time - *last_time;
    if(args->advertisement_cycle_time >= elapsed_time){
        *last_time = cur_time;
        return 1;
    }
    return 0;
}

void reply_ok(int connfd, char const * const message){
    char reply[MAX_MSG_LEN];
    sprintf(reply, "%s\n", message);
    Rio_writen(connfd,reply, strlen(reply));
}


/******************************************
 *         handle server command
 ******************************************
 */
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
    // event = IT_IS_TIME_TO_ADVERTISE_ROUTES;
    reply_ok(connfd,"OK");
}

void handle_REMOVEUSER(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    // event = IT_IS_TIME_TO_ADVERTISE_ROUTES;
    reply_ok(connfd,"OK");
}

void handle_ADDCHAN(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    // event = IT_IS_TIME_TO_ADVERTISE_ROUTES;
    reply_ok(connfd,"OK");
}

void handle_REMOVECHAN(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    // event = IT_IS_TIME_TO_ADVERTISE_ROUTES;
    reply_ok(connfd,"OK");
}

void handle_USERTABLE(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    reply_ok(connfd,"OK 0");
}

void handle_CHANTABLE(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    reply_ok(connfd,"OK 0");
}

