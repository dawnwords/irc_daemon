/* To compile: gcc sircd.c rtlib.c rtgrading.c csapp.c -lpthread -osircd */
#define _GNU_SOURCE

#include "sircd.h"

int local_client_fd; /*local fd to connection with daemon on the local node.*/

/* Extern Global variables */
extern pool p;

extern channel *header, *footer; /* the header and footer of the linked list of struct channel */
extern user* user_table[FD_SETSIZE]; /* the user table with users locates at its 'clientfd' position */

extern rt_config_file_t   curr_node_config_file;  /* The config_file  for this node */
extern rt_config_entry_t *curr_node_config_entry; /* The config_entry for this node */


int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    user* u;

    /* Init node and set port accoriding to configue file */
    init_node(argc, argv);

    /* Init the linked list of channel and the table of user */
    init_channel();
    init_user();
    
    /* Init server socket and set it as unblocked */
    listenfd = init_unblocking_server_socket(curr_node_config_entry->irc_port);

    /* connect to local daemon */
    local_client_fd = socket_connect(curr_node_config_entry->ipaddr,curr_node_config_entry->local_port);

    /* Init struct pool */
    init_pool();

    add_listen_fd(listenfd);

    while (1) {
        /* Wait for listening/connected descriptor(s) to become ready */
        p.ready_set = p.read_set;
        p.nready = Select(p.maxfd+1, &p.ready_set, NULL, NULL, NULL);

        /* If listening descriptor ready, add new client to pool */
        if (FD_ISSET(listenfd, &p.ready_set)) {
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

            /* create a user struct for the connect fd */
            u = (user*) Calloc(1,sizeof(user));
            user_table[connfd] = u;

            /*set server_name and host_name for user */
            u->host_name = strdup(Gethostbyaddr((const char*)&clientaddr.sin_addr,sizeof(clientaddr.sin_addr),AF_INET)->h_name);
            u->server_name = strdup(Gethostbyname("localhost")->h_name);

            add_client(connfd);
        }

        /* Echo a text line from each ready connected descriptor */
        check_clients();
    }
}

void send_msg_back(int connfd,char* msg){
    //DEBUG
    printf(" # MSG BACK TO %d:\n\t%s\n", connfd,msg);
    rio_writen(connfd,msg,strlen(msg));
}

void request_check(int index, char* buf){
    char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1];
    int arg_num = tokenize(buf,tokens);
    int type,connfd;

    connfd = p.clientfd[index];

    if (!strcmp(tokens[0],"USER"))
        type = USER_CMD;
    else if(!strcmp(tokens[0],"NICK"))
        type = NICK_CMD;
    else if(!strcmp(tokens[0],"JOIN"))
        type = JOIN_CMD;
    else if(!strcmp(tokens[0],"QUIT"))
        type = QUIT_CMD;
    else if(!strcmp(tokens[0],"PART"))
        type = PART_CMD;
    else if(!strcmp(tokens[0],"LIST"))
        type = LIST_CMD;
    else if(!strcmp(tokens[0],"WHO"))
        type = WHO_CMD;
    else if(!strcmp(tokens[0],"PRIVMSG"))
        type = PRIVMSG_CMD;
    else
        type = UNKONWN_CMD;

    //DEBUG
    int i;
    printf(" # CMD:%s\t ARG_NUM:%d\t",tokens[0],arg_num);
    for(i=0;i<4;i++)
        printf(" ARG%d:%s\t",i,tokens[i+1]);
    printf("\n");

    if(type == UNKONWN_CMD)
        unknown_command(connfd,tokens[0]);
    else if(arg_num < ARG_NUM[type]){
        char msg[MAX_MSG_LEN];
        if(type == NICK_CMD)
            /* NICK ERRRO TYPE: ERR_NOERR_NONICKNAMEGIVEN */
            snprintf(msg,MAX_MSG_LEN,":No nickname given\n");
        else if(type == PRIVMSG_CMD){
            /* PRIVMSG ERRRO TYPE: ERR_NORECIPIENT */
            if(!arg_num)
                snprintf(msg,MAX_MSG_LEN,":No recipient given PRIVMSG\n");
            /* PRIVMSG ERRRO TYPE: ERR_NOTEXTTOSEND */
            else
                snprintf(msg,MAX_MSG_LEN,":No text to send\n");
        }else
            snprintf(msg,MAX_MSG_LEN,"%s:Not enough parameters\n", tokens[0]);
        send_msg_back(connfd,msg);
    }
    else
        switch(type){
            case QUIT_CMD:
                quit_command(index);
                break;
            case USER_CMD:
                user_command(connfd,tokens[1],tokens[4]);               
                break;
            case NICK_CMD:
                nick_command(connfd,tokens[1]);
                break;
            default:
                if(check_register(connfd)){
                    switch(type){
                        case JOIN_CMD:
                            join_command(connfd,tokens[1]);
                            break;
                        case PART_CMD:
                            part_command(connfd,tokens[1],1);
                            break;
                        case LIST_CMD:
                            list_command(connfd);
                            break;
                        case WHO_CMD:
                            who_command(connfd,tokens[1]);
                            break;
                        case PRIVMSG_CMD:
                            privmsg_command(connfd,tokens[1],tokens[2]);
                    }
                }else
                    send_msg_back(connfd,":You have not registered\n");
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
            if (Rio_readlineb(&rio, buf, MAXLINE) > 0) {
                
                get_msg(buf,buf);
                request_check(i, buf);

                while( rio.rio_cnt > 0 ){
                    
                    Rio_readlineb(&rio,buf,MAXLINE);
                    get_msg(buf,buf);
                    request_check(i,buf);
                }
            } else{/* EOF detected. User Exit */
                //debug
                char buf[MAX_MSG_LEN];
                show_user(buf,MAX_MSG_LEN,user_table[connfd]);
                printf("!!Exit%s\n", buf);

                /* do quit command for user at connfd[i] */
                quit_command(i);
            }
            //debug
            debug_list_all_user();
            debug_list_all_channel();
            printf("===================================\n");
        }
    }
}

void show_motd(int connfd){
    user* u = check_register(connfd);
    if(u){
        char MOTD[MAX_MSG_LEN];
        int length = 0;

        length += snprintf(MOTD+length, MAX_MSG_LEN-length,":IRC_SERVER 375 %s :- Hello! Message of the day -  \n",u->nick_name);
        length += snprintf(MOTD+length, MAX_MSG_LEN-length,":IRC_SERVER 372 %s :- Welcome to Use IRC Server!\n",u->nick_name);
        length += snprintf(MOTD+length, MAX_MSG_LEN-length,":IRC_SERVER 376 %s :End of /MOTD command\n",u->nick_name);

        send_msg_back(connfd,MOTD);

        send_command_to_daemon("ADDUSER",u->nick_name);
    }
}

void unknown_command(int connfd, char* cmd){
    char msg[MAX_MSG_LEN];
    snprintf(msg, MAX_MSG_LEN, "%s: Unknown command\n",cmd);
    send_msg_back(connfd,msg);
}

void nick_command(int connfd,char* nick_name){
    int length = 0;
    char msg[MAX_MSG_LEN];

    if(!nick_valid(nick_name))
        length += snprintf(msg,MAX_MSG_LEN,"%s:Erroneus nickname\n",nick_name);
    else{
        /* if nick name has not been used, give it to current user */
        if(find_user(nick_name,connfd) < 0){
            user* u = user_table[connfd];
            /* USER without NICK*/
            u->nick_name = strdup(nick_name);
            show_motd(connfd);
        } else
            length += snprintf(msg,MAX_MSG_LEN,"%s:Nickname is already in use\n",nick_name);
    }       

    if(length > 0)
        send_msg_back(connfd,msg);        
}

void user_command(int connfd, char* user_name, char* real_name){
    /* get user struct by connfd */
    user* u = user_table[connfd];
    int length = 0;
    char msg[MAX_MSG_LEN];
    if(u->real_name && u->user_name)
        length += snprintf(msg,MAX_MSG_LEN,":You may not reregister\n");
    else{
        /* set username and realname for u*/
        u->user_name = strdup(user_name);
        u->real_name = strdup(real_name);
        show_motd(connfd);
    }
    
    if(length > 0)
        send_msg_back(connfd,msg);
}

void send_command_to_daemon(char const *cmd, char const *param){
    int length = 0;
    char buffer[MAX_MSG_LEN];
    length += snprintf(buffer+length, MAX_MSG_LEN-length,"%s %s\n",cmd,param);
    send_msg_back(local_client_fd,buffer);
}

void quit_command(int index){
    int connfd = p.clientfd[index];
    user* u;
    if((u = user_table[connfd])){
        
       send_command_to_daemon("REMOVEUSER",u->nick_name);
        
        if(u->located_channel)
            part_command(connfd,u->located_channel->name,0);
        free_user(u);
        user_table[connfd] = NULL;
    }

    close(connfd);
    FD_CLR(connfd, &p.read_set);
    p.clientfd[index] = -1;
}

void join_command(int connfd,char* channel_name){
    int length = 0;
    char back[MAX_MSG_LEN];

    if(channel_valid(channel_name)){
        /* if located channel exisits, part command is needed first */
        user *u = user_table[connfd];
        if(u->located_channel){
            /* JOIN the same channel twice should be ignore */
            if(!strcasecmp(u->located_channel->name,channel_name))
                return;
            /* otherwise should PART the orignal channel first */
            part_command(connfd,u->located_channel->name,1);
        }        

        /* try to find a channel with the name of 'channel_name' */
        channel* c;        
        if(!(c = find_channel(channel_name))){
            /* if no such channel, create a new one */
            c = create_channel(channel_name);
            send_command_to_daemon("ADDCHAN",c->name);
        }
        u->located_channel = c;

        /* echo to all member */
        length += snprintf(back + length, MAX_MSG_LEN - length, ":%s JOIN %s\n",u->nick_name,u->located_channel->name);
        invoke_channel(connfd,back,u->located_channel);

        /* add connfd into the channel list */
        add_int_list(u->located_channel->member,connfd);

        /* write the list in the channel back back */
        int_list* ufd;
        user* channelu;
        for(ufd = u->located_channel->member->next;ufd;ufd = ufd->next){
            channelu = user_table[ufd->element];

            length += snprintf(back + length, MAX_MSG_LEN - length, ":IRC_SERVER 353 %s = %s:%s\n",
                u->nick_name,channelu->located_channel->name,channelu->nick_name);
        }
            
        length += snprintf(back + length, MAX_MSG_LEN - length, ":IRC_SERVER 366 %s %s :End of /NAMES list\n",u->nick_name,u->located_channel->name);
    }else
        snprintf(back, MAX_MSG_LEN, "%s:No such channel\n",channel_name);
   
    send_msg_back(connfd,back);
}

void part_command(int connfd,char* channel_name,int need_writeback){
    char msg[MAX_MSG_LEN];

    /* find user of self */
    user *u = user_table[connfd];
    channel *c = u->located_channel;
    
    if(!find_channel(channel_name))
        snprintf(msg,MAX_MSG_LEN,"%s:No such channel\n",channel_name);
    else if(!c || !c->name || strcasecmp(channel_name,c->name))
        snprintf(msg,MAX_MSG_LEN,"%s:You're not on that channel\n",channel_name);
    else{
        snprintf(msg,MAX_MSG_LEN,":%s!%s@%s QUIT: See You!~\n",u->nick_name,u->user_name,u->host_name);
        /* if the channel has no one left, then remove it out*/
        int size = remove_int_list(c->member,connfd);
        if(size == 0){
            /* send message to daemon of REMOVECHAN */    
            send_command_to_daemon("REMOVECHAN",c->name);
            remove_channel(c);            
        }            
        
        /* if there is still someone at the channel, send a message to tell them 'u' has left */
        else
            invoke_channel(connfd, msg, c); 
        
        /* set located channel to null*/
        u->located_channel = NULL;
    }
    if(need_writeback)
        send_msg_back(connfd,msg);
}

void list_command(int connfd){
    int length = 0;
    char buf[MAX_MSG_LEN];
    channel *c;
    user *self = user_table[connfd];

    length += snprintf(buf + length, MAX_MSG_LEN - length, ":IRC_SERVER 321 %s Channel :Users Name\n",self->nick_name);
    /* traverse all the channels to list its name and number of users */
    for(c = header->next; c->next; c=c->next)
        length += snprintf(buf + length, MAX_MSG_LEN - length, ":IRC_SERVER 322 %s %s %d\n",
                self->nick_name,c->name, c->member->element);

    length += snprintf(buf + length, MAX_MSG_LEN - length, ":IRC_SERVER 323 %s :End of /LIST\n",self->nick_name);
    send_msg_back(connfd,buf);
}

void who_command(int connfd,char *match){    
    int length = 0;
    char buf[MAX_MSG_LEN];
    channel* c;
    user* u;
    user* self = user_table[connfd];

    /* traverse channel list to find name match */
    if((c = find_channel(match))) {
        int_list *temp;
        for (temp = c->member->next;temp;temp = temp->next){
            u = user_table[temp->element];
            length += snprintf(buf + length, MAX_MSG_LEN - length, ":IRC_SERVER 352 %s %s %s %s %s %s H :%d %s\n",
                self->nick_name,u->located_channel->name,u->user_name,u->host_name,u->server_name,u->nick_name,0,u->real_name);
        }
    }

    /* if no such channel matches, traverse users to find name match */
    else {
        int i;
        for (i = 0; i <= p.maxi; i++) {
            u = user_table[p.clientfd[i]];
            if(u && u->user_name && u->server_name && u->host_name && u->real_name && u->nick_name &&
                (!strcasecmp(u->user_name,match) ||!strcasecmp(u->server_name,match) || !strcasecmp(u->host_name,match) ||
                !strcasecmp(u->real_name,match) || !strcasecmp(u->nick_name,match))){
                length += snprintf(buf + length, MAX_MSG_LEN - length, ":IRC_SERVER 352 %s %s %s %s %s %s H :%d %s\n",
                    self->nick_name,u->located_channel->name,u->user_name,u->host_name,u->server_name,u->nick_name,0,u->real_name);
            }
        }
    }

    length += snprintf(buf + length, MAX_MSG_LEN - length, ":IRC_SERVER 315 %s %s :End of /WHO list\n",
        self->nick_name,match);

    send_msg_back(connfd,buf);
}

void privmsg_command(int connfd,char *target_list,char* msg){
    user *u = user_table[connfd];
    char buf[MAX_MSG_LEN];
    channel *c;        
    char *target;
    int tar_fd;

    while((target = strsep(&target_list,","))){
        snprintf(buf,MAX_MSG_LEN,":%s PRIVMSG %s:%s\n",u->nick_name, target, msg);
        /* ignore PRIVMSG to self */
        if(!strcasecmp(target,u->nick_name))
            continue;
        /* if target is a valid channal, send the message to its memembers*/
        if(channel_valid(target) && (c = find_channel(target)))
            invoke_channel(connfd,buf,c);
        /* if target is a valid user, send the message to it */
        else if(nick_valid(target) && (tar_fd = find_user(target,connfd)) >= 0)
            send_msg_back(tar_fd,buf);
        /* illegal target should be returned */
        else{
            snprintf(buf,MAX_MSG_LEN,"%s:No such nick/channel\n",target);
            send_msg_back(connfd,buf);
        }          
    }  
}

void invoke_channel(int connfd,char *msg, channel* c){
    int_list* member;
    if(c)
        for(member=c->member->next;member;member = member->next)
            if(member->element != connfd)
                send_msg_back(member->element,msg);
}

int show_channel(char* result, int max,channel* c){
    int length = 0;

    length += snprintf(result + length, max - length, "\t\t[CHANNEL %s(%i)]\n",c->name,c->member->element);
    if(c->member->element > 0){
        length += snprintf(result + length, max - length, "\t\t\tMember List:\n");

        int_list *temp;
        char user_info[MAX_MSG_LEN];
        for (temp = c->member->next;temp;temp = temp->next){
            show_user(user_info, MAX_MSG_LEN ,user_table[temp->element]);
            length += snprintf(result + length, max - length, "\t\t\t%s\n",user_info);
        }
    }

    return length;
}

void debug_list_all_user(){
    int i, connfd;
    char result[MAX_MSG_LEN];
    //DEBUG
    printf("\t~DEBUG:CURRENT USERS:\n");

    for (i = 0; i <= p.maxi; i++) {
        connfd = p.clientfd[i];
        if(show_user(result,MAX_MSG_LEN, user_table[connfd]))
            printf("\t\tID=%d%s", connfd ,result);
    }
}

void debug_list_all_channel(){
    char result[MAX_MSG_LEN];
    //DEBUG
    printf("\t~DEBUG:CURRENT CHANNELS:\n");

    channel* temp;
    for(temp = header->next; temp->next; temp=temp->next)
        if(show_channel(result,MAX_MSG_LEN, temp))
            printf("%s", result);
}