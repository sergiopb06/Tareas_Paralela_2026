#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_CUSTOMERS 25
#define NUM_BARBERS 3 
#define N 20
#define SOFA_SEATS 4


//BARBERSHOP
static int customers = 0;       
static bool shop_closed = false;
static sem_t mutex;
static sem_t sofa;
static sem_t customer1;        
static sem_t customer2;        
static sem_t barber;        
static sem_t seated;        
static sem_t payment;        
static sem_t receipt;        
static sem_t cash_register;        
static long current_payer = -1;


static sem_t* queue1[N];
static int head1 = 0;
static int tail1 = 0;

static sem_t* queue2[N];
static int head2 = 0;
static int tail2 = 0;


void balk(long id){
    printf("Customer %ld leaves (balk)\n", id);
}

void sit_on_sofa(long id){
    printf("  Customer %ld sits on the sofa\n", id);
}
void get_hair_cut(long id){
    printf("  Customer %ld is getting a haircut\n", id);
    usleep(200000);
}

void pay(long id){
    printf("      Customer %ld pays\n", id);
    usleep(200000);
}

void cut_hair(long id){
    printf("-- Barber is cutting hair\n");
    usleep(200000);
}

void accept_payment(long id){
    printf("-- Barber %ld accepts payment from customer %ld\n", id, current_payer);
    usleep(200000);

}


//CUSTOMER
void* customer_thread(void* arg){
    long id = (long)arg;

    sem_t self_sem1;
    sem_t self_sem2;

    sem_init(&self_sem1, 0, 0);
    sem_init(&self_sem2, 0, 0);

    usleep(30000);     

    sem_wait(&mutex);
    if (customers == N){
        sem_post(&mutex);  
        sem_post(&self_sem2);
        sem_post(&self_sem1);
        balk(id);
        return NULL;
    }

    customers++;
    queue1[tail1] = &self_sem1;
    tail1 = (tail1 + 1) % N;
    printf("Customer %ld enters (customers in shop: %d)\n", id, customers);
    sem_post(&mutex);

    //enterShop()
    sem_post(&customer1);    
    sem_wait(&self_sem1);      

    sem_wait(&sofa);
    sit_on_sofa(id);
    sem_post(&seated);

    sem_wait(&mutex);
    queue2[tail2] = &self_sem2;
    tail2 = (tail2 + 1) % N;
    printf("  Customer %ld waits for a chair\n", id);
    sem_post(&mutex);

    sem_post(&customer2);
    sem_wait(&self_sem2);
    sem_post(&sofa);

    sem_wait(&barber);
    get_hair_cut(id);

    sem_wait(&cash_register);
    current_payer = id;
    pay(id);
    sem_post(&payment);
    sem_wait(&receipt);
    sem_post(&cash_register);

    sem_wait(&mutex);
    customers--;
    sem_post(&mutex);
    printf("Customer %ld leaves\n", id);

    sem_destroy(&self_sem1);
    sem_destroy(&self_sem2);


    return NULL;
}

//BARBER
void* barber_thread(void* arg){
    long id = (long)arg;

    while (true){
        sem_wait(&customer1);      
        if (shop_closed){           
            break;
        }

        sem_wait(&mutex);
        sem_t* sem = queue1[head1];
        head1 = (head1 + 1) % N;
        sem_post(sem);
        sem_post(&seated);
        sem_post(&mutex);

        sem_wait(&customer2);
        sem_wait(&mutex);
        sem = queue2[head2];
        head2 = (head2 + 1) % N;
        sem_post(&mutex);
        sem_post(sem);    

        sem_post(&barber);
        cut_hair(id);

        sem_wait(&payment);   
        accept_payment(id);
        sem_post(&receipt);   
    }

    return NULL;
}


int main(void) {
    pthread_t barbers_t[NUM_BARBERS];
    pthread_t customers_t[NUM_CUSTOMERS];

    sem_init(&mutex, 0, 1);
    sem_init(&sofa, 0, SOFA_SEATS);
    sem_init(&customer1, 0, 0);
    sem_init(&customer2, 0, 0);
    sem_init(&barber, 0, 0);
    sem_init(&seated, 0, 0);
    sem_init(&payment, 0, 0);
    sem_init(&receipt, 0, 0);
    sem_init(&cash_register, 0, 1);

    for (long i = 0; i < NUM_BARBERS; i++)
        pthread_create(&barbers_t[i], NULL, barber_thread, (void*)i);

    for (long i = 0; i < NUM_CUSTOMERS; i++)
        pthread_create(&customers_t[i], NULL, customer_thread, (void*)i);

    for (int i = 0; i < NUM_CUSTOMERS; i++)
        pthread_join(customers_t[i], NULL);

    //All customers done or left, shop closes
    shop_closed = true;
     for (int i = 0; i < NUM_BARBERS; i++)
        sem_post(&customer1);      

    for (int i = 0; i < NUM_BARBERS; i++)
        pthread_join(barbers_t[i], NULL);

    sem_destroy(&cash_register);
    sem_destroy(&receipt);
    sem_destroy(&payment);
    sem_destroy(&seated);
    sem_destroy(&barber);
    sem_destroy(&customer2);
    sem_destroy(&customer1);
    sem_destroy(&sofa);
    sem_destroy(&mutex);
    return 0;
}