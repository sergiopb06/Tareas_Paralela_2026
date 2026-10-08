#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_CUSTOMERS 10
#define N 4 


//BARBERSHOP
static int customers = 0;       
static bool shop_closed = false;
static sem_t mutex;
static sem_t customer;        
static sem_t customer_done;     
static sem_t barber_done;
static sem_t* queue[N];
static int head = 0;
static int tail = 0;



void balk(long id){
    printf("Customer %ld leaves (balk)\n", id);
}

void get_hair_cut(long id){
    printf("  Customer %ld is getting a haircut\n", id);
    usleep(200000);
}

void cut_hair(void){
    printf("-- Barber is cutting hair\n");
    usleep(200000);
}


//CUSTOMER
void* customer_thread(void* arg){
    long id = (long)arg;

    sem_t self_sem;
    sem_init(&self_sem, 0, 0);
    usleep(30000);     

    sem_wait(&mutex);
    if (customers == N){
        sem_post(&mutex);  
        sem_post(&self_sem);
        balk(id);
        return NULL;
    }

    customers++;
    queue[tail] = &self_sem;
    tail = (tail + 1) % N;
    printf("Customer %ld enters (customers in shop: %d)\n", id, customers);
    sem_post(&mutex);

    sem_post(&customer);    
    sem_wait(&self_sem);      

    get_hair_cut(id);

    sem_post(&customer_done);   
    sem_wait(&barber_done);    

    sem_wait(&mutex);
    customers--;
    sem_post(&mutex);
    printf("Customer %ld leaves\n", id);

    sem_destroy(&self_sem);

    return NULL;
}

//BARBER
void* barber_thread(void* arg){
    (void)arg;

    while (true){
        sem_wait(&customer);      
        if (shop_closed){           
            break;
        }

        sem_wait(&mutex);
        sem_t* sem = queue[head];
        head = (head + 1) % N;
        sem_post(&mutex);

        sem_post(sem);

        cut_hair();

        sem_wait(&customer_done);   
        sem_post(&barber_done);   
    }

    return NULL;
}


int main(void) {
    pthread_t barber_t;
    pthread_t customers_t[NUM_CUSTOMERS];

    sem_init(&mutex, 0, 1);
    sem_init(&customer, 0, 0);
    sem_init(&customer_done, 0, 0);
    sem_init(&barber_done, 0, 0);

    pthread_create(&barber_t, NULL, barber_thread, NULL);

    for (long i = 0; i < NUM_CUSTOMERS; i++)
        pthread_create(&customers_t[i], NULL, customer_thread, (void*)i);

    for (int i = 0; i < NUM_CUSTOMERS; i++)
        pthread_join(customers_t[i], NULL);

    //All customers done or left, shop closes
    shop_closed = true;
    sem_post(&customer);           
    pthread_join(barber_t, NULL);   

    sem_destroy(&barber_done);
    sem_destroy(&customer_done);
    sem_destroy(&customer);
    sem_destroy(&mutex);
    return 0;
}