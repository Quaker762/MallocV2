/**
 * 
 * 
 */
#include "memblk.h"

void list_append_block(list_t* list, memblk_t* block)
{
    if(list->head == NULL)
    {
        list->head = block;
    }
    
    if(list->tail != NULL)
    {
        list->tail->next = block;
        block->prev = list->tail;
    }
    
    list->tail = block; 
    list->tail->next = NULL;
}

void list_delete_block(list_t* list, memblk_t* block)
{
    if(block == list->head)
    {
        list->head = block->next;
    }

    if(block == list->tail)
    {
        list->tail = block->prev;
    }

    if(block->next != NULL)
    {
        block->next->prev = block->prev;
    }
    if(block->prev != NULL)
    {
        block->prev->next = block->next;
    }

    block->next = NULL;
    block->prev = NULL;
}