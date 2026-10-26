#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_READERS 10000
#define NUM_WRITERS 2
#define ITERATIONS  2


//LIGHTSWITCH
typedef struct{
    int counter;
    sem_t mutex;
} lightswitch_t;

void lightswitch_init(lightswitch_t* ls){
    ls->counter = 0;
    sem_init(&ls->mutex, 0, 1);
}


void lightswitch_lock(lightswitch_t* ls, sem_t* semaphore){
    sem_wait(&ls->mutex);
    ls->counter++;
    if (ls->counter == 1){ //first in turns lights on
        sem_wait(semaphore);
    }

    sem_post(&ls->mutex);
}

void lightswitch_unlock(lightswitch_t* ls, sem_t* semaphore){
    sem_wait(&ls->mutex);
    ls->counter--;
    if (ls->counter == 0) {
        sem_post(semaphore); //last out turns lights off
    }

    sem_post(&ls->mutex);
}

void lightswitch_destroy(lightswitch_t* ls){
    sem_destroy(&ls->mutex);
}




static lightswitch_t read_lightswitch;
static lightswitch_t write_lightswitch;
static sem_t no_readers;
static sem_t no_writers;
static int shared_data = 0;


//READER
void* reader(void* arg){
    long id = (long)arg;

    for (int i = 0; i < ITERATIONS; i++){
        sem_wait(&no_readers); 
        lightswitch_lock(&read_lightswitch, &no_writers);
        sem_post(&no_readers);  

        printf("Reader %ld reads: %d\n", id, shared_data);
        usleep(100000);

        lightswitch_unlock(&read_lightswitch, &no_writers);
        usleep(100000);
    }

    return NULL;
}

//WRITER
void* writer(void* arg){
    long id = (long)arg;

    for (int i = 0; i < ITERATIONS; i++){
        lightswitch_lock(&write_lightswitch, &no_readers);
        sem_wait(&no_writers);

        shared_data++;
        printf("-- Writer %ld writes: %d\n", id, shared_data);
        usleep(100000);

        sem_post(&no_writers);
        lightswitch_unlock(&write_lightswitch, &no_readers);

        usleep(100000);
    }

    return NULL;
}


int main(void) {
    pthread_t readers[NUM_READERS];
    pthread_t writers[NUM_WRITERS];

    lightswitch_init(&read_lightswitch);
    lightswitch_init(&write_lightswitch);

    sem_init(&no_readers, 0, 1);
    sem_init(&no_writers, 0, 1);


    for (long i = 0; i < NUM_READERS; i++)
        pthread_create(&readers[i], NULL, reader, (void*)i);

    for (long i = 0; i < NUM_WRITERS; i++)
        pthread_create(&writers[i], NULL, writer, (void*)i);

    for (int i = 0; i < NUM_READERS; i++)
        pthread_join(readers[i], NULL);

    for (int i = 0; i < NUM_WRITERS; i++)
        pthread_join(writers[i], NULL);


    sem_destroy(&no_writers);
    sem_destroy(&no_readers);

    lightswitch_destroy(&read_lightswitch);    
    lightswitch_destroy(&write_lightswitch);
    
    return 0;
}