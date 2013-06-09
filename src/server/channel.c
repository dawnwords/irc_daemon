#include "channel.h"

channel *header, *footer; /* the header and footer of the linked list of struct channel */

void init_channel(){
    header = (channel*) Calloc(1,sizeof(channel));
    footer = (channel*) Calloc(1,sizeof(channel));
    header->next = footer;
    footer->prev = header;    
}

void free_channel(channel* c){
    if(c){
        if(c->name)
            Free(c->name);
        if(c->member)
            Free(c->member);
        Free(c);
    }
}

void remove_channel(channel* channel){
    channel->prev->next = channel->next;
    channel->next->prev = channel->prev;
    free_channel(channel);
}

int channel_valid_char(char c){
    if(c == ' ' || c == '\7' || c == '\0' || c== '\13' || c== '\10' || c==',')
        return 0;
    return 1;
}

int channel_valid(char* name){
    if(strlen(name)>MAX_NAME_LENGTH || (name[0]!='#' && name[0]!='&'))
        return 0;
    int i;
    for(i=1;i<strlen(name);i++)
        if(!channel_valid_char(name[i]))
            return 0;
    return 1;
}

channel* find_channel(char *name){
    channel* temp;
    for(temp = header; temp != footer; temp = temp->next)
        if(temp->name && !strcasecmp(temp->name, name))
            return temp;
    return NULL;
}

channel* create_channel(char* name){
    if(!name)
        return NULL;

    channel* new_channel;

    if((new_channel = find_channel(name)))
        return new_channel;

    new_channel = (channel*)Malloc(sizeof(channel));
    new_channel->name = strdup(name);
    new_channel->prev = footer->prev;
    new_channel->next = footer;
    new_channel->member = (int_list*)Calloc(1,sizeof(int_list)); 

    /* the number of user is stored at header of int_list*/
    new_channel->prev->next = new_channel;
    footer->prev = new_channel;

    return new_channel;
}
