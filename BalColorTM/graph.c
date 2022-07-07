#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include<string.h>

#include "graph.h"

// Global var
extern int *color_arr;

// Read graph
struct Graph* graph_read(const char *filename, int nthreads)
{
    FILE *fp;
    fp = fopen(filename,"r");
    char line[50];
    char *token;
    int V;
    int dest;
    int source;
    int counter = 1;
    struct Graph* graph = NULL;
    int edges = 0;
    weight_t weight;    

    while ( fgets ( line, sizeof line, fp ) != NULL ) /* read a line */
    {
        token=strtok(line," ");
        if(strcmp(token,"c")==0 && counter==2)
        {
            token=strtok(NULL," ");
            token=strtok(NULL," ");
            token=strtok(NULL," ");
            token=strtok(NULL," ");
            V=atoi(token);
            graph = createGraph(V, nthreads);
        }
        else if(strcmp(token,"a")==0)
        { 
            token=strtok(NULL," ");
            source=atoi(token);
            token=strtok(NULL," ");
            dest=atoi(token);
            token=strtok(NULL," ");
            weight=atof(token);
            edges++;
            addEdge(graph, source - 1, dest - 1);

        }
        counter++;
    }   

    return graph;
}

// Read graph
struct Graph* graph_read_mtx(const char *filename, int nthreads)
{
    FILE *fp;
    fp = fopen(filename,"r");
    char *line;
    char *token;
    int V, E;
    int dest;
    int source;
    weight_t weight;
    int counter = 1;
    struct Graph* graph = NULL;
    int edges = -1;

    line = (char *) malloc(1000 * sizeof(char));

    while ( fgets ( line, 1000, fp ) != NULL ) /* read a line */
    {
        token=strtok(line," ");

        if(token[0] == '%')
            ;
        else if(token[0] != '%' && edges == -1){
            V = atoi(token);
            token=strtok(NULL," ");
            token=strtok(NULL," ");
            E = atoi(token);
            graph = createGraph(V, nthreads);
            edges++;
        }else{
            source = atoi(token);
            token=strtok(NULL," ");
            dest = atoi(token);
            edges++;
            addEdge(graph, source - 1, dest - 1);
        }
    }

    free(line); 
    return graph;
}

// A utility function to create a new adjacency list node
struct AdjListNode* newAdjListNode(int dest)
{
    struct AdjListNode* newNode =
        (struct AdjListNode*) malloc(sizeof(struct AdjListNode));
    newNode->dest = dest;
    return newNode;
}

// A utility function that creates a graph of V vertices
struct Graph* createGraph(int V, int nthreads)
{
    int i;
    struct Graph* graph = (struct Graph*) malloc(sizeof(struct Graph));
    graph->V = V;
    graph->edges = 0;
    graph->maxDegree = 0;

    // Create an array of adjacency lists.  Size of array will be V
    graph->array = (struct AdjList*) malloc(V * sizeof(struct AdjList));
    color_arr = (int *) malloc(V * sizeof(int));


    // Initialize each adjacency list as empty by making head as NULL
    for (i = 0; i < V; ++i){   
        graph->array[i].index = i;   
        graph->array[i].head = NULL;
        graph->array[i].neighboors = 0;
        // Initialize color as unassigned
        color_arr[i] = -1;


    }

    return graph;
}

// Adds an edge to an undirected graph
void addEdge(struct Graph* graph, int src, int dest)
{
    // Add an edge from src to dest.  A new node is added to the adjacency
    // list of src.  The node is added at the begining
    if(graph->array[src].head==NULL) 
        graph->array[src].head = (struct AdjListNode *) malloc(sizeof(struct AdjListNode));
    else 
        graph->array[src].head = (struct AdjListNode *) realloc(graph->array[src].head, (graph->array[src].neighboors+1) * sizeof(struct AdjListNode));

    graph->array[src].head[graph->array[src].neighboors].dest = dest;
    graph->array[src].neighboors++;


    /*
     * Store the maximimum vertex degree.
     */
    if(graph->maxDegree < graph->array[src].neighboors)
        graph->maxDegree = graph->array[src].neighboors;

    // Since graph is undirected, add an edge from dest to src also
    if(graph->array[dest].head==NULL) 
        graph->array[dest].head = (struct AdjListNode *) malloc(sizeof(struct AdjListNode));
    else 
        graph->array[dest].head = (struct AdjListNode *) realloc(graph->array[dest].head, (graph->array[dest].neighboors+1) * sizeof(struct AdjListNode));

    graph->array[dest].head[graph->array[dest].neighboors].dest = src;
    graph->array[dest].neighboors++;

    /*
     * Store the maximimum vertex degree.
     */
    if(graph->maxDegree < graph->array[dest].neighboors)
        graph->maxDegree = graph->array[dest].neighboors;

    graph->edges++;
}

