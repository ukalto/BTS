/**
 * @file sharedmemory.c
 * @author
 * @brief Defines all funtions of a sharedmemory
 * @version 0.1
 * @date 12.11.2022
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SHM_NAME "/shm"

/**
 * @brief Opens a shared memory and handles upcoming errors
 * 
 * @param programName 
 * @param type 
 * @return the shared memory file descriptor
 */
int openSHM(const char *programName){
	int shmfd;
	shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
	if(shmfd == -1){
		fprintf(stderr, "%s - Couldn't open shared memory object: %s\n", programName, strerror(errno));
		exit(EXIT_FAILURE);
    }
	return shmfd;
}

/**
 * @brief Maps the shared memory and handles upcoming errors
 * 
 * @param shmfd 
 * @param shmSize 
 * @param programName 
 * @return solution_buffer pointer
 */
void* mapSHM(int shmfd, size_t shmSize, const char *programName){
	void *solution_buffer = mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if(solution_buffer == MAP_FAILED){
		fprintf(stderr, "%s - Couldn't map shared memory object: %s\n", programName, strerror(errno));
		exit(EXIT_FAILURE);
    }
    if(close(shmfd) == -1){
		fprintf(stderr, "%s - Couldn't close shared memory file directory: %s\n", programName, strerror(errno));
		exit(EXIT_FAILURE);
    }
	return solution_buffer;
}

/**
 * @brief Sets the size of the shared memory and handles upcoming errors
 * 
 * @param shmfd 
 * @param shmSize 
 * @param programName 
 */
void truncateSHM(int shmfd, size_t shmSize, const char *programName){
	if(ftruncate(shmfd, shmSize) < 0){
		fprintf(stderr, "%s - Truncating shared memory object was not possible: %s\n", programName, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief Unmaps the shared memory and handles upcoming errors
 * 
 * @param solution_buffer 
 * @param shmSize 
 * @param programName 
 */
void unmapSHM(void *solution_buffer, size_t shmSize, const char *programName){
    if(munmap(solution_buffer, shmSize) == -1){
		fprintf(stderr, "%s - Unmapping shared memory object was not possible: %s\n", programName, strerror(errno));
		exit(EXIT_FAILURE);
    }
}

/**
 * @brief Unlinks the shared memory and handles upcoming errors
 * 
 * @param programName 
 */
void unlinkSHM(const char *programName){
    if(shm_unlink(SHM_NAME) == -1) {
		fprintf(stderr, "%s - Unlinking shared memory object was not possible: %s\n", programName, strerror(errno));
		exit(EXIT_FAILURE);
    }
}