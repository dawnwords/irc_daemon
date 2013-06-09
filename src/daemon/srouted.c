#include "srouted.h"

/* Global variables */
extern u_long curr_nodeID;
extern rt_config_file_t   curr_node_config_file;  /* The config_file  for this node */
extern rt_config_entry_t *curr_node_config_entry; /* The config_entry for this node */
extern rt_args_t args;

pool p;
int listen_server_fd,send_to_server_fd,event;

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
    init_daemon( argc, argv );
    
    listen_server_fd = init_unblocking_server_socket(curr_node_config_entry->local_port);

    init_pool();

    add_listen_fd(listen_server_fd);

    while (1) {
        wait_for_event();       
        if (event == INCOMING_ADVERTISEMENT){ 
            //process_incoming_advertisements_from_neighbor();         
        }else if (event == IT_IS_TIME_TO_ADVERTISE_ROUTES){
            //advertise_all_routes_to_all_neighbors();
            //check_for_down_neighbors();
            //expire_old_routes();
            //delete_very_old_routes();        
        }
    }

    return 0;
}

void wait_for_event(){
    event = 0;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    while(!event){
        p.ready_set = p.read_set;
        p.nready = Select(p.maxfd+1, &p.ready_set, NULL, NULL, &timeout);
        if (FD_ISSET(listen_server_fd, &p.ready_set)) {
            send_to_server_fd = Accept(listen_server_fd, (SA *)&clientaddr, &clientlen);
            add_client(send_to_server_fd);            
        }
        check_clients();
    }
}

void check_clients() {
    int i, connfd;
    char buf[MAXLINE];
    rio_t rio;

    for (i = 0; (i <= p.maxi) && (p.nready > 0); i++) {
        connfd = p.clientfd[i];
        rio = p.clientrio[i];

        if ((connfd > 0) && FD_ISSET(connfd, &p.ready_set)){
            p.nready--;
            if (rio_readlineb(&rio, buf, MAXLINE) > 0) {
                //DEBUG
                printf("RECEIVE MESSAGE FROM SERVER:%s\n",buf);
                get_msg(buf,buf);
                handle_command(buf, connfd);
            } else{
                /* EOF detected. Server Disconnected. */
                close(connfd);
                FD_CLR(connfd, &p.read_set);
                p.clientfd[i] = -1;
            }
        }
    }
}

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

void reply_ok(int connfd, char const * const message){
    char reply[MAX_MSG_LEN];
    sprintf(reply, "%s\n", message);
    Rio_writen(connfd,reply, strlen(reply));
}

void handle_ADDUSER(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    event = IT_IS_TIME_TO_ADVERTISE_ROUTES;
    reply_ok(connfd,"OK");
}

void handle_REMOVEUSER(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    event = IT_IS_TIME_TO_ADVERTISE_ROUTES;
    reply_ok(connfd,"OK");
}

void handle_ADDCHAN(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    event = IT_IS_TIME_TO_ADVERTISE_ROUTES;
    reply_ok(connfd,"OK");
}

void handle_REMOVECHAN(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    event = IT_IS_TIME_TO_ADVERTISE_ROUTES;
    reply_ok(connfd,"OK");
}

void handle_USERTABLE(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    reply_ok(connfd,"OK 0");
}

void handle_CHANTABLE(int connfd, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1], int tokens_num){
    reply_ok(connfd,"OK 0");
}

