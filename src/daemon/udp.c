#include "udp.h"

int init_udp_server_socket(int port){
    int listenfd; 
    struct sockaddr_in serveraddr;

    if ((listenfd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0)
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