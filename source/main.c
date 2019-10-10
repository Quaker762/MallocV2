/**
 * COSC1114 Memory Allocator
 */
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include <sys/time.h>
#include <sys/resource.h>

#include "allocator.h"

// sizeof == 8 bytes
typedef struct
{
    int     x;
    char    c;
} small_struct;

// sizeof == 41 bytes (VERY unaligned!)
typedef struct
{
    int x;
    long y;
    char c[21];
    double v;
} __attribute__((packed)) medium_struct;


// sizeof == 113 bytes
typedef struct
{
    char arr[113];
} big_struct;

// sizeof == 512 bytes
typedef struct
{
    char arr[512];
} huge_struct;

// sizeof == 1024 = 1KiBKiB
typedef struct
{
    char arr[1024];
} insane_struct;

typedef enum
{
    SMALL,
    MEDIUM,
    BIG,
    HUGE,
    INSANE
} allocation_type;

#define ALLOCS_PER_THREAD 512

void print_info()
{
    printf("average allocated size \t= %ld\n", average_allocated_size());
    printf("average free size \t= %ld\n", average_free_size());
    printf("number of blocks \t= %ld\n", number_of_allocated_blocks());
    printf("number of free blocks \t= %ld\n", number_of_free_blocks());
}

void *thread_func(void *unused)
{
    void *alloc_ptrs[ALLOCS_PER_THREAD];

    printf("Thread %ld starting.\n", pthread_self());
    for(int i = 0; i < ALLOCS_PER_THREAD; ++i)
    {
        alloc_ptrs[i] = alloc((i+1)*5);
        //printf("Thread %ld: alloc - %p\n", pthread_self(), alloc_ptrs[i]);
    }
    printf("Thread %ld deallocing.\n", pthread_self());
    for(int i = 0; i < ALLOCS_PER_THREAD; ++i)
    {
        //printf("Thread %ld: dealloc - %p\n", pthread_self(), alloc_ptrs[i]);
        dealloc(alloc_ptrs[i]);
    }
    printf("Thread %ld allocing.\n", pthread_self());
    for(int i = 0; i < ALLOCS_PER_THREAD; ++i)
    {
        alloc_ptrs[i] = alloc((i+1)*3);
        //printf("Thread %ld: alloc - %p\n", pthread_self(), alloc_ptrs[i]);
    }
    return 0;
}

#define NO_OF_THREADS 100

int main(int argc, char **argv)
{
    FILE*               names;
    allocation_type     alloc_type = SMALL;
    struct rusage       usage;

    if(argc <= 1)
    {
        printf("usage: alloc <usage_type>\nUsage types are: first, best, worst\n\n");
        exit(-1);
    }

    char* strategy = argv[1];
    for(size_t i = 0; i < strlen(strategy); i++)
        strategy[i] = tolower(strategy[i]);

    if(!strcmp(strategy, "first"))
    {
        allocator_set_method(ALLOC_FF); // Set to the "Best Fit" allocation strategy
    }
    else if(!strcmp(strategy, "best"))
    {
        allocator_set_method(ALLOC_BF); // Set to the "Best Fit" allocation strategy
    }
    else if(!strcmp(strategy, "worst"))
    {
        allocator_set_method(ALLOC_WF); // Set to the "Best Fit" allocation strategy
    }
    else
    {
        printf("invalid strategy %s!\n", strategy);
        exit(-1);
    }

    pthread_t thr_ids[NO_OF_THREADS];

    for(int i = 0; i < NO_OF_THREADS; ++i)
    {
        if(pthread_create(&thr_ids[i], NULL, thread_func, NULL) != 0)  
        {
            perror("Can not create thread");
        }
    }

    for(int i = 0; i < NO_OF_THREADS; ++i)
    {
        if(pthread_join(thr_ids[i], NULL) != 0)
        {
            perror("Can not join thread");
        }
        printf("Thread %lu joined with main.\n", thr_ids[i]);
    }
    return 0;
}
