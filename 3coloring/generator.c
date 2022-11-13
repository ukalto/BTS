/**
 * @file generator.c
 * @author
 * @brief Creates a generator which is responsible for writing solutions to the solution buffer which are read by the supervisor
 * @version 0.1
 * @date 12.11.2022
 * 
 * @copyright Copyright (c) 2022
 * valgrind --tool=memcheck --leak-check=yes ./generator 0-1 0-2 1-2 to check for memory leaks
 */
#include <regex.h>
#include <time.h>

#include "3color.h"
#include "semaphore.c"
#include "sharedmemory.c"

#define PROGRAM_NAME "./generator"

static volatile sig_atomic_t quit = 0;
static int shmfd = -1;
static shm_t *solution_buffer = NULL;
static sem_t *freeSem = NULL;
static sem_t *usedSem = NULL;
static sem_t *writeSem = NULL; 

/**
 * @brief Function which is called when the input is wrong
 * 
 */
static void wrongInputError(void){
	fprintf(stderr, "Use: %s d-d d-d d-d where d is an integer.\n",PROGRAM_NAME);
	exit(EXIT_FAILURE);
}

/**
 * @brief Handles the signal when detected and sets quit to 1 so generator terminates
 * 
 * @param signal 
 */
static void handleSignal(int signal) { 
	quit = 1; 
}

/**
 * @brief Function which is responsible to liste to all signals by the user
 * 
 */
