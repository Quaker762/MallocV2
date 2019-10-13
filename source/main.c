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

#define ALLOCS_PER_THREAD   2500
#define STRUCT_ARRAY_SIZE   8000
#define NUMBER_ITERATIONS   1
#define NUMBER_OF_THREADS   12

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

static allocation_type          alloc_type = SMALL;

void print_info()
{
    printf("average allocated size \t= %ld\n", average_allocated_size());
    printf("average free size \t= %ld\n", average_free_size());
    printf("number of blocks \t= %ld\n", number_of_allocated_blocks());
    printf("number of free blocks \t= %ld\n", number_of_free_blocks());
}

/**
 *  Note for Paul!
 * 
 * I was having a few issues with getrusage giving me really fucking stupid values
 * that were greater than 1 billion.... Yeah...
 * 
 * I'm going to be using a profiler/the time bash function to measure the speed of 
 * the execution time of this function/the whole program. 
 */
void* thread_func(void* data)
{
    int             local_count = 0;
    int             len = 0;
    struct          rusage usage;
    struct timeval start, end;
    static char*    nameArr[ALLOCS_PER_THREAD];
    int             size = 0;
    size_t          us = 0;

    for(;;)
    {
        if(local_count++ >= ALLOCS_PER_THREAD)
            break;

        len = rand() % sizeof(insane_struct);

        //getrusage(RUSAGE_SELF, &usage);
        //start = usage.ru_utime;
        nameArr[local_count] = (char*)alloc(len);
        //getrusage(RUSAGE_SELF, &usage);
        //end = usage.ru_utime;

        //us += end.tv_usec - start.tv_usec;
        len = 0;
    }
    //printf("allocation block for thread %ld took %lu us\n", pthread_self(), us);
    return 0;
}

int main(int argc, char **argv)
{
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

    medium_struct*      mediumAllocs[STRUCT_ARRAY_SIZE];
    big_struct*         bigAllocs[STRUCT_ARRAY_SIZE];
    huge_struct*        hugeAllocs[STRUCT_ARRAY_SIZE];
    insane_struct*      insaneAllocs[STRUCT_ARRAY_SIZE];

    printf("Fragmenting base memory allocation list...\n");

    for(int i = 0; i < NUMBER_ITERATIONS; i++)
    {
        // Let's allocate some structs too.
        // Because these are VERY randomised allocations, we need 'array pointers'
        // so we can allocate each array linearly
        int mp=0, bp=0, hp=0, ip=0;
        for(int j = 0; j < STRUCT_ARRAY_SIZE; j++)
        {
            int r = rand() % 5;
            alloc_type = (allocation_type)r;

            switch(alloc_type)
            {
            case MEDIUM:
                if(mp > STRUCT_ARRAY_SIZE)
                    break;
                //printf("allocating a medium struct\n");
                mediumAllocs[mp] = (medium_struct*)alloc(sizeof(medium_struct));
                //printf("that allocation took %ld microseconds!\n", usage.ru_utime.tv_usec - start);
                mp++;
                break;
            case BIG:
                if(bp > STRUCT_ARRAY_SIZE)
                    break;
                //printf("allocating a big struct\n");
                bigAllocs[bp] = (big_struct*)alloc(sizeof(big_struct));
                //printf("that allocation took %ld microseconds!\n", usage.ru_utime.tv_usec - start);
                bp++;
                break;
            case HUGE:
                if(hp > STRUCT_ARRAY_SIZE)
                    break;
                //printf("allocating a huge struct\n");
                hugeAllocs[hp] = (huge_struct*)alloc(sizeof(huge_struct));
                //printf("that allocation took %ld microseconds!\n", usage.ru_utime.tv_usec - start);
                hp++;
                break;
            case INSANE:
                if(ip > STRUCT_ARRAY_SIZE)
                    break;
                //printf("allocating an insane struct\n");
                insaneAllocs[ip] = (insane_struct*)alloc(sizeof(insane_struct));
                //printf("that allocation took %ld microseconds!\n", usage.ru_utime.tv_usec - start);
                ip++;
                break;
            default:
                break;
            }
        }

        for(int j = 0; j < STRUCT_ARRAY_SIZE; j++)
        {
            int r = rand() % 5;
            int slot = 0;
            alloc_type = (allocation_type)r;

            switch(alloc_type)
            {
            case MEDIUM:
                slot = rand() % mp;
                if(mediumAllocs[slot] != NULL)
                {
                    dealloc(mediumAllocs[slot]);
                    mediumAllocs[slot] = NULL;
                }
                break;
            case BIG:
                slot = rand() % bp;
                if(bigAllocs[slot] != NULL)
                {
                    dealloc(bigAllocs[slot]);
                    bigAllocs[slot] = NULL;
                }
                break;
            case HUGE:
                slot = rand() % hp;
                if(hugeAllocs[slot] != NULL)
                {
                    dealloc(hugeAllocs[slot]);
                    hugeAllocs[slot] = NULL;
                }
                break;
            case INSANE:
                slot = rand() % ip;
                if(insaneAllocs[slot] != NULL)
                {
                    dealloc(insaneAllocs[slot]);
                    insaneAllocs[slot] = NULL;
                }
                break;
            default:
                break;
            }
        }
    }

    printf("Done fragmenting the list!\n");

    srand(time(NULL));

    // Let's spawn a bunch of threads
    pthread_t th_ids[NUMBER_OF_THREADS];
    for(int i = 0; i < NUMBER_OF_THREADS; ++i)
    {
        if(pthread_create(&th_ids[i], NULL, thread_func, NULL) != 0)  
        {
            fprintf(stderr, "Can not create thread");
            abort();
        }
    }

    for(int i = 0; i < NUMBER_OF_THREADS; ++i)
    {
        if(pthread_join(th_ids[i], NULL) != 0)
        {
            fprintf(stderr, "Can not join thread");
            abort();
        }
        printf("Thread %lu joined with main.\n", th_ids[i]);
    }

    //print_alloc_list();
    //print_free_list();
    print_info();

    return 0;
}
