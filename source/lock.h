/**
 *  Read write lock implementation
 * 
 *  A read write lock operates on the princple that there are multiple 
 *  readers, as well as multiple writers for pieces of data. As a lot of code these days is multi-threaded 
 *  (meaning more than one function is running concurrently in a single process), it is extremely dangerous
 *  the way that a single piece of globally shared data (such as our free/allocated list) can be edited at any time by any thread. 
 * 
 *  As we do not have control of the scheduler, it could pick any thread at any time. For example, take
 *  a global variable 'd'. Thread A and Thread B both increment d, but A prints for d % 2 and B prints
 *  for d % 1. It is impossible to tell which function will run when, and hence, A or B might miss
 *  turns at printing, and the expected output of A 0, B 1, A 2, B 3... may not occur. This means the
 *  data is corrupt. This is ESPECIALLY bad for a memory allocator, in which the journaling itself may be put
 *  into a corrupt state, and data from another thread may be trampled, which could lead to undefined behaviour
 *  or worse, SIGSEGV and an abort(). 
 * 
 *  A "read write lock" attempts to mitigate this problem by allowing multiple readers of the data (assuming that
 *  no one is currently writing) all the while only allowing one "writer" to modify the data. If a "writer" currently
 *  holds the lock (i.e, num_writers > 0), then all calls to acquire the lock are blocked, and are considered waiting.
 *  The same goes for readers. If num_readers > 0, and a writer attempts to acquire the lock, then that call is 
 *  blocked until 
 */

#ifndef _LOCK_H_
#define _LOCK_H

#include <pthread.h>
#include <stdint.h>

#define RWLOCK_INITIALIZER { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0, 0 }
/**
 *  rwlock
 */
typedef struct
{
    pthread_mutex_t     lock;           /**< Mutex lock for this rwlock */
    pthread_cond_t      rd_cond;        /**< Condition variable */
    pthread_cond_t      wr_cond;
    int                 num_readers;    /**< Number of readers. If this is less than 0, than a writer(s) has the lock */
    int                 rd_queue;
    int                 wr_queue;
    pthread_t           writer_id;      /**< Stores the TID of the writer */
} rwlock_t;

/**
 * Initialise an rwlock
 */
int rwlock_init(rwlock_t* lock);

/**
 *  Acquire the read lock
 */
int rwlock_rdlock(rwlock_t* lock);

int rwlock_wrlock(rwlock_t* lock);

int rwlock_unlock(rwlock_t* lock);


#endif