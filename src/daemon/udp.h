#ifndef __UDP_H__
#define __UDP_H__

#include "../common/csapp.h"
#include "../common/util.h"
#include "rtgrading.h"

#define MAX_LINK_ENTRIES 32

typedef struct LSA_t {
	char version;
	char ttl;
	short type;

	int sender_id;
	int seq_num;

	int num_link_entries;
	int num_user_entries;
	int num_channel_entries;

	int link_entries[MAX_LINK_ENTRIES];
	char user_entries[FD_SETSIZE][MAX_NAME_LENGTH];
	char channel_entries[FD_SETSIZE][MAX_NAME_LENGTH];
} LSA;

int init_udp_server_socket(int port);

#endif /*__UDP_H__*/