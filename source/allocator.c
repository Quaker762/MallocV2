/**
 * Implementation of allocator.h
 */
#include "allocator.h"
#include "memblk.h"

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//#define ALLOC_DEBUG

static list_t alloc_list    = {NULL, NULL, RWLOCK_INITIALIZER}; // List of blocks 'in use' by the allocator 
static list_t free_list     = {NULL, NULL, RWLOCK_INITIALIZER}; // List blocks that are currently free for use.

static bool init            = false;
static size_t brk_start;
static size_t brk_end;

static pthread_mutex_t brk_lock = PTHREAD_MUTEX_INITIALIZER;
static alloc_method_t  cur_method = ALLOC_FF;

static size_t num_allocated = 0;
static size_t num_free = 0;

void allocator_set_method(alloc_method_t method)
{
    cur_method = method;        
}
void print_alloc_list()
{

    printf("\nalloc_list: ");
    memblk_t* block = alloc_list.head;
    rwlock_rdlock(&alloc_list.lock);
    while(block != NULL)
    {
        //printf("%p(%ld) -> ", (void*)block, block->size);
        printf("block: %p prev: %p next: %p size: %ld,\t data: %p\n",
                (void*)block, (void*)block->prev,
                (void*)block->next, block->size,
                (void*)block->data);

        block = block->next;
    }

    printf("NULL\n");
    printf("alloc_list = %p\n", (void*)alloc_list.head);
    printf("last_allocated = %p\n\n\n", (void*)alloc_list.tail);
    rwlock_unlock(&alloc_list.lock);
}

void print_free_list()
{
    printf("\nfree: ");
    memblk_t* block = free_list.head;
    rwlock_rdlock(&free_list.lock);
    while(block != NULL)
    {
        //printf("%p(%ld) -> ", (void*)block, block->size);
        printf("block: %p prev: %p next: %p size: %ld,\t data: %p\n",
                (void*)block, (void*)block->prev,
                (void*)block->next, block->size,
                (void*)block->data);

        block = block->next;
    }

    printf("NULL\n");
    printf("free = %p\n", (void*)free_list.head);
    printf("free = %p\n\n\n", (void*)free_list.tail);
    rwlock_unlock(&free_list.lock);
}

static void _alloc_init()
{
    if(!init)
    {
        brk_start = (size_t)sbrk(0);
        brk_end = brk_start;
        pthread_mutex_init(&brk_lock, NULL);
        rwlock_init(&alloc_list.lock);
        rwlock_init(&free_list.lock);
        init = true;
    }
}

/**
 * Create a new block by calling sbrk. This allocates enough
 * room for the block, and the size of the void pointer to
 * hold the actual data itself.
 */
static memblk_t* _alloc_create_new_block(size_t size)
{
    memblk_t* block;

#ifdef ALLOC_DEBUG
    printf("_alloc_create_new_block: creating a new block of size %ld\n", size);
#endif
    pthread_mutex_lock(&brk_lock);
    block       = sbrk(sizeof(memblk_t));
    if(block == (void*)-1)
    {
        printf("call to sbrk failed!\n");
        abort();
    }
    block->size = size;
    block->data = sbrk(size);
    block->next = NULL;
    block->prev = NULL;
    pthread_mutex_init(&block->lock, NULL);
    pthread_mutex_unlock(&brk_lock);

#ifdef ALLOC_DEBUG
    printf("_alloc_create_new_block: block at %p chunk at %p\n", (void*)block, block->data);
#endif

    return block;
}

/**
 *  Create a split block
 *
 *  Allocate a new node (using sbrk). The value of block->ptr
 *  is then set to dataptr.
 */
static memblk_t* _alloc_create_split_block(size_t size, void* dataptr)
{
    memblk_t* block;

#ifdef ALLOC_DEBUG
    printf("_alloc_create_split_block: creating a new block of size %ld\n", size);
#endif
    pthread_mutex_lock(&brk_lock);
    block       = sbrk(sizeof(memblk_t));
    if(block == (void*)-1)
    {
        printf("call to sbrk failed!\n");
        abort();
    }
    block->size = size;
    block->data= dataptr;
    block->next = NULL;
    block->prev = NULL;
    pthread_mutex_init(&block->lock, NULL);
    //printf("_alloc_create_split_block: block created at %p\n", (void*)block);
    pthread_mutex_unlock(&brk_lock);
#ifdef ALLOC_DEBUG
    printf("_alloc_create_split_block: block created at %p\n", (void*)block);
#endif

    return block;
}

/**
 * Find first 'size' sized block in the free list.
 * 
 * Iterates over the free list to find a block that first fits the size that we need passed
 * in via the argument 'size'. The block is then locked, and returned to the caller.
 * 
 * NULL is returned in either the situation the requested size cannot be serviced, or
 * all blocks of the requested size are locked.
 */
static memblk_t* find_first_free(size_t size)
{
    memblk_t* block = free_list.head;

    while(block != NULL)
    {
        //printf("block @ %p\n", block);
        if(block->size >= size)
        {
            // Attempt to acquire the lock for this block.
            // This prevents us from acquiring the block twice. If  the block is to be acquired twice, it will be passed to 
            // _update_free_list() while it is currently on the alloc list. This is impossible, and means that the state
            // of the lists has become corrupt.
            if(pthread_mutex_trylock(&block->lock) == 0)
                return block;
        }

        block = block->next;
    }

    return NULL;
}

static void* _alloc_first_fit(size_t size)
{
    memblk_t* found;

    _alloc_init();

    // First, let's check the free list
    rwlock_rdlock(&free_list.lock);
    found = find_first_free(size);
    rwlock_unlock(&free_list.lock);

    if(found != NULL)
    {
        if(found->size >= size)
        {
            int size_diff = found->size - size;
            found->size = size;

            rwlock_wrlock(&free_list.lock);
            // Let's add the split block to the free list
            list_append_block(&free_list, _alloc_create_split_block(size_diff, (((uint8_t*)found->data) + size_diff)));
            rwlock_unlock(&free_list.lock);
            num_free++;
        }

        rwlock_wrlock(&free_list.lock);
        list_delete_block(&free_list, found);
        rwlock_unlock(&free_list.lock);

        // Now let's add the block we found to the allocated list
        rwlock_wrlock(&alloc_list.lock);
        list_append_block(&alloc_list, found);
        pthread_mutex_unlock(&found->lock);
        rwlock_unlock(&alloc_list.lock);
        num_free--;
        return found->data;
    }

    // There doesn't seem to be a block for this size in the free list, so let's
    // create it.
    rwlock_wrlock(&alloc_list.lock);
    memblk_t* block = _alloc_create_new_block(size);
    list_append_block(&alloc_list, block);
    rwlock_unlock(&alloc_list.lock);

    return block->data;
}

static void* _alloc_best_fit(size_t size)
{
    return NULL;
}

static void* _alloc_worst_fit(size_t size)
{
    return NULL;
}

void* alloc(size_t size)
{
    void* ptr;

    if((signed long long int)size < 0)
    {
        printf("alloc: allocation size < 0! Size: %ld\n", size);
        return NULL;
    }

    if(cur_method == ALLOC_FF)
        ptr = _alloc_first_fit(size);
    else if(cur_method == ALLOC_BF)
        ptr = _alloc_best_fit(size);
    else if(cur_method == ALLOC_WF)
        ptr = _alloc_worst_fit(size);
    else
    {
        printf("Unknown allocation strategy! Aborting...\n");
        exit(-1);
    }

    num_allocated++;

#ifdef ALLOC_DEBUG
    printf("alloc: allocated a new pointer at %p\n", ptr);

    print_alloc_list();
    print_free_list();
#endif

    return ptr;
}

void dealloc(void* chunk)
{
    // The user is attempting to deallocate a nullptr!
    if(chunk == NULL)
        return;

    // Let's try and find this block in the allocated list
    rwlock_rdlock(&alloc_list.lock);
    memblk_t* block = alloc_list.head;
    while(block != NULL)
    {
        if(block->data == chunk)
            break;
        
        block = block->next;
    }
    rwlock_unlock(&alloc_list.lock);

    if(block == NULL)
    {
        print_alloc_list();
        print_free_list();
        printf("dealloc(): Unable to find block for pointer %p!\n", chunk);
        abort();
    }
    else
    {
        // At this point, we know that:
        //      The pointer at 'chunk' was a valid allocated pointer
        //      We know where the block is
        // Let's remove this block from the chain
        rwlock_wrlock(&alloc_list.lock);
        list_delete_block(&alloc_list, block);
        rwlock_unlock(&alloc_list.lock);

        // Now let's add it to the free list
        rwlock_wrlock(&free_list.lock);
        list_append_block(&free_list, block);
        rwlock_unlock(&free_list.lock);
    }
}

size_t average_allocated_size()
{
    double      ret;
    double      sum = 0;
    rwlock_rdlock(&alloc_list.lock);
    memblk_t*   block = alloc_list.head;

    while(block != NULL)
    {
        sum += block->size;
        block = block->next;
    }
    rwlock_unlock(&alloc_list.lock);

    ret = floor(sum / num_allocated);

    return (size_t)ret;
}

size_t average_free_size()
{
    double      ret;
    double      sum = 0;
    rwlock_rdlock(&free_list.lock);
    memblk_t*   block = free_list.head;

    while(block != NULL)
    {
        sum += block->size;
        block = block->next;
    }
    rwlock_unlock(&free_list.lock);
    ret = floor(sum / num_allocated);

    return (size_t)ret;
}

size_t number_of_allocated_blocks()
{
    return num_allocated;
}


size_t number_of_free_blocks()
{
    return num_free;
}

void print_free_block_sizes()
{
    rwlock_rdlock(&free_list.lock);
    memblk_t* block = free_list.head;

    while(block != NULL)
    {
        printf("%ld -> ", block->size);
        block = block->next;
    }
    rwlock_unlock(&free_list.lock);
    printf("\n");
}