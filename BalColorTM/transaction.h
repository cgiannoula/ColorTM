#ifndef __TRANSACTION_H_
#define __TRANSACTION_H_

#include <pthread.h>
#include <immintrin.h>

typedef struct count_abort_st {
    int tx_starts,
        tx_commits,
        tx_aborts,
        tx_lacqs;

    int tx_conflicts;
    int tx_capacity;
    int tx_explicits;
    int tx_explicit_lock;
    int tx_nested;
    int tx_debug;
    int tx_retry;
    int tx_rest;
}count_tx_t;

extern count_tx_t * tx_thread_counter_new();
extern void tx_thread_counter_print(int tid, count_tx_t *counter);
extern void tx_thread_counter_add(void *d1, void *d2, void *dst);
extern int begin_transaction(int retries, pthread_spinlock_t *fallback_lock, count_tx_t *counter);
extern void end_transaction(pthread_spinlock_t *fallback_lock, count_tx_t *counter);
#endif
