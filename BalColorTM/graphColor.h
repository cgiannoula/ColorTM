#ifndef __GRAPHCOLOR_H_
#define __GRAPHCOLOR_H_

#include <pthread.h>
#include <stdbool.h>

#include "graph.h"
#include "transaction.h"

typedef struct graphColor_stats_st{
    double total_time;
} graphColor_stats_t;


typedef struct thread_args_st {
    /* generic attributes */
    int tid;
    int nthreads;
    void* (*tfunc)(void*);

    /* application specific attributes */
    struct Graph* graph;
    struct graphColor_stats_st *stats;
    count_tx_t *counter;

} thread_args_t;

struct ColorList{
    int vertices;
    int *head;
};

struct ColorList *sort_arr;

struct rank_t{
    float curr;
};

struct rank_t *rank_arr;

extern void printColor(struct Graph *graph);
extern void checkCorrectness(struct Graph *graph);
extern int ColorsUsed(struct Graph *graph);
extern void graphColor_stats_init(graphColor_stats_t *stats);
extern void graphColor_stats_print(graphColor_stats_t *stats);
extern void graphColor_help_func(struct Graph *graph, int nthreads);
#endif
