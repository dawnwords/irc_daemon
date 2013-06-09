#include "socket.h"

pool p;

extern u_long curr_nodeID;

int init_udp_server_socket(int port){
    int listenfd; 
    struct sockaddr_in serveraddr;

    if ((listenfd = socket( PF_INET, SOCK_DGRAM, 0 )) < 0)
        unix_error("Creating UDP Socket Error");

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    
    if (bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr))<0) 
        unix_error("UDP Binding Error");

    printf( "UDP on port %d\n", port);

    return listenfd;
}

int init_unblocking_server_socket(int port){    
    /* Init server socket and set it as unblocked */
    int listenfd = Open_listenfd(port);
    unblock_socket(listenfd);

    printf( "I am node %lu and I listen on port %d for new users\n", curr_nodeID, port);

    return listenfd;
}

int socket_connect(unsigned long target_ip, int target_port){
    int sockfd;
    struct sockaddr_in servaddr;

    // create a socket and then connect to the server
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(target_port);
    //inet_aton(target_ip, &servaddr.sin_addr);
    servaddr.sin_addr.s_addr = htonl(target_ip);

    printf( "SERVER IP %s\n", inet_ntoa(servaddr.sin_addr) );

    Connect(sockfd, (struct sockaddr* )&servaddr, sizeof(servaddr));

    // should set sock to be non-blocking
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0){
        perror("fcntl (set non-blocking)");
        exit(1);
    }   

    return sockfd;          
}

void unblock_socket(int socketfd){
     /* flag value for setsockopt */
    int opts = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opts , sizeof(int));

    /* getting current options */
    if ((opts = fcntl(socketfd, F_GETFL)) < 0)
        unix_error("Error on fcntl\n");

    /* modifying and applying */
    opts = (opts | O_NONBLOCK);
    if (fcntl(socketfd, F_SETFL, opts))
        unix_error("Error on fcntl\n");
}

void init_pool() {
    /* Initially, there are no connected descriptors */
    int i;
    p.maxi = -1;
    p.maxfd = -1;
    for (i=0; i< FD_SETSIZE; i++)
        p.clientfd[i] = -1;    

    FD_ZERO(&p.read_set);
}

void add_listen_fd(int listenfd){
    p.maxfd = listenfd > p.maxfd ? listenfd : p.maxfd;    
    FD_SET(listenfd, &p.read_set);
}

void add_client(int connfd) {
    int i;
    p.nready--;
    for (i = 0; i < FD_SETSIZE; i++)  /* Find an available slot */
        if (p.clientfd[i] < 0) {
            /* Add connected descriptor to the pool */
            p.clientfd[i] = connfd;
            rio_readinitb(&p.clientrio[i], connfd);

            /* Add the descriptor to descriptor set */
            FD_SET(connfd, &p.read_set);

            /* Update max descriptor and pool highwater mark */
            if (connfd > p.maxfd)
                p.maxfd = connfd;
            if (i > p.maxi)
                p.maxi = i;
            break;
        }
    if (i == FD_SETSIZE) /* Couldn't find an empty slot */
        app_error("add_client error: Too many clients");
}