#ifndef _GNU_SOURCE 
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <sched.h>
#include <stdbool.h>
#include <immintrin.h>

#include "graph.h"
#include "timers_lib.h"
#include "graphColor.h"
#include "transaction.h"

#define NUM_RETRIES 50 

/* Global vars */
__attribute__((aligned(64))) pthread_spinlock_t fallback_lock;
__attribute__((aligned(64))) extern int *color_arr;
__attribute__((aligned(64))) int *nrcolors;
__attribute__((aligned(64))) int balance;
__attribute__((aligned(64))) int under_full_bin;
__attribute__((aligned(64))) int gl_colOffset;
__attribute__((aligned(64))) uint64_t gl_forbidden;


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
    return NULL;
}

// Initialize global statistics
void graphColor_stats_init(struct graphColor_stats_st *stats)
{
    //   	stats->total_time = 0.0;
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
                printf("ERROR %d %d\n", i , graph->array[i].head[j].dest);
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
    available = (int*) malloc((graph->maxDegree+10) * sizeof(int));

    for(i=0; i<graph->maxDegree+10; i++)
        available[i] = 0; 

    for(u=0; u<graph->V; u++)
        available[color_arr[u]] = 1;

    int count=0;
    for(i=0; i<graph->maxDegree+10; i++){
        if(available[i] == 1)
            count++;
    }

    free(available);
    return count;
}


// A utility function to find over-full and under-full color classes
int over_full_bins(struct Graph *graph, int colors){

    int i, u=0;
    int under_full;
    int V = graph->V;

    nrcolors = (int *) malloc((colors+1000) * sizeof(int));
    for(i=0; i<colors+1000; i++){
        nrcolors[i] = 0;
    }

    balance = V / colors;
    for(i=0; i<V; i++){     
        nrcolors[color_arr[i]] += 1; 
    }

    for(i=0; i<colors; i++){
        if(u==0 && nrcolors[i] < balance){
            u = 1;
            under_full = i;
        }
    }

    under_full_bin = under_full;
    gl_colOffset = (under_full / MAXBIT) * MAXBIT;
    int help_forbidden = under_full % MAXBIT;
    uint64_t help;
    gl_forbidden = 0;
    for(i=0; i<help_forbidden; i++){
        help = 1;
        help = help << i;
        gl_forbidden = gl_forbidden | help;
    }

    //printf("nrcolors %d balance %d colOffset %d\n",
    //        under_full, balance, gl_colOffset);
    //printf("forbidden %" PRIu64 "\n", gl_forbidden);

    return under_full;
}

// Function to color vertices of the graph
void graphColor(struct Graph *graph, int nthreads)
{
    int i, u, v, color, check, tid, nb, k;
    int V = graph->V;// Get the number of vertices in graph
    int maxDegree = graph->maxDegree;// Get the maxDegree 
    uint64_t offset, help, helpu, colOffset, forbidden;
    uint64_t r1, r2, clr;
    count_tx_t *total_counter_tx = tx_thread_counter_new();
    timer_tt *wall_timer;
    int border_neighbr=0;


    omp_set_dynamic(0); 
    omp_set_num_threads(nthreads);
    /*
     * Assign colors to vertices.
     */
#pragma omp parallel private(border_neighbr, r1, r2, clr, colOffset, v, help, forbidden, check, u, color, tid, nb, i, k) 
    {
        tid = omp_get_thread_num();
        setaffinity_oncpu(tid);

        count_tx_t *counter = tx_thread_counter_new();
        __sync_synchronize();

        if(tid == 0)
            wall_timer = timer_init();


        int *neigh_index = (int *) malloc((maxDegree+1) * sizeof(int)); 
        for(i=0; i<=maxDegree; i++){ 
            neigh_index[i] = -1;
        }

#pragma omp barrier
        if(tid == 0){
            //wall_timer = timer_init();
            timer_start(wall_timer);
        }

#pragma omp for schedule(dynamic, 100)
        for(u=0; u<V; u++){
            nb = graph->array[u].neighboors;
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

                    if(color_arr[v] == -1){ //FIXME  
                        neigh_index[border_neighbr] = v;
                        border_neighbr++;
                    }

                }

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
                    begin_transaction(NUM_RETRIES, &fallback_lock, counter);
                    check=1;
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

#pragma omp barrier
        if(tid==0){
            /*
             * Stop wall_timer.
             */
            timer_stop(wall_timer);
            /*
             * Print elapsed time.
             */
            printf("\n\n");
            double time_elapsed = timer_report_sec(wall_timer);
            printf("Time elapsed in coloring (sec): \t\t%lf \n", time_elapsed);
            printf("\n\n");
        }
        __sync_synchronize();

#pragma omp barrier
        }
return;
}

// Function to balance vertices of the graph across color classes
void graphColorBalance(struct Graph *graph, int nthreads, int colors)
{
    //omp_init_lock(&fallback_lock);
    int i, u, v, color, check, tid, nb, k, color2;
    int V = graph->V;// Get the number of vertices in graph
    int maxDegree = graph->maxDegree;// Get the maxDegree 
    uint64_t offset, help, helpu, colOffset, forbidden;
    uint64_t r1, r2, clr;
    count_tx_t *total_counter_tx = tx_thread_counter_new();
    timer_tt *wall_timer;
    int border_neighbr=0;
    int h;


    omp_set_dynamic(0); 
    omp_set_num_threads(nthreads);

#pragma omp parallel private(border_neighbr, r1, r2, clr, colOffset, v, help, forbidden, check, u, color, tid, nb, i, k, h, color2) 
    {
        tid = omp_get_thread_num();
        setaffinity_oncpu(tid);

        count_tx_t *counter = tx_thread_counter_new();
        __sync_synchronize();


        int *neigh_index = (int *) malloc((maxDegree+1) * sizeof(int)); 
        for(i=0; i<=maxDegree; i++){ 
            neigh_index[i] = -1;
        }

#pragma omp barrier
        if(tid == 0){
            wall_timer = timer_init();
            timer_start(wall_timer);
        }

#pragma omp for schedule(dynamic, 100) 
        for(u=0; u<V; u++){

            if((color_arr[u] >= under_full_bin))
                continue; 
            if((color_arr[u] < under_full_bin) && (nrcolors[color_arr[u]] <= balance))
                continue; 


            nb = graph->array[u].neighboors;
REPEAT:
            color = -1;
            border_neighbr = 0;
            colOffset = gl_colOffset;
            h=0;

            while(color == -1){
                if(h==0){
                    forbidden = gl_forbidden;
                    h++;
                }else
                    forbidden = 0;

                for(i=0; i<nb; i++){
                    v = graph->array[u].head[i].dest;

                    if(color_arr[v] >= colOffset && color_arr[v] < colOffset + MAXBIT){
                        help = 1;
                        help = help << (color_arr[v] - colOffset);
                        forbidden = forbidden | help;
                    }

                    if(color_arr[v] < under_full_bin){  
                        neigh_index[border_neighbr] = v;
                        border_neighbr++;
                    }

                }

                if(forbidden == 0xFFFFFFFFFFFFFFFF){
                    border_neighbr = 0;
                    colOffset += MAXBIT;
                    continue;
                }

                for(i=colOffset; i<colOffset+MAXBIT && i<colors; i++){
                    if(nrcolors[i] >= balance){
                        help = 1;
                        help = help << (i - colOffset);
                        forbidden = forbidden | help;
                    }
                }


                if(forbidden == 0xFFFFFFFFFFFFFFFF){
                    border_neighbr = 0;
                    colOffset += MAXBIT;
                    continue;
                }

                r1 = ~forbidden;
                r2 = forbidden + 1;
                clr = r1 & r2;
                color = log_2(clr) + (int) colOffset;
                if(color >= colors){
                    ;// __sync_fetch_and_add(&nrcolors[color_arr[u]], 1);
                }else if(border_neighbr == 0){
                    __sync_fetch_and_sub(&nrcolors[color_arr[u]], 1);
                    color_arr[u] = color; 
                    __sync_fetch_and_add(&nrcolors[color], 1);
                }else{
                    color2 = color_arr[u];
                    begin_transaction(NUM_RETRIES, &fallback_lock, counter);
                    check=1;
                    for(i=0; i<border_neighbr; i++){  
                        if(color_arr[neigh_index[i]] == color){
                            check = 0;
                            break;
                        }
                    }
                    if(check == 1){
                        color_arr[u] = color;
                        end_transaction(&fallback_lock, counter);
                        __sync_fetch_and_sub(&nrcolors[color2], 1);
                        __sync_fetch_and_add(&nrcolors[color], 1);	
                    }else{
                        end_transaction(&fallback_lock, counter);	
                        goto REPEAT;
                    }       
                } 

            }

        }

#pragma omp barrier
        if(tid==0){
            timer_stop(wall_timer);
        }
        __sync_synchronize();

#pragma omp barrier
        for(k=0; k<nthreads; k++){
            if(tid == k){
                tx_thread_counter_print(k, counter);
                tx_thread_counter_add(counter, total_counter_tx, total_counter_tx);
            }
#pragma omp barrier
        }

        if(tid == 0){
            printf("\n\n");
            printf("TOTAL ");
            tx_thread_counter_print(-1, total_counter_tx);
            printf("\n\n");
            double time_elapsed = timer_report_sec(wall_timer);
            printf("Time elapsed in balancing (sec): \t\t%lf \n", time_elapsed);
            printf("\n\n");

        }

    }
    return;
}


void graphColor_help_func(struct Graph *graph, int nthreads){

    int i;
    time_t t;
    /* Intializes random number generator */
    srand((unsigned) time(&t));

    /*
     * Initialize lock for TX, dijkstra stats and abort counter for TX
     */
    pthread_spin_init(&fallback_lock, PTHREAD_PROCESS_SHARED); //FIXME	
    rank_arr = (struct rank_t*) malloc(graph->V * sizeof(struct rank_t));

    for(i=0; i<graph->V; i++){
        rank_arr[i].curr = 1 / (float) graph->V;
    }

    graphColor(graph, nthreads);
    checkCorrectness(graph);
    int colors = ColorsUsed(graph);

    int first_under_full_bin = over_full_bins(graph, colors);
    __sync_synchronize();

    graphColorBalance(graph, nthreads, colors);

    colors = ColorsUsed(graph);
    //for(i=0;i<=colors; i++){
    //    printf("Color %d Vertices %d\n", i, nrcolors[i]);
    //}

    return;
}
