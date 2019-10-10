/**
 * Memory block data strucure.
 *
 * This is a 'node' for the memory allocator that holds not only the data of the actual pointer, but
 *
 */
#ifndef _MEMBLK_H_
#define _MEMBLK_H_

#include <pthread.h>
#include <stdint.h>
#include <stddef.h>

#include "lock.h"

#define BLOCK_MAGIC 0xcafebabe

/**
 * Memory Block data structure
 *
 * Defines a block of memory as well as its' size.
 */
struct memblk
{
    pthread_mutex_t lock;   // This locks' block. Prevents a "double acquire".
    uint32_t        magic;	// Memblock magic number (to assure that this is a valid memory block!)
    size_t          size;	// The size of this memory block in bytes
    void*           data;	// The actual data stored in this allocated block
    struct memblk*  prev;   // The previous block in the chain
    struct memblk*  next;	// The next memory block in the chain
};

typedef struct memblk memblk_t;

struct list
{
    memblk_t*   head;   /** First list element */
    memblk_t*   tail;   /** Last list element */
    rwlock_t    lock;   /** The lock for this list */
};

typedef struct list list_t;

struct heap
{
    size_t      total_allocated;
    memblk_t*   allocated;
    memblk_t*   free;
};

typedef struct heap heap_t;

/**
 * Append 'block' to the end of the list
 */
void list_append_block(list_t*, memblk_t*);

/**
 * Delete a block from the list
 */
void list_delete_block(list_t*, memblk_t*);

/**
 * Non-allocator related block search
 */
memblk_t* list_find_block(list_t*, memblk_t*);
#endif
