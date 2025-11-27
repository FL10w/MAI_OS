#ifndef SHARED_H
#define SHARED_H

#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>

#define SHM_NAME "/lab3_shm"
#define SEM_DATA_READY_NAME "/lab3_sem_data_ready"
#define SEM_DATA_FREE_NAME "/lab3_sem_data_free"

typedef struct {
    char data[4096]; 
    int ready;       
} SharedData;

#endif