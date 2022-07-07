#include <stdio.h>
#include <pthread.h>
#include <immintrin.h>
#include "transaction.h"

count_tx_t *tx_thread_counter_new()
{
    count_tx_t *counter = (count_tx_t *) malloc(sizeof(count_tx_t));

    counter->tx_starts = 0;
    counter->tx_commits = 0;
    counter->tx_aborts = 0;
    counter->tx_lacqs = 0;

    counter->tx_conflicts = 0; 
    counter->tx_capacity = 0;	
    counter->tx_explicits = 0;
    counter->tx_explicit_lock = 0;
    counter->tx_debug = 0;
    counter->tx_nested = 0;
    counter->tx_retry = 0;
    counter->tx_rest = 0;

    return counter;
}

void tx_thread_counter_print(int tid, count_tx_t *counter)
{
    printf("TXSTATS: %10d %10d %10d %10d (", tid,
            counter->tx_starts, counter->tx_commits, counter->tx_aborts);

    printf(" %10d %10d %10d (%10d) %10d %10d %10d %10d", counter->tx_conflicts, counter->tx_capacity,
            counter->tx_explicits, counter->tx_explicit_lock, counter->tx_debug,
            counter->tx_nested, counter->tx_retry, counter->tx_rest);
    printf(" )");
    printf(" %10d\n", counter->tx_lacqs);
}

void tx_thread_counter_add(void *d1, void *d2, void *dst)
{
    count_tx_t *data1 = d1, *data2 = d2, *dest = dst;

    dest->tx_starts = data1->tx_starts + data2->tx_starts;
    dest->tx_commits = data1->tx_commits + data2->tx_commits;
    dest->tx_aborts = data1->tx_aborts + data2->tx_aborts;
    dest->tx_lacqs = data1->tx_lacqs + data2->tx_lacqs;

    dest->tx_conflicts = data1->tx_conflicts + data2->tx_conflicts;
    dest->tx_capacity = data1->tx_capacity + data2->tx_capacity;
    dest->tx_explicits = data1->tx_explicits + data2->tx_explicits;
    dest->tx_explicit_lock = data1->tx_explicit_lock + data2->tx_explicit_lock;
    dest->tx_debug = data1->tx_debug + data2->tx_debug;
    dest->tx_nested = data1->tx_nested + data2->tx_nested;
    dest->tx_retry = data1->tx_retry + data2->tx_retry;
    dest->tx_rest = data1->tx_rest + data2->tx_rest;
}

int begin_transaction(int retries, pthread_spinlock_t *fallback_lock, count_tx_t *counter, int tid)
{
    int num_retries = retries;
    int aborts = 0;
    unsigned int status = 0;

    while(1){

        while (*fallback_lock == 0){
            __asm__ __volatile__("rep;nop": : :"memory");
        }

        counter->tx_starts++;
        status = _xbegin(); 

        if(status == _XBEGIN_STARTED){ 
            //> Transaction started successfully
            //> Check if lock is free, else abort explicitly.
            //> This also adds the lock to the read set so we abort if another thread acquires(writes to) the lock.
            if(*fallback_lock != 1)
                _xabort(0xff);

        }else{					//> Transaction was aborted
            aborts++; 
            counter->tx_aborts++;
            //> Abort reason was...
            if(status & _XABORT_CONFLICT)	//> data conflict
                counter->tx_conflicts++; 
            else if(status & _XABORT_CAPACITY)	//> capacity (transaction read or write set overflow)
                counter->tx_capacity++;
            else if(status & _XABORT_EXPLICIT){
                counter->tx_explicits++;
                if(_XABORT_CODE(status) == 0xff){
                    counter->tx_explicit_lock++;
                }
            }else if(status & _XABORT_DEBUG)
                counter->tx_debug++;
            else if(status & _XABORT_NESTED)
                counter->tx_nested++;
            else
                counter->tx_rest++; 

            if(status & _XABORT_RETRY)
                counter->tx_retry++;

            //> If we exceeded the number of retries acquire the lock and move forward to execute the
            //> critical section. Else retry in speculative mode (e.g. go back to the while loop).
            if(aborts >= num_retries){
                //pthread_spin_lock(fallback_lock);

                while(1){
                    while(*fallback_lock == 0) // do nothing
                        __asm__ __volatile__("rep;nop": : :"memory");
                    if(pthread_spin_trylock(fallback_lock) == 0)
                        return 0;
                }

            }else
                continue;
        }
        return 0;
    }
}

void end_transaction(pthread_spinlock_t *fallback_lock, count_tx_t *counter)
{
    if(_xtest()){
        _xend();
        counter->tx_commits++;
    }else{
        pthread_spin_unlock(fallback_lock);
        counter->tx_lacqs++;
    }

    return;
}

