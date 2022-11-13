/**
 * @file semaphore.c
 * @author
 * @brief Defines all funtions of a semaphore
 * @version 0.1
 * @date 12.11.2022
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define SEM_FREE "/sem_free"
#define SEM_USED "/sem_used"
#define SEM_WRITE_BLOCK "/sem_write"

/**
 * @brief Opens a semaphore and handles upcoming errors if type 0 its for the supervisor if type 1 its for the generator
 * 
 * @param sem 
 * @param semName 
 * @param semSize 
 * @param type 
 * @param programName 
 */
void openSem(sem_t **sem, char *semName, size_t semSize, int type, const char *programName){
	if(type == 0){
        *sem = sem_open(semName, O_CREAT | O_RDWR, 0600, semSize);
    }else {
        *sem = sem_open(semName, O_RDWR);
    }
	if(*sem == SEM_FAILED){
		fprintf(stderr, "%s - Couldn't open semaphore: %s\n", programName, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief Handles wait_sem for the given semaphore and handles upcoming errors
 * 
 * @param sem 
 * @param programName 
 */
void waitSem(sem_t *sem, const char *programName){
	if(sem_wait(sem) == -1){
		if (errno != EINTR){
			fprintf(stderr, "%s - Couldn't lock semaphore: %s\n", programName, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}

/**
 * @brief Handles sem_post for the given semaphore and handles upcoming errors
 * 
 * @param sem 
 * @param programName 
 */
void postSem(sem_t *sem, const char *programName){
	if(sem_post(sem) == -1){
		if (errno != EINTR){
			fprintf(stderr, "%s - Couldn't post semaphore: %s\n", programName, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}

/**
 * @brief Handles sem_close for the given semaphore and handles upcoming errors
 * 
 * @param sem 
 * @param programName 
 */
void closeSem(sem_t *sem, const char *programName){
	if(sem_close(sem) < 0){
		fprintf(stderr, "%s - Couldn't close semaphore: %s\n", programName, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief Handles sem_unlink for the given semaphore and handles upcoming errors
 * 
 * @param semName 
 * @param programName 
 */
void unlinkSem(char *semName, const char *programName){
	if(sem_unlink(semName) < 0){
		fprintf(stderr, "%s - Couldn't unlink semaphore: %s\n", programName, strerror(errno));
		exit(EXIT_FAILURE);
	}
}