/**
 * Memory allocator interface
 */

#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

#include <stddef.h>

typedef enum
{
    ALLOC_FF,   /** First fit allocation strategy */
    ALLOC_BF,   /** Best fit allocation strategy */
    ALLOC_WF    /** Worst fit allocation strategy */
} alloc_method_t;

/**
 *  Set the desired allocation strategy
 */
void allocator_set_method(alloc_method_t method);

/**
 *  Initialise the allocator
 */
void allocator_init();

/**
 * Allocate a chunk of memory of 'size_t' bytes.
 */
void* alloc(size_t);

/**
 * Deallocate a chunk of memory
 */
void dealloc(void*);




/**
 * The following are a few special functions to help with the report
 * writing, as well as general data gathering
 */

/**
 * Print a list of the size of blocks in the free list
 */
void print_free_block_sizes();

/**
 * Get the average size of an allocated block.
 */
size_t average_allocated_size();

/**
 * Get the average size of a free block.
 */
size_t average_free_size();

/**
 * Get the number of currently allocated blocks
 */
size_t number_of_allocated_blocks();

/**
 * Get the number of free blocks
 */
size_t number_of_free_blocks();

/**
 * Print out information about every block in the allocated list
 */
void print_alloc_list();

/**
 * Print out information about every block in the free list
 */
void print_free_list();



#endif
