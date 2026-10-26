#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 10
#define ITERATIONS  3


//MORRIS
static int room1 = 0;
static int room2 = 0;
static sem_t mutex; 
static sem_t t1;
static sem_t t2;
static int shared_data = 0;

void morris_enter(void){
    sem_wait(&mutex);
    room1++;
    sem_post(&mutex);

    sem_wait(&t1);
    room2++;
    sem_wait(&mutex);
    room1--;

    if(room1 == 0){
        sem_post(&mutex);
        sem_post(&t2);
    }

    else{
        sem_post(&mutex);
        sem_post(&t1);
    }

    sem_wait(&t2);
    room2--;
}


void morris_exit(void){
    if(room2 == 0){
        sem_post(&t1);
    }

    else{
        sem_post(&t2);
    }
}

//THREAD
void* worker(void* arg){
    long id = (long)arg;

    for (int i = 0; i < ITERATIONS; i++){
        morris_enter();

        shared_data++;
        printf("Thread %ld enters (iteration %d): %d | room1=%d room2=%d\n", id, i, shared_data, room1, room2);
        usleep(100000);

        morris_exit();
        usleep(100000);
    }

    return NULL;
}


int main(void) {
    pthread_t threads[NUM_THREADS];

    sem_init(&mutex, 0, 1);
    sem_init(&t1, 0, 1);
    sem_init(&t2, 0, 1);



    for (long i = 0; i < NUM_THREADS; i++)
        pthread_create(&threads[i], NULL, worker, (void*)i);

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);


    sem_destroy(&t2);
    sem_destroy(&t1);
    sem_destroy(&mutex);

    return 0;
}