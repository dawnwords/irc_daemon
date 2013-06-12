#ifndef __UDP_H__
#define __UDP_H__

#include "../common/csapp.h"
#include "../common/util.h"
#include "rtgrading.h"
#include "../common/rtlib.h"
#include "lsa_list.h"
#include "wait_ack_list.h"


void send_to(int udp_sock, LSA *package_to_send,struct sockaddr_in *target_addrp);
int init_udp_server_socket(int port);

#endif /*__UDP_H__*/