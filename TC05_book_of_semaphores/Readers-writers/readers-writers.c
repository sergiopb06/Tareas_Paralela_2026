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
static sem_t room_empty;
static int shared_data = 0;


//READER
void* reader(void* arg){
    long id = (long)arg;

    for (int i = 0; i < ITERATIONS; i++){
        lightswitch_lock(&read_lightswitch, &room_empty);

        printf("Reader %ld reads: %d\n", id, shared_data);
        usleep(100000);

        lightswitch_unlock(&read_lightswitch, &room_empty);
        usleep(100000);
    }

    return NULL;
}

//WRITER
void* writer(void* arg){
    long id = (long)arg;

    for (int i = 0; i < ITERATIONS; i++){
        sem_wait(&room_empty);

        shared_data++;
        printf("-- Writer %ld writes: %d\n", id, shared_data);
        usleep(100000);

        sem_post(&room_empty);
        usleep(100000);
    }

    return NULL;
}


int main(void) {
    pthread_t readers[NUM_READERS];
    pthread_t writers[NUM_WRITERS];

    lightswitch_init(&read_lightswitch);
    sem_init(&room_empty, 0, 1);

    for (long i = 0; i < NUM_READERS; i++)
        pthread_create(&readers[i], NULL, reader, (void*)i);

    for (long i = 0; i < NUM_WRITERS; i++)
        pthread_create(&writers[i], NULL, writer, (void*)i);

    for (int i = 0; i < NUM_READERS; i++)
        pthread_join(readers[i], NULL);

    for (int i = 0; i < NUM_WRITERS; i++)
        pthread_join(writers[i], NULL);


    sem_destroy(&room_empty);
    lightswitch_destroy(&read_lightswitch);
    return 0;
}