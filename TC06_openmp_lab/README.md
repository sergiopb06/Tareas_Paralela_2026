# Lab — Parallelising an image pipeline with OpenMP

**CI-0117 · Programación Paralela y Concurrente · ECCI, Universidad de Costa Rica**

## The scenario

A research group keeps a camera trained on a test rig and ends up with
thousands of 12-megapixel frames. For each frame they need an **edge map** and
a **histogram of edge intensities** — the histogram is what tells them whether
anything moved between two frames.

The pipeline they have works, and it is too slow. One frame takes about half a
second on one core, and they have thousands of frames. Your job is to
parallelise it with OpenMP without changing what it produces.

```
      load          stage 1        stage 2         stage 3        stage 4         save
    ┌────────┐   ┌───────────┐   ┌──────────┐   ┌──────────┐   ┌───────────┐   ┌────────┐
    │  PPM   │──▶│ grayscale │──▶│  5x5     │──▶│  Sobel   │──▶│ histogram │──▶│  PPM   │
    │  read  │   │           │   │  blur    │   │          │   │ 256 bins  │   │  write │
    └────────┘   └───────────┘   └──────────┘   └──────────┘   └───────────┘   └────────┘
      serial      per-pixel       neighbourhood  neighbourhood   shared bins      serial
```

Stages 1 to 4 are yours to parallelise. Load and save are strictly serial, and
that is what sets your Amdahl ceiling.

## Build and run

```sh
make            # builds everything into bin/
make image      # generates data/input.ppm, 4000 x 3000, deterministic
make run        # serial baseline, then your version
```

The image generator uses a fixed seed, so every machine produces the same
image and the same histogram. Two students can compare numbers directly.

To choose the thread count:

```sh
./bin/pipeline_omp data/input.ppm 4      # as an argument
OMP_NUM_THREADS=4 ./bin/pipeline_omp data/input.ppm
```

## What you have to do

Only `src/main_omp.c` is yours. Do not modify `filters.c`, `ppm.c` or
`main_serial.c` — they are the reference, and the grader uses them.

As handed to you the file compiles and runs, but every stage still calls the
serial version, so it is exactly as slow as the baseline. There are four TODOs.

| Task | Stage | What to use |
|---|---|---|
| 1 | grayscale | one `parallel for` |
| 2 | blur | `parallel for`, consider `collapse(2)` |
| 3 | Sobel | same shape as the blur |
| 4 | histogram | four versions: naive, `critical`, `atomic`, `reduction` |

Task 4 is the heart of the lab. Implement **all four** and run them, because
the comparison is the point — not any single version.

## Correctness comes first

A fast wrong answer is worth nothing. Your version must produce an edge map
identical to the serial one, byte for byte:

```sh
./bin/pipeline_serial data/input.ppm
./bin/pipeline_omp    data/input.ppm 4
cmp data/edges_serial.ppm data/edges_omp.ppm   # must print nothing
```

The program also checks the histogram for you, against the serial reference,
and prints how many of the 256 bins came out wrong.

## What to measure

Fill in this table in your report, one row per thread count, on **your** machine:

- Machine: 

- **CPU:** AMD Ryzen 5 3600 6-Core Processor
- **Cores / threads:** 6 physical cores, 2 threads per core, 12 logical CPUs
- **Max frequency:** 4208 MHz
- **Cache:** L2 3 MiB (6 instances), L3 32 MiB (2 instances)
- **RAM:** 15 GiB
- **OS:** Ubuntu 26.04.1 LTS, kernel 7.0.0-38-generic
- **Compiler:** gcc 15.2.0 (`-O2 -fopenmp`)

- Results come from running: 
   - python 3 bench_tables.py

| threads | gray | blur | sobel | hist (reduction) | total | speedup |
|---|---|---|---|---|---|---|
| 1 | 0.020 | 0.194 | 0.045 | 0.005 | 0.321 | 1.00x |
| 2 | 0.010 | 0.099 | 0.025 | 0.003 | 0.194 | 1.65x |
| 4 | 0.005 | 0.051 | 0.015 | 0.001 | 0.232 | 1.38x |
| 8 | 0.004 | 0.042 | 0.015 | 0.001 | 0.223 | 1.44x |

And this one, for stage 4 only:

| threads | naive | critical | atomic | reduction |
|---|---|---|---|---|
| 1 | 0.005 | 0.104 | 0.051 | 0.005 |
| 4 | 0.088 | 0.553 | 0.139 | 0.001 |

## Questions to answer

1. **Run the naive histogram with one thread, then with four.** With one
   thread it is correct. With four it loses counts, and loses a *different*
   number on every run. Explain both observations.


   - With the naive version there is no protection, multiple threads can write and read the same value. Here goes a race condition. It works with one thread because there is no competition, with more than one thread they fight over the data. The number of lost count is different on every run because of non-determinism. The operating system decides the order of each thread on every run. This causes every run to have different results.


2. **Where is the barrier?** The blur reads a neighbourhood of `gray`, so
   stage 1 has to be completely finished before any thread starts stage 2.
   You should not have needed to write `#pragma omp barrier` anywhere.
   What guarantees the ordering?


   - The parallel for in OMP has an implicit barrier at the end. When a thread finishes its iterations it waits there until all toher threads of the team finish theirs. Because of that, grayscale_omp cannot return until all of gray is written, the does blur_omp start, so the blur always sees the complete gray.

3. **`critical` against `atomic`.** Both give the right answer. Measure both
   and explain the gap. What does `critical` do that `atomic` does not?


   - Critical serializes a whole block of code. Atomic serializes one operation. Critical is a lock, like a mutex, all threads that reach it have to take the same lock. The have to release, sleep and wake up. Atomic protects only a single instruction, there is no lock. Measured: with 1 thread critical took 0.104 s and atomic 0.051 s (2.0x slower); with 4 threads critical took 0.553 s and atomic 0.139 s (4.0x slower).

4. **`critical` as you add threads.** Time it with 1, 2 and 4 threads. It does
   not get faster. Explain why, and say what that implies about putting a
   `critical` inside a hot loop.


   - It doesn't get faster because critical works as a mutex, every thread has to enter and leave the critical section, only one can be inside at a time. The loop is the critical section, so threads end up running one after another, adding more threads only means a longer line to get the lock. Measured: 0.104 s with 1 thread, 0.375 s with 2, 0.553 s with 4 and 0.865 s with 8, so it got slower, not faster That menas that a critical section inside a hot loop removes parallelism, it serializes the whole for loop.

5. **Amdahl.** The program prints the measured `P` and the ceiling. Using the
   numbers from *your* run: what is the most threads worth buying for this
   pipeline, and which change would raise the ceiling more — more cores, or
   making the save faster?


   - Measured with 1 thread: P = 0.823, so the Amdahl ceiling is 1/(1−P) = 1/0.177 = 5.65x. 
   - Using S(n) = 1/((1−P)+P/n), Amdahl predicts:
      - 1.70x for 2 threads.
      - 2.61x for 4 threads.
      - 3.18x for 6 threads.
      - 3.57x for 8 threads.
      - 4.07x for 12 threads.
      - 4.38x for 16 threads. 
   - The speedup keeps growing, but each extra thread adds less than the previous one. The most worth buying is about 6 threads, which are the 6 physical cores of this machine.

   - Making the save faster would raise the ceiling more than adding cores. More cores cannot pass the ceiling, because 1/(1−P) does not depend on the number of threads. The ceiling only goes up if the serial part (load + save = 0.016 + 0.040 = 0.056 s) gets smaller. 

6. **Granularity.** Regenerate the image at 400 x 300 with
   `./bin/gen_image data/small.ppm 400 300` and run both versions on it.
   Does OpenMP still win? Relate your answer to where the crossover point is.


   - Yes, but not for much. On the 400 x 300 image the median total was 0.003 s for the serial version and 0.003, 0.002, 0.001 and 0.001 s for OpenMP with 1, 2, 4 and 8 threads, so the gain is about 1 to 2 ms. The whole run is so short that the cost of creating and synchronizing the threads is close to the work they save: 8 threads were no better than 4, and critical went from 0.001 s with 1 thread to 0.009 s with 4. 


## Check map matches
   - With:
      - ./bin/pipeline_serial data/input.ppm
      - ./bin/pipeline_omp data/input.ppm 4
      - cmp data/edges_serial.ppm data/edges_omp.ppm
   - The exit code for cmp was 0. The edge map for the parallel task was identical to the serial one.

## What to hand in

- `src/main_omp.c` with the four tasks done
- The two measurement tables, filled in, saying which machine and how many cores
- Written answers to the six questions, one short paragraph each
- The output of `cmp`, showing your edge map matches

## Layout

```
openmp-lab/
├── Makefile
├── README.md
├── include/
│   ├── ppm.h          PPM reader and writer interface
│   └── filters.h      the four stages
├── src/
│   ├── ppm.c          do not modify
│   ├── filters.c      serial reference, do not modify
│   ├── gen_image.c    deterministic test image
│   ├── main_serial.c  baseline, do not modify
│   └── main_omp.c     YOUR FILE
├── solution/          reference solution (your instructor may withhold this)
├── build/             objects, generated
├── bin/               executables, generated
└── data/              images and histograms, generated
```

## Notes

- `-fopenmp` goes on **both** the compile and the link step. Leave it off and
  the pragmas are silently ignored: the program still runs, still gives the
  right answer, and is quietly sequential.
- Time with `omp_get_wtime()`, never `clock()`. `clock()` adds up CPU time
  across all threads, so a parallel program appears to take *longer* than the
  serial one even when it finished sooner.
- Measure with the `printf` calls commented out. Writing to the terminal is
  serialised by the operating system and will dominate everything else.
- Run each configuration several times and take the median. One run of a
  parallel program measures almost nothing.
