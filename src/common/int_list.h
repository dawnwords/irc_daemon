#ifndef __INT_LIST_H__
#define __INT_LIST_H__

#include "csapp.h"

typedef struct int_list_struct{
    int element;
    struct int_list_struct* next;
} int_list;

void add_int_list(int_list* list, int element);
int remove_int_list(int_list* list, int element);

#endif