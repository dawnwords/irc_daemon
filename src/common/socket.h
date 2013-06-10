#ifndef __MY_SOCKET_H__
#define __MY_SOCKET_H__

#include "csapp.h"
#include "rtlib.h"
#include <stdlib.h>

typedef struct {                    /* represents a pool of connected descriptors */
    int maxfd;                      /* largest descriptor in read_set */
    fd_set read_set;                /* set of all active descriptors */
    fd_set ready_set;               /* subset of descriptors ready for reading  */
    int nready;                     /* number of ready descriptors from select */
    int maxi;                       /* highwater index into client array */
    int clientfd[FD_SETSIZE];       /* set of active descriptors */
    rio_t clientrio[FD_SETSIZE];    /* set of active read buffers */
} pool;

int init_unblocking_server_socket(int port);

void unblock_socket(int socketfd);
int socket_connect(unsigned long target_ip, int target_port);
void init_pool();
void add_listen_fd(int listenfd);
void add_client(int connfd);

#endif