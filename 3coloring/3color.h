/**
 * @file 3color.h
 * @author
 * @brief Defines all structers used to solve 3color
 * @version 0.1
 * @date 12.11.2022
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <signal.h>
#include <stdio.h>
#include <stdint.h>

/**
 * @brief Maximum of solutions in sharedmemory
 * 
 */
#define MAX_DATA 50

/**
 * @brief The maximum edges in a solution which is allowed
 * 
 */
#define MAX_EDGES 8

/**
 * @brief Structure of one node in the graph
 * 
 */
typedef struct node{
	int nodeIdx;
	int8_t color;
} node_t;

/**
 * @brief Structure of one edge in the graph
 * 
 */
typedef struct edge{
	int first_node;
	int second_node;
} edge_t;

/**
 * @brief Structure of one possible graph solution
 * 
 */
typedef struct solution{
	edge_t edges[MAX_EDGES];
	int numberOfEdges; 
} solution_t;

/**
 * @brief Structure of my shared memory which simulates the circular buffer
 * solutions contains every genereated solution
 * quit marks if everything should be terminated: 0=normal 1=shutdown
 * write is the current writing position for all generators
 * readPos is the current reading position for the supervisor
 * shmTracker tracks all generated generators to free them from waiting   
 * 
 */
typedef struct shm{
	solution_t solutions[MAX_DATA];
	volatile int quit;
	volatile int write;
	volatile int readPos;
	volatile int shmTracker;
} shm_t;