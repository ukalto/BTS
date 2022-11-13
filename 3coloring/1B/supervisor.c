/**
 * @file supervisor.c
 * @author Maximilian Gaber, 52009273
 * @brief Creates a supervisor which is responsible for reading solutions from generators
 * @version 0.1
 * @date 12.11.2022
 * 
 * @copyright Copyright (c) 2022
 * valgrind --tool=memcheck --leak-check=yes ./supervisor to check for memory leaks
 */
#include "3color.h"
#include "semaphore.c"
#include "sharedmemory.c"

#define PROGRAM_NAME "./supervisor"

static int shmfd = -1;
static shm_t *solution_buffer = NULL;
static sem_t *freeSem = NULL;
static sem_t *usedSem = NULL;
static sem_t *writeSem = NULL; 

/**
 * @brief Handles the signal when detected and sets quit to 1 so supervisor terminates
 * 
 * @param signal 
 */
static void handleSignal(int signal) { 
	solution_buffer->quit = 1; 
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
 * @brief Function which prints the whole solution in a nice format to stdout
 * 
 * @param solution 
 */
static void printSolution(solution_t solution){
	fprintf(stdout, "[%s] Solution with %d edges:", PROGRAM_NAME, solution.numberOfEdges);
	for(int i = 0; i < solution.numberOfEdges; i++){
		fprintf(stdout, " %d-%d", solution.edges[i].first_node,  solution.edges[i].second_node);
	}
	fprintf(stdout, "\n");
}

/**
 * @brief Checks if the given solution is better than the current best solution it sets the 
 * best solution to the given solution and prints it, if the solution has 0 edges removed its 
 * a 3 colorable graph or if the bestsolution is still the best it does nothing
 * anyway the readposition is moved up by one unless its the best solution
 * 
 * @param solution 
 * @param bestSolution 
 * @return solution_t 
 */
static int overwriteSolutionIfBetter(solution_t solution, solution_t *bestSolution){
	if(solution.numberOfEdges == 0){
			fprintf(stdout, "[%s] The graph is 3-colorable!\n", PROGRAM_NAME);
			solution_buffer->quit = 1;
			return 0;
	}
	if(bestSolution->numberOfEdges > solution.numberOfEdges){
		printSolution(solution);
		memcpy(bestSolution->edges, solution.edges, sizeof(((solution_t *)0)->edges));
		bestSolution->numberOfEdges = solution.numberOfEdges;
		return 1;
	}
	return -1;
}

/**
 * @brief Function which is called when the programm exits. Unmaps shared memory and closes semaphores and unlinks both.
 * 
 */
static void closeUp(void){
	if(solution_buffer != NULL){
		solution_buffer->quit = 1;
		if(freeSem != NULL){
			for(size_t i = 0; i < solution_buffer->shmTracker;i++){
				postSem(freeSem, PROGRAM_NAME);
			}
		}
    }
    if(freeSem != NULL){
        closeSem(freeSem, PROGRAM_NAME);
		unlinkSem(SEM_FREE, PROGRAM_NAME);
    }
    if(usedSem != NULL){
        closeSem(usedSem, PROGRAM_NAME);
		unlinkSem(SEM_USED, PROGRAM_NAME);
    }
    if(writeSem != NULL){
        closeSem(writeSem, PROGRAM_NAME);
		unlinkSem(SEM_WRITE_BLOCK, PROGRAM_NAME);
    }
	if(solution_buffer != NULL){
		unmapSHM(solution_buffer, sizeof(*solution_buffer), PROGRAM_NAME);
	}
	unlinkSHM(PROGRAM_NAME);
}

/**
 * @brief this is the main method which manages the whole program process first we introduce the atexit function
 * which helps us to closeup everything either when closed successfully or not. Next we check if the input is right and
 * introduce the signal handler, then we create our sharedmemory, then we open our 3 semaphores, then we create a best_solution
 * which tells us the current best solution at all time, then we read as long solutions from the memory as we find a perfect graph
 * which is in our case a 3 colorable one
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char **argv)
{
	if(atexit(closeUp) < 0){
        fprintf(stderr, "%s - Couldn't set up closeup function: %s\n", PROGRAM_NAME, strerror(errno));
        exit(EXIT_FAILURE);
    }
	if(argc > 1){
		printf("%s: Arguments are not supported!", PROGRAM_NAME);
		exit(EXIT_FAILURE);
	}

	listenToSignal();
	shmfd = openSHM(PROGRAM_NAME);
	truncateSHM(shmfd, sizeof(shm_t), PROGRAM_NAME);
    solution_buffer = mapSHM(shmfd, sizeof(*solution_buffer), PROGRAM_NAME);
    shmfd = -1;

	solution_buffer->quit = 0;
	solution_buffer->write = 0;
	solution_buffer->readPos = 0;
	solution_buffer->shmTracker = 0;

	openSem(&freeSem, SEM_FREE, MAX_DATA, 0, PROGRAM_NAME);
	openSem(&usedSem ,SEM_USED, 0, 0, PROGRAM_NAME);
	openSem(&writeSem, SEM_WRITE_BLOCK, 1, 0, PROGRAM_NAME);
	
	solution_t bestSolution = { .numberOfEdges = MAX_EDGES+1};

	while(solution_buffer->quit == 0){
		waitSem(usedSem, PROGRAM_NAME);
		if(overwriteSolutionIfBetter(solution_buffer->solutions[solution_buffer->readPos], &bestSolution) == 0){
			break;
		}
		solution_buffer->readPos = (solution_buffer->readPos+1) % MAX_EDGES;
		
		postSem(freeSem, PROGRAM_NAME);
	}
	exit(EXIT_SUCCESS);
} 