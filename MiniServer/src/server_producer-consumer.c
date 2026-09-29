
#include "../includes/net_util.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define _POSIX_C_SOURCE 200809L

#ifndef NUM_CONSUMERS
#define NUM_CONSUMERS 4  //threads
#endif

#ifndef QUEUE_CAPACITY
#define QUEUE_CAPACITY 16 //buffer of connections
#endif

#define DEFAULT_PORT 8080
#define LISTEN_BACKLOG 64
#define DRAIN_SECONDS 1

static volatile sig_atomic_t g_running = 1;

static unsigned long g_requests_served = 0;

static pthread_mutex_t g_requests_served_mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct {
    int file_descriptor;
    unsigned long connection_id;
} connection_t;

//CONNECTION QUEUE (BUFFER)
typedef struct {
    connection_t *items[QUEUE_CAPACITY];
    int head; //reference for consumers
    int tail; //reference for producer
    int count; 
    int finish;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} connection_queue_t;


//INITIALIZE QUEUE
static connection_queue_t g_queue = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER
};


//PRODUCER PUSHES CONNECTIONS TO QUEUE
static void queue_push(connection_t *conn)
{
    pthread_mutex_lock(&g_queue.mutex);

    //while the queue is full, producer sleeps until a slot in queue is available, without wasting CPU, hence we use pthread_cond_wait(), release the mutex while sleeping
    while (g_queue.count == QUEUE_CAPACITY){
        pthread_cond_wait(&g_queue.not_full, &g_queue.mutex);
    }


    g_queue.items[g_queue.tail] = conn; 
    g_queue.tail = (g_queue.tail + 1) % QUEUE_CAPACITY;
    g_queue.count++;

    pthread_cond_signal(&g_queue.not_empty); //signal for consumers that the queue is NOT empty.
    pthread_mutex_unlock(&g_queue.mutex);
}



//CONSUMERS POP CONNECTIONS FROM QUEUE
static connection_t *queue_pop(void)
{
    pthread_mutex_lock(&g_queue.mutex);

    //while the queue is empty and is NOT finished, the consumer sleeps until there is something in the queue, release the mutex while sleeping
    while (g_queue.count == 0 && !g_queue.finish){
        pthread_cond_wait(&g_queue.not_empty, &g_queue.mutex);
    }

    //Reaching here with an empty queue means the queue is finished, no more connections allowed in the queue.
    //Without this check the consumer can read empty slots in queue.
    if (g_queue.count == 0){
        pthread_mutex_unlock(&g_queue.mutex);
        return NULL;
    }

    connection_t *conn = g_queue.items[g_queue.head];
    g_queue.head = (g_queue.head + 1) % QUEUE_CAPACITY;
    g_queue.count--;

    pthread_cond_signal(&g_queue.not_full);
    pthread_mutex_unlock(&g_queue.mutex);

    return conn;
}



static void on_sigint(int signum)
{
    (void)signum;
    g_running = 0;
}

static int install_signal_handlers(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;

    //sigaction(int signum, const struct sigaction *act struct sigaction *oldact)
    //SIGINT = the signal that Ctrl-C sends.
    if (sigaction(SIGINT, &sa, NULL) < 0)
    {
        perror("sigaction");
        return -1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;


    
    if (sigaction(SIGPIPE, &sa, NULL) < 0)
    {
        perror("sigaction");
        return -1;
    }

    return 0;
}

//Modified handle_connection(). Now instead of creating a thread-per-connection, it takes connections from the queue until it is finished.
static void *consumer_main(void *arg)
{

    (void)arg;
    connection_t *conn; 

    while((conn = queue_pop()) != NULL){

        printf("[Handling connection %lu] accepted\n", conn->connection_id);
        fflush(stdout);

        if (nu_drain_request(conn->file_descriptor) < 0)
        {
            (void)nu_send_response(conn->file_descriptor, conn->connection_id);
        }

        pthread_mutex_lock(&g_requests_served_mutex); //LOCK
        unsigned long current = g_requests_served; //Global shared read
        sched_yield();
        g_requests_served = current + 1; //Global share write

        //HERE IS RACE CONDITION
        //2 threads can read the same current, and modify it.
        //ADDED MUTEX TO STOP RACE CONDITION
        pthread_mutex_unlock(&g_requests_served_mutex); //UNLOCK

        if (close(conn->file_descriptor) < 0)
            perror("close(file_descriptor)");

        free(conn);
    }

    return NULL;

}

static unsigned short parse_port(int argc, char **argv)
{
    if (argc < 2)
    {
        return DEFAULT_PORT;
    }

    char *end = NULL;
    errno = 0;
    long value = strtol(argv[1], &end, 10);

    if (errno != 0 || end == argv[1] || *end != '\0' ||
        value <= 0 || value > 65535) {
        fprintf(stderr, "invalid port '%s', using %d\n", argv[1], DEFAULT_PORT);
        return DEFAULT_PORT;
        }

    return (unsigned short)value;
}




int main(int argc, char **argv)
{
    if (install_signal_handlers() < 0)
    {
        return EXIT_FAILURE;
    }
    unsigned short port = parse_port(argc, argv);

    int listen_file_descriptor = nu_listen(port, LISTEN_BACKLOG);

    if (listen_file_descriptor < 0)
    {
        return EXIT_FAILURE;
    }

    printf("listening on port %u — Ctrl-C to stop\n", port);
    fflush(stdout);



    pthread_t consumers[NUM_CONSUMERS];
    int started = 0;

    //Create Consumers
    for (int i = 0; i < NUM_CONSUMERS; i++){
        int pthread_created = pthread_create(&consumers[i], NULL, consumer_main, NULL);

        if (pthread_created != 0){
            fprintf(stderr, "pthread_create failed %s\n", strerror(pthread_created));
            break;
        }

        started++;
    }

    //Condition to check if any consumers were created. If no consumers were created (started == 0) it exits the program.
    if (started == 0){
        close(listen_file_descriptor);
        return EXIT_FAILURE;
    }



    unsigned long accepted = 0; 

    while (g_running)
    {                                 //accept(int socket, struct sockaddr *address, socklen_t *address_len)
        int client_file_descriptor = accept(listen_file_descriptor, NULL, NULL);
        if (client_file_descriptor < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("accept");
            break;
        }
        connection_t *conn = malloc(sizeof(connection_t));

        if (conn == NULL)
        {
            fprintf(stderr, "out of memory, dropping connection\n");
            close(client_file_descriptor);
            continue;
        }

        conn->file_descriptor = client_file_descriptor;
        conn->connection_id = ++accepted;


        queue_push(conn);

    }


    if (close(listen_file_descriptor))
    {
        perror("close(listen_file_descriptor)");
    }


    pthread_mutex_lock(&g_queue.mutex);
    g_queue.finish = 1;
    pthread_cond_broadcast(&g_queue.not_empty); //Tell the consumers no more connections will arrive in queue
    pthread_mutex_unlock(&g_queue.mutex);

    //Wait for consumers to empty the queue and exit 
    for (int i = 0; i < started; i++){
        int pthread_joined = pthread_join(consumers[i], NULL);
        if (pthread_joined != 0){
            fprintf(stderr, "pthread_join failed %s\n", strerror(pthread_joined));
        }
    }

    unsigned long final_count;
    pthread_mutex_lock(&g_requests_served_mutex);
    final_count = g_requests_served;
    pthread_mutex_unlock(&g_requests_served_mutex);


    printf("\naccepted: %lu\n", accepted);
    printf("served:   %lu\n", final_count);
    printf("lost:     %ld\n", (long)accepted - (long)final_count);

    return EXIT_SUCCESS;
}