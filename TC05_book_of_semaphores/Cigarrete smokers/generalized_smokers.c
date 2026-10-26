#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define RUN_SECONDS 3

//AGENT SEMAPHORES
static sem_t tobacco;
static sem_t paper;
static sem_t match;

//PUSHER & SMOKER
static int num_tobacco = 0;
static int num_paper = 0;
static int num_match = 0;
static sem_t mutex;
static sem_t tobacco_sem;
static sem_t paper_sem;
static sem_t match_sem;



//AGENT A
void * agent_a(void* arg){
    (void)arg;

    while(true){

        printf("Agent A puts: tobacco and paper\n");
        sem_post(&tobacco);
        sem_post(&paper);
        usleep(100000);
    }

    return NULL;
}


//AGENT B
void * agent_b(void* arg){
    (void)arg;

    while(true){

        printf("Agent B puts: paper and match\n");
        sem_post(&paper);
        sem_post(&match);
        usleep(100000);
    }

    return NULL;
}


//AGENT C
void * agent_c(void* arg){
    (void)arg;

    while(true){

        printf("Agent C puts: tobacco and match\n");
        sem_post(&tobacco);
        sem_post(&match);
        usleep(100000);
    }

    return NULL;
}



//PUSHER A | TOBACCO
void* pusher_a(void* arg){
    (void)arg;

    while(true){
        sem_wait(&tobacco);
        sem_wait(&mutex);

        if(num_paper){
            num_paper--;
            sem_post(&match_sem);
        }

        else if(num_match){
            num_match--;
            sem_post(&paper_sem);
        }

        else{
            num_tobacco++;
            printf(" ++ Pusher A: tobacco on table = %d\n", num_tobacco);
        
        }

        sem_post(&mutex);
        
    }

    return NULL;
}




//PUSHER B | PAPER
void* pusher_b(void* arg){
    (void)arg;

    while(true){
        sem_wait(&paper);
        sem_wait(&mutex);

        if(num_tobacco){
            num_tobacco--;
            sem_post(&match_sem);
        }

        else if(num_match){
            num_match--;
            sem_post(&tobacco_sem);
        }

        else{
            num_paper++;
            printf(" ++ Pusher B: paper on table = %d\n", num_paper);
        }

        sem_post(&mutex);
        
    }

    return NULL;
}



//PUSHER C | MATCH
void* pusher_c(void* arg){
    (void)arg;

    while(true){
        sem_wait(&match);
        sem_wait(&mutex);

        if(num_tobacco){
            num_tobacco--;
            sem_post(&paper_sem);
        }

        else if(num_paper){
            num_paper--;
            sem_post(&tobacco_sem);
        }

        else{
            num_match++;
            printf(" ++ Pusher C: matches on table = %d\n", num_match);
        }

        sem_post(&mutex);
        
    }

    return NULL;
}


//SMOKER WITH TOBACCO
void* smoker_tobacco(void* arg){
    (void)arg;

    while (1){
        sem_wait(&tobacco_sem);

        printf("-- Smoker with TOBACCO makes a cigarette\n");
        usleep(100000); // makeCigarette()

        usleep(100000); // smoke()
    }

    return NULL;
}

//SMOKER WITH PAPER
void* smoker_paper(void* arg){
    (void)arg;

    while (1){
        sem_wait(&paper_sem);

        printf("-- Smoker with PAPER makes a cigarette\n");
        usleep(100000); // makeCigarette()

        usleep(100000); // smoke()
    }

    return NULL;
}

//SMOKER WITH MATCH
void* smoker_match(void* arg){
    (void)arg;

    while (1){
        sem_wait(&match_sem);

        printf("-- Smoker with MATCH makes a cigarette\n");
        usleep(100000); // makeCigarette()

        usleep(100000); // smoke()
    }

    return NULL;
} 



int main(void) {
    pthread_t agents[3];
    pthread_t pushers[3];
    pthread_t smokers[3];

    sem_init(&tobacco, 0, 0);
    sem_init(&paper, 0, 0);
    sem_init(&match, 0, 0);

    sem_init(&mutex, 0, 1);
    sem_init(&tobacco_sem, 0, 0);
    sem_init(&paper_sem, 0, 0);
    sem_init(&match_sem, 0, 0);

    pthread_create(&smokers[0], NULL, smoker_tobacco, NULL);
    pthread_create(&smokers[1], NULL, smoker_paper, NULL);
    pthread_create(&smokers[2], NULL, smoker_match, NULL);

    pthread_create(&pushers[0], NULL, pusher_a, NULL);
    pthread_create(&pushers[1], NULL, pusher_b, NULL);
    pthread_create(&pushers[2], NULL, pusher_c, NULL);

    pthread_create(&agents[0], NULL, agent_a, NULL);
    pthread_create(&agents[1], NULL, agent_b, NULL);
    pthread_create(&agents[2], NULL, agent_c, NULL);

    sleep(RUN_SECONDS);
    //In the book it says smokers loop forever, used while(true) for infinite loop. Program ends in run time RUN_SECONDS. After RUN_SECONDS it exits. 

    return 0;
}