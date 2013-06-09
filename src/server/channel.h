#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "../common/csapp.h"
#include "../common/int_list.h"
#include "../common/util.h"

typedef struct channel_struct{
    char* name;
    int_list* member;
    struct channel_struct* prev;
    struct channel_struct* next;
} channel;

void init_channel();
void free_channel(channel* c);
void remove_channel(channel* channel);
int channel_valid(char* name);
channel* find_channel(char *name);
channel* create_channel(char* name);

#endif