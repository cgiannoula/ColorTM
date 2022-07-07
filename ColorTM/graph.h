#ifndef __GRAPH_H_
#define __GRAPH_H_

#include <inttypes.h>
#include <stdint.h>

//#define MAXBIT 0x0000000000000040
#define MAXBIT 64

typedef float weight_t;

struct AdjListNode
{
    int dest;
};

// A structure to represent an adjacency liat
struct AdjList
{
    int neighboors;
    struct AdjListNode *head;  // pointer to head node of list
};

// A structure to represent a graph. A graph is an array of adjacency lists.
// Size of array will be V (number of vertices in graph)
struct Graph
{
    unsigned int V;
    unsigned int edges;
    unsigned int maxDegree;
    struct AdjList* array;
};

int *color_arr;
int *nrcolors;

extern struct Graph* graph_read(const char *filename, int nthreads);
extern struct Graph* graph_read_mtx(const char *filename, int nthreads);
extern struct AdjListNode* newAdjListNode(int dest);
extern struct Graph* createGraph(int V, int nthreads);
extern void addEdge(struct Graph* graph, int src, int dest);
#endif