static void listenToSignal(void){
	struct sigaction sa = { .sa_handler = handleSignal };
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

/**
 * @brief checks if a node with the nodeIdx already exists in the nodes array 1 if yes 0 if not
 * 
 * @param nodeIdx 
 * @param currLen 
 * @param nodes 
 * @return int 
 */
static int checkNodesExisting(int nodeIdx, int currLen, node_t **nodes){
	for(int i = 0; i < currLen; i++){
		if (nodeIdx == (*nodes)[i].nodeIdx){
            return 1;
		}
	}
	return 0;
}

/**
 * @brief adds node if not alreay in nodes array to the nodes array 1 if successfull 0 if not
 * 
 * @param nodeIdx 
 * @param currLen 
 * @param nodes 
 * @return int 
 */
static int addNodes(int nodeIdx, int currLen, node_t **nodes){
	if(checkNodesExisting(nodeIdx, currLen, nodes) != 1){
		(*nodes)[currLen].nodeIdx = nodeIdx;
		(*nodes)[currLen].color = 0;
		return 1;
	}
	return 0;
}

/**
 * @brief converts one argv to an edge 
 * 
 * @param toConvert 
 * @param edge 
 */
static void convertEdge(char *toConvert, edge_t *edge){
	edge->first_node = strtol(strtok(toConvert, "-"), NULL, 10);
	edge->second_node = strtol(strtok(NULL, "-"), NULL, 10);
}

/**
 * @brief parses the input, firstly a regex is created to check if the given edges are in the right format
 * then the edge is converted, then nodes are added if not already in nodes array
 * at the end we set the nodes count with is the length if the nodes array to the currLen which we count
 * through the process and set the edges count to argc-1
 * 
 * @param argc 
 * @param argv 
 * @param nodes 
 * @param edges 
 * @param nodesCount 
 * @param edgesCount 
 */
static void parseInput(int argc, char **argv, node_t **nodes, edge_t **edges, int *nodesCount, int *edgesCount){
	regex_t reg;
	if(regcomp(&reg, "^[0-9]*-[0-9]*$", REG_EXTENDED | REG_NOSUB)) {
        fprintf(stderr, "ERROR: %s Couldn't create regex!\n",PROGRAM_NAME);
        exit(EXIT_FAILURE);
    }
	int currLen = 0;
	node_t **currNodes = malloc(sizeof(node_t)*argc*2);
	for(int i = 1; i < argc; i++){
		if(regexec(&reg, argv[i], 0, NULL, 0) == 0){
			// edge
			convertEdge(argv[i], &((*edges)[i - 1]));
			edge_t edge = (*edges)[i - 1];
			memcpy(currNodes, nodes, sizeof(node_t)*argc*2);
			// nodes from edge
			currLen += addNodes(edge.first_node, currLen, currNodes);
			currLen += addNodes(edge.second_node, currLen, currNodes);
		} else {
			wrongInputError();
		}
	}
	*nodesCount = currLen;
	*edgesCount = argc-1;
	regfree(&reg);
	free(currNodes);
}

/**
 * @brief sets the color for every node ranomly to exact one value of these numbers: 0,1,2 which each represents one color
 * 
 * @param nodesCount 
 * @param nodes 
 */
static void randomNodeColor(int nodesCount, node_t *nodes){
	for(int i = 0; i < nodesCount; i++){
		nodes[i].color = (rand() % 3);
	}
}

/**
 * @brief Get the Node object with the given nodeIdx if it exists
 * 
 * @param nodeIdx 
 * @param currLen 
 * @param nodes 
 * @return node_t* 
 */
static node_t* getNode(int nodeIdx, int currLen, node_t *nodes){
	for(int i = 0; i < currLen; i++){
		if(nodes[i].nodeIdx == nodeIdx){
			return &nodes[i];
		}
	}
	return NULL;
}

/**
 * @brief finds a possible solution with has less the MAX_EDGES by removing every edge between two nodes with
 * the same color
 * 
 * @param nodes 
 * @param edges 
 * @param nodesCount 
 * @param edgesCount 
 * @param solution 
 * @return int 
 */
static int findSolution(node_t *nodes, edge_t *edges, int nodesCount, int edgesCount, solution_t *solution){
	randomNodeColor(nodesCount, nodes);
	int i = 0;
	for(int countEdges = 0; i < edgesCount; i++){
		node_t fn = *getNode(edges[i].first_node, nodesCount, nodes);
		node_t sn = *getNode(edges[i].second_node, nodesCount, nodes);
 		if(fn.color == sn.color){
			if(solution->numberOfEdges == MAX_EDGES)
				break;
			solution->edges[countEdges] = edges[i];
			solution->numberOfEdges++;
			countEdges++;
		}
		if(countEdges == edgesCount) return 0;
	}
	return i == edgesCount ? 1 : 0; 
}

/**
 * @brief Function which is called when the programm exits. Unmaps shared memory and closes semaphores.
 * 
 */
static void closeUp(void){
	if(solution_buffer != NULL){
		solution_buffer->shmTracker--;
        unmapSHM(solution_buffer, sizeof(*solution_buffer), PROGRAM_NAME);
    }
    if(freeSem != NULL){
        closeSem(freeSem, PROGRAM_NAME);
    }
    if(usedSem != NULL){
        closeSem(usedSem, PROGRAM_NAME);
    }
    if(writeSem != NULL){
        closeSem(writeSem, PROGRAM_NAME);
    }
}

/**
 * @brief writes the solution to the shared memory
 * 
 * @param solution 
 */
static void writeToSolutionBuffer(solution_t solution){
	waitSem(writeSem, PROGRAM_NAME);
	waitSem(freeSem, PROGRAM_NAME);

 	solution_buffer->solutions[solution_buffer->write] = solution;
	solution_buffer->write = (solution_buffer->write+1) % MAX_EDGES;

	postSem(usedSem, PROGRAM_NAME);
	postSem(writeSem, PROGRAM_NAME);
}

/**
 * @brief this is the main method which manages the whole program process first we introduce the atexit function
 * which helps us to closeup everything either when closed successfully or not. Next we check if the input is right and
 * introduce the signal handler, then we parse our input, then we open our sharedmemory and check if we already found a 
 * perfect solution only needed when parallel generators are running we open our 3 semaphores, then set the solution_buffer 
 * tracker to +1 so we can know how many generators are running, then we introduce random seeds, then we search for a perfect 
 * solution until one generator finds one
 * 
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char **argv) {
	if(atexit(closeUp) < 0){
        fprintf(stderr, "%s - Couldn't set up closeup function: %s\n", PROGRAM_NAME, strerror(errno));
        exit(EXIT_FAILURE);
    }
	if(argc <= 1){
		fprintf(stderr, "%s Error: No edges given.\n", PROGRAM_NAME);
		wrongInputError();
	}

	listenToSignal();

	node_t *nodes = malloc(sizeof(node_t)*argc*2);
	edge_t *edges = malloc(sizeof(edge_t)*argc);
	int nodesCount;
	int edgesCount; 

	parseInput(argc, argv, &nodes, &edges, &nodesCount, &edgesCount);

    shmfd = openSHM(PROGRAM_NAME);
    solution_buffer = mapSHM(shmfd, sizeof(*solution_buffer), PROGRAM_NAME);
    shmfd = -1;
	if(solution_buffer->quit == 1) exit(EXIT_SUCCESS);
	openSem(&freeSem, SEM_FREE, 0, 1, PROGRAM_NAME);
	openSem(&usedSem ,SEM_USED, 0, 1, PROGRAM_NAME);
	openSem(&writeSem, SEM_WRITE_BLOCK, 0, 1, PROGRAM_NAME);

	solution_buffer->shmTracker++;

	srand(time(NULL)*getpid());

	while(solution_buffer->quit == 0 && quit == 0){
		solution_t solution = {.numberOfEdges = 0};
		if(findSolution(nodes, edges, nodesCount, edgesCount, &solution) == 1){
			writeToSolutionBuffer(solution);
		}
	}

	free(edges);
	free(nodes);
	exit(EXIT_SUCCESS);
}