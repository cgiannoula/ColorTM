#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <sched.h>
#include <stdbool.h>
#include <immintrin.h>
//#include <numa.h>

#include "graph.h"
#include "timers_lib.h"
#include "graphColor.h"
#include "transaction.h"

#define NUM_RETRIES 50 

/* Global vars */
pthread_barrier_t start_barrier;
__attribute__((aligned(64))) pthread_spinlock_t fallback_lock;
__attribute__((aligned(64))) extern int *color_arr;
__attribute__((aligned(64))) extern int *nrcolors;

int log_2(int64_t x){
    int64_t mul=1;
    int ret;

    ret = 0;
    while(mul != x){
        mul *= 2;
        ret++;
    }   
    return ret;
}

// New stats counter 
struct graphColor_stats_st *stats_thread_counter_new()
{
    struct graphColor_stats_st * stats= (struct graphColor_stats_st *) malloc(sizeof(struct graphColor_stats_st));
    stats->total_time = 0.0;

    return stats;
}

// Initialize global statistics
void graphColor_stats_init(struct graphColor_stats_st *stats)
{
    stats->total_time = 0.0;
    return;
}

// Print global statistics
void graphColor_stats_print(struct graphColor_stats_st *stats)
{
    fprintf(stdout, "Total time: \t\t %f\n", stats->total_time);
    fprintf(stdout, "\n\n\n");
    return;
}

static void setaffinity_oncpu(int cpu)
{
    cpu_set_t cpu_mask;
    int err;

    CPU_ZERO(&cpu_mask);
    CPU_SET(cpu, &cpu_mask);

    err = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);
    if (err) {
        perror("sched_setaffinity");
        exit(1);
    }
}

// A utility function used to print the solution
void printColor(struct Graph *graph)
{
    int i;
    printf("Vertex   Color\n");
    for (i = 0; i < graph->V; ++i)
        printf("%d \t\t %d\n", i, color_arr[i]);
}

// A utility function used to check the correctness of the graph coloring
void checkCorrectness(struct Graph *graph){

    int i, j;
    printf("Checking correctness ...\n");
    for(i=0; i<graph->V; i++){
        if(color_arr[i] == -1){
            printf("ERROR \n");
            return;
        }
        for(j=0; j<graph->array[i].neighboors; j++)
            if(color_arr[i] == color_arr[graph->array[i].head[j].dest] && i != graph->array[i].head[j].dest){
                printf("ERROR \n");
                return;
            }
    }

    printf("OK \n");
    return;
}

// A utility function to count the number of colors used to color the graph
int ColorsUsed(struct Graph *graph){
    int *available;
    int i, u;
    available = (int*) malloc((graph->maxDegree+1) * sizeof(int));

    for(i=0; i<=graph->maxDegree; i++)
        available[i] = 0; 

    for(u=0; u<graph->V; u++)
        available[color_arr[u]] = 1;

    int count=0;
    for(i=0; i<=graph->maxDegree; i++){
        if(available[i] == 1)
            count++;
    }

    free(available);
    return count;
}

// A utility function to count the number of vertices per color class
int color_class_sizes(struct Graph *graph, int colors){

    int i, u=0;
    int V = graph->V;

    nrcolors = (int *) malloc((colors+1000) * sizeof(int));
    for(i=0; i<colors+1000; i++){
        nrcolors[i] = 0;
    }

    for(i=0; i<V; i++){     
        nrcolors[color_arr[i]] += 1; 
    }

    return 0;
}

// Function to color vertices of the graph
void *thread_func(void *args)
{
    thread_args_t *targs = (thread_args_t*)args;
    struct Graph *graph = targs->graph;
    int tid = targs->tid;
    int nthreads = targs->nthreads;
    struct graphColor_stats_st *stats = targs->stats;
    count_tx_t *counter = targs->counter;
    int i, u, v, color, check;
    int V = graph->V; // Get the number of vertices in graph
    int maxDegree = graph->maxDegree; // Get the maxDegree 
    uint64_t offset, help, helpu, colOffset, forbidden;
    uint64_t r1, r2, clr;

    setaffinity_oncpu(tid);
    __sync_synchronize();

    int vert_per_proc = V / nthreads;
    int mod = V % nthreads;

    for(i = 0; i < mod; i++)
        if(tid == i)
            vert_per_proc ++;

    int startVertex, endVertex;
    startVertex = tid * vert_per_proc;
    endVertex = (tid + 1) * vert_per_proc;

    if(tid >= mod){
        startVertex = mod * (vert_per_proc + 1) + (tid -  mod) * vert_per_proc;
        endVertex = startVertex + vert_per_proc;
    }

    /*
     * Warm up
     */
    int dummy=0;
    int max = 0;
    for(u=startVertex; u<endVertex; u++){
        if(graph->array[u].neighboors !=0)
            dummy++;
        if(graph->array[u].neighboors > max)
            max = graph->array[u].neighboors;
        if(color_arr[u] == -1)
            dummy++;
    }

    /*
     * A temporary array to store the available colors.
     */    
    int border_neighbr=0;
    __attribute__((aligned(64))) int *neigh_index;
    neigh_index = (int *) malloc((max+1) * sizeof(int));

    for(i=0; i<=max; i++){
        neigh_index[i] = -1;
    }


    // Wait until all threads go to the starting point.	
    pthread_barrier_wait(&start_barrier);


    /*
     * Assign colors to vertices.
     */
    for(u=startVertex; u<endVertex; u++){
        int nb = graph->array[u].neighboors;
        /*
         * Process all adjacent vertices and flag their colors
         * us unavailable.
         */
REPEAT:
        border_neighbr = 0;
        colOffset = 0;

        while(color_arr[u] == -1){
            forbidden = 0;
            for(i=0; i<nb; i++){
                v = graph->array[u].head[i].dest;

                /*
                 * Set the forbidden colors as forbidden bits
                 */
                if(color_arr[v] >= colOffset && color_arr[v] < colOffset + MAXBIT){
                    help = 1;
                    help = help << (color_arr[v] - colOffset);
                    forbidden = forbidden | help;
                }

                if((v < startVertex || v >= endVertex) && color_arr[v] == -1){ 
                    neigh_index[border_neighbr] = v;
                    border_neighbr++;
                }

            }

            /*
             * Find the first bit if exists
             * and set it as u's color.
             * If doesn't exist, repeat using
             * an increased(+64) colorRange.
             */ 

            /*
             * First zero bit:
             * 1. Invert the number
             * 2. Compute the two's complement of the inverted number (=initial number+1)
             * 3. AND the results (1) and (2)
             * 4. Find the position by computing the binary logarithm of (3)
             */   
            if(forbidden == 0xFFFFFFFFFFFFFFFF){
                border_neighbr = 0;
                colOffset += MAXBIT;
                continue;
            }

            /*
             * Assign the found color.
             * If no border vertex, avoid synchronization (TM).
             */
            r1 = ~forbidden;
            r2 = forbidden + 1;
            clr = r1 & r2;
            color = log_2(clr) + (int) colOffset;

            if(border_neighbr == 0){
                color_arr[u] = color; 
            }else{
                begin_transaction(NUM_RETRIES, &fallback_lock, counter, tid);
                check = 1;
                for(i=0; i<border_neighbr; i++){
                    if(color_arr[neigh_index[i]] == color){
                        check = 0;
                        break;
                    }
                }
                if(check == 1){
                    color_arr[u] = color;
                    end_transaction(&fallback_lock, counter);	
                }else{
                    end_transaction(&fallback_lock, counter);	
                    goto REPEAT;
                }       
            } 
        }
    }

    pthread_exit(NULL);

}

// The start function 
void graphColor(struct Graph *graph, int nthreads)
{
    int i;
    int V = graph->V, v;// Get the number of vertices in graph

    /*
     * Initialize the starting barrier
     */
    pthread_barrier_init(&start_barrier, NULL, nthreads+1);

    /*
     * Initialize lock for TX, dijkstra stats and abort counter for TX
     */
    pthread_spin_init(&fallback_lock, PTHREAD_PROCESS_SHARED);
    struct  graphColor_stats_st **stats;// = (struct graphColor_stats_st **) malloc(nthreads * sizeof(struct  graphColor_stats_st *));
    count_tx_t **counters = (count_tx_t **) malloc(nthreads * sizeof(count_tx_t *));


    /*
     * Initialize thread arguments, etc.
     */ 
    pthread_t *tids = (pthread_t*)malloc(nthreads * sizeof(pthread_t));
    thread_args_t *targs = (thread_args_t*)malloc(nthreads * sizeof(thread_args_t));

    /* threads */
    for(i=0; i<nthreads; i++) {
        targs[i].tfunc = thread_func;  
    }

    for(i=0; i<nthreads; i++){
        targs[i].tid = i;
        targs[i].nthreads = nthreads;
        targs[i].graph = graph;
        counters[i] = tx_thread_counter_new();
        targs[i].counter = counters[i];		
    }

    for(i=0; i<nthreads; i++)
        pthread_create(&tids[i], NULL, targs[i].tfunc, (void*)&targs[i]);

    /*
     * Wait until all threads go to the starting point.
     */
    pthread_barrier_wait(&start_barrier);

    /*
     * Init and start wall_timer.
     */
    timer_tt *wall_timer = timer_init();
    timer_start(wall_timer);

    for(i=0; i<nthreads; i++)
        pthread_join(tids[i], NULL);

    /*
     * Stop wall_timer.
     */
    timer_stop(wall_timer);

    /*
     * Print TX statistics 
     */
    count_tx_t *total_counter_tx = tx_thread_counter_new();
    for(i=0; i<nthreads; i++){
        tx_thread_counter_print(i, counters[i]);
        tx_thread_counter_add(counters[i], total_counter_tx, total_counter_tx);
    }
    printf("\n\n");
    printf("TOTAL ");
    tx_thread_counter_print(-1, total_counter_tx);
    printf("\n\n\n");

    /*
     * Print elapsed time.
     */
    double time_elapsed = timer_report_sec(wall_timer);
    printf("Time elapsed: \t\t%lf \n", time_elapsed);

    /*
     * Free structures
     */
    free(tids);
    free(counters);
    free(targs); 
    pthread_barrier_destroy(&start_barrier);


    return;
}

