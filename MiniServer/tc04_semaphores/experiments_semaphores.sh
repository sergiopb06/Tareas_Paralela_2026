#!/bin/bash
cd "$(dirname "$0")/.."

for R in 5000 500 100; do #R = number of requests per thread
    for T in 1 4 10 50; do #T = number of threads
        for N in 1 2 4; do #N = number of cores
            case $N in
                1) CORES=0 ;;
                2) CORES=0,1 ;;
                4) CORES=0-3 ;;
            esac


            for K in 1 4 16 64; do #K = slots for counting semaphore
                
                cc -std=c11 -O2 -w -pthread -Iincludes \
                    -DMAX_ACTIVE_THREADS=$K \
                    -o bin/server_sem src/server_semaphores.c src/net_util.c

                total=0
                for rep in 1 2 3; do #run the same experiment 3 times and average the time.
                    taskset -c $CORES ./bin/server_sem 8080 > /dev/null &
                    pid=$!
                    sleep 0.5


                    t0=$(date +%s.%N)
                    taskset -c 4,5,10,11 ./bin/load_client 127.0.0.1 8080 $T $R
                    t1=$(date +%s.%N)


                    kill -INT $pid
                    wait $pid
                    sleep 1

                    elapsed=$(awk -v a="$t0" -v b="$t1" 'BEGIN{print b-a}')
                    total=$(awk -v t="$total" -v e="$elapsed" 'BEGIN{print t+e}')
                done

                avg=$(awk -v t="$total" 'BEGIN{print t/3}')
                tp=$(awk -v h="$T" -v r="$R" -v t="$avg" 'BEGIN{print h*r/t}') #how many request per second the server handles
                echo "R=$R T=$T N=$N K=$K  time=$avg throughput=$tp"
                
            done
        done
    done
done


#Para elaborar este script de experimentos se recibio apoyo de Claude Sonnet 5.5 (Anthropic, 2026), que proporciono la estructura general del script y los comandos utilizados.
