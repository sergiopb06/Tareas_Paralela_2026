## Mini Server

Instructions to set up a mini server:

- Install all the required dependencies

Running the project:

- Run make

Run the server

- Run the server using the command: ./bin/server_unsafe 8080
- Args:
    - The first argument is the port. You can select another port number if the port is already in use. The server will start listening on the specified port.

Run the client

- Run the client using the command: ./bin/load_client 127.0.0.1 8080 8 150
- Args:
- The first argument is the server IP address. You can select another IP address if the server is running on a different machine.
- The second argument is the server port number where the server is listening.
- The third argument is the number of threads.
- The fourth argument is the number of requests per thread. The client will send a total of (number of threads * number of requests per thread) requests to the server.


## TC03 Producer-consumer
Run the experiments:
- Give execution permission: 
    - chmod +x experiments_producer-consumer.sh
- Run the script and save the output: 
    - ./experiments_producer-consumer.sh | tee results_producer-consumer.txt
- Parameters: 
    - R: number of requests per thread (5000, 500).
    - T: number of threads (1, 4, 10, 50).
    - N: number of cores for the server (1, 2, 4). 
    - C: number of consumers (1, 4, 20).
    - Q: capacity of the queue (1, 16, 100).
- Each combination is run 3 times and the time is averaged.
- Throughput = (T * R / time)
- The raw data is in "results_producer-consumer.xlxs" (sheet "in").

## TC04 Semaphores
Run the experiments:
- Give execution permission: 
    - chmod +x experiments_semaphores.sh
- Run the script and save the output: 
    - ./experiments_semaphores.sh | tee results_semaphores.txt
- Parameters: 
    - R: number of requests per thread (5000, 500).
    - T: number of threads (1, 4, 10, 50).
    - N: number of cores for the server (1, 2, 4). 
    - K: counting semaphore initial value [max active threads] (...).
- Each combination is run 3 times and the time is averaged.
- Throughput = (T * R / time)
- The raw data is in "results_semaphores.xlxs" (sheet "in").

