#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h> 
#include <sys/time.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include "graphColor.h"
#include "graph.h"
#include "transaction.h"
#include "timers_lib.h"

extern graphColor_stats_t stats;

// Driver program to test above functions
int main(int argc,char ** argv)
{

    unsigned int nthreads;
    int next_option, test_flag, print_flag;
    char graphfile[256];
    struct Graph* graph;

    if(argc==1){
        printf("Usage: ./BalColorTM --graph <graphfile>\n"
                "\t\t --nthreads <nthreads>\n" 
              );
        exit(EXIT_FAILURE);
    }
    nthreads = 1;

    print_flag = 0;
    test_flag = 0;
    nthreads = 1;

    /* getopt stuff */
    const char* short_options = "g:n:s:pt";
    const struct option long_options[]={
        {"graph", 1, NULL, 'g'},
        {"nthreads", 1, NULL, 'n'},
        {"print", 0, NULL, 'p'},
        {"test", 0, NULL, 't'},
        {NULL, 0, NULL, 0}
    };

    do {
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);

        switch(next_option){
            case 'n':
                nthreads = atoi(optarg);
                break;

            case 't':
                test_flag = 1;
                break;

            case 'p':
                print_flag = 1;
                break;

            case 'g':
                sprintf(graphfile, "%s", optarg);
                break;

            case '?':
                fprintf(stderr, "Unknown option!\n");
                exit(EXIT_FAILURE);

            case -1:	//Done with options
                break;	

            default:	//Unexpected error
                exit(EXIT_FAILURE);
        }

    }while(next_option != -1);

    fprintf(stdout, "Read graph\n\n");
    //graph = graph_read(graphfile, nthreads);
    graph = graph_read_mtx(graphfile, nthreads);


    /*
     * Parallel TM
     */
    fprintf(stdout, "MULTIPLE THREADS\n");
    fprintf(stdout, "Nthreads %d\n\n\n", nthreads);

    graphColor_help_func(graph, nthreads);

    if(print_flag)
        printColor(graph);

    if(test_flag)
        checkCorrectness(graph);

    /*
     * Print statistics
     */
    fprintf(stdout, "\n\n");
    fprintf(stdout, "Vertices: \t\t %u\n", graph->V);
    fprintf(stdout, "Edges: \t\t\t %u\n", graph->edges);

    int colorsUsed = ColorsUsed(graph);
    fprintf(stdout, "Colors used: \t\t %d\n", colorsUsed);


    return 0;
}

