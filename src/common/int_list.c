#include "int_list.h"

void add_int_list(int_list* list, int element){
    int_list* new = (int_list*)malloc(sizeof(int_list));
    new->element = element;
    new->next = list->next;
    list->next = new;
    list->element++;    /* increase the size of the linked list by 1 */
}

int remove_int_list(int_list* list, int element){
    int_list *temp = list->next;
    int_list *next;

    /* only one element left */
    if(!list->next->next){
        Free(temp);
        return --(list->element);        /* decrease the size of the linked list by 1 */
    }

    for(; temp->next->next; temp = temp->next){
        if(element == temp->element){
            next = temp->next;
            temp->element = next->element;
            temp->next = next->next;
            Free(next);
            return --(list->element);    /* decrease the size of the linked list by 1 */
        }
    }

    next = temp->next;

    if(element == temp->element){
        temp->element = next->element;
        temp->next = NULL;
        Free(next);  
        return --(list->element);        /* decrease the size of the linked list by 1 */
    }
    if(next && element == next->element){
        temp->next = NULL;
        Free(next);
        return --(list->element);        /* decrease the size of the linked list by 1 */
    }
    /* not found, return the size of the linked list */
    return list->element; 
}