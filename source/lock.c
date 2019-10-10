/**
 *  Implementation of lock.h
 */
#include "lock.h"

#include <stdio.h>
#include <string.h>

/**
 * Initialise a read/write lock.
 */
int rwlock_init(rwlock_t* lock)
{
    if(lock == NULL)
    {
        printf("rwlock_init: lock == NULL!\n");
        return -1;
    }

    int ret;

    ret = pthread_mutex_init(&lock->lock, NULL);
    if(ret != 0)
    {
        printf("pthread_mutex_init failed! Reason: %s", strerror(ret));
        return ret;
    }

    ret = pthread_cond_init(&lock->rd_cond, NULL);
    if(ret != 0)
    {
        printf("pthread_cond_init failed! Reason: %s", strerror(ret));
        return ret;
    }

    ret = pthread_cond_init(&lock->wr_cond, NULL);
    if(ret != 0)
    {
        printf("pthread_cond_init failed! Reason: %s", strerror(ret));
        return ret;
    }

    lock->num_readers = 0;
    lock->wr_queue = 0;
    lock->rd_queue = 0;;
    lock->writer_id = 0;
    return 0;
}

/**
 * Acquire the rwlock as a write lock
 * 
 * This code blocks if:
 *  - There is already a writer that has the lock
 *  - There are readers that have the lock
 */
int rwlock_wrlock(rwlock_t* lock)
{
    int ret;

    if(lock == NULL)
    {
        printf("rwlock_wrlock: lock == NULL!\n");
        return -1;
    }

    if((ret = pthread_mutex_lock(&lock->lock)) != 0)
        return ret;

    // If someone is writing, or there are other threads reading, then 
    // we need to wait for rwlock_unlock() to send the OKAY signal to
    // the cond var. 
    if(lock->num_readers == -1 || lock->num_readers > 0)
    {
        lock->wr_queue++; // Increment the number of writers in the queue

        // Block here until we get a signal from rwlock_unlock()
        do
            pthread_cond_wait(&lock->wr_cond, &lock->lock);
        while(lock->num_readers == -1 || lock->num_readers > 0);

        lock->wr_queue--; // If we've unblocked, there's one less writer in the queue (that's us!).
    }

    // Inform other threads there is a write in progress.
    lock->num_readers = -1;
    lock->writer_id = pthread_self(); // This lock is now the current writer. Tag it current with the caller's TID

    if((ret = pthread_mutex_unlock(&lock->lock)) != 0)
        return ret;

    return 0;
}

int rwlock_rdlock(rwlock_t* lock)
{
    int ret;

    if(lock == NULL)
    {
        printf("rwlock_rdlock: lock == NULL!\n");
        return -1;
    }

    if((ret = pthread_mutex_lock(&lock->lock)) != 0)
        return ret;

    // Block and wait for the cond signal from rwlock_unlock() if there's a write currently
    // occuring. We also block here if there are writers in the queue. This prevents writer
    // starvation in the case of lots of readers (which could effectively slow our program
    // down to a crawl, or even put it into an infinite loop).
    if(lock->num_readers == -1 || lock->wr_queue > 0)
    {
        lock->rd_queue++; // Add this reader to the queue of readers

        // All threads that are waiting here unblock when the cond signal
        // is sent. 
        do
            pthread_cond_wait(&lock->rd_cond, &lock->lock);
        while(lock->num_readers == -1);

        // If we get here, it means that 'pthread_cond_wait()' has unblocked, hence the writer has relinquised the lock
        lock->rd_queue--;
    }
 
    
    lock->num_readers++;;

    if((ret = pthread_mutex_unlock(&lock->lock)) != 0)
        return ret;

    return 0;
}

static int _rwlock_rdunlock(rwlock_t* lock)
{
    int ret;
    if((ret = pthread_mutex_lock(&lock->lock)) != 0)
        return ret;

    lock->num_readers--;

    // If there are writers waiting in the queue, signal to the first waiting
    // one that is okay for them to acquire the lock for themselves.
    if(lock->wr_queue > 0 && lock->num_readers == 0)
        pthread_cond_signal(&lock->wr_cond);

    if((ret = pthread_mutex_unlock(&lock->lock)) != 0)
        return ret;

    return 0;
}

static int _rwlock_wrunlock(rwlock_t* lock)
{
    int ret;
    if((ret = pthread_mutex_lock(&lock->lock)) != 0)
        return ret;

    lock->num_readers = 0;
    lock->writer_id = 0;

    // If there are readers, inform them all that they may acquire the
    // lock by "brocasting" to them it is okay to unblock. 
    // As before, if there are writers waiting, and there are no current
    // readers that have the lock, then inform the next blocked writer in 
    // the queue that is okay for them to unblock and acquire the lock.
    if(lock->wr_queue > 0 && lock->num_readers == 0)
        pthread_cond_signal(&lock->wr_cond);
    else if(lock->rd_queue > 0)
        pthread_cond_broadcast(&lock->rd_cond);

    if((ret = pthread_mutex_unlock(&lock->lock)) != 0)
        return ret;

    return 0;
}

int rwlock_unlock(rwlock_t* lock)
{
    // To unlock the rwlock, it is first necessary to determine whether the caller
    // has the lock as a reader or a writer. This is done by checking the tag of the 
    //
    // This is how glibc implements this without needing to separate the lock functions, which
    // is actually rather elegant.
    if(lock->writer_id == pthread_self())
        return _rwlock_wrunlock(lock);
    else
       return _rwlock_rdunlock(lock);
}