#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// bench links against either lethe or glibc — same source, different binary

#define ITERS 1000000
#define NUM_SIZES 7
static const size_t SIZES[NUM_SIZES] = {16, 32, 64, 128, 256, 512, 1024};

static double now_ns(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

//! Single-threaded: ns per malloc+free pair for each size
static void bench_single(void){
    printf("\n--- Single-threaded malloc/free ---\n");
    printf("%-10s %12s\n", "Size", "ns/op");
    printf("---------- ------------\n");

    for (int s = 0; s < NUM_SIZES; s++){
        size_t sz = SIZES[s];
        double start = now_ns();
        for (int i = 0; i < ITERS; i++){
            void* p = malloc(sz);
            *(volatile char*)p = 1;  // prevent optimizer from removing
            free(p);
        }
        double elapsed = now_ns() - start;
        printf("%-10zu %12.1f\n", sz, elapsed / ITERS);
    }
}

//! Multi-threaded: total ops/sec across all threads
typedef struct {
    int thread_count;
    long long total_ops;
} mt_result_t;

static void* mt_worker(void* arg){
    (void)arg;
    for (int i = 0; i < ITERS; i++){
        size_t sz = SIZES[i % NUM_SIZES];
        void* p = malloc(sz);
        *(volatile char*)p = 1;
        free(p);
    }
    return NULL;
}

static void bench_multi(int nthreads){
    pthread_t threads[64];

    double start = now_ns();
    for (int i = 0; i < nthreads; i++){
        pthread_create(&threads[i], NULL, mt_worker, NULL);
    }
    for (int i = 0; i < nthreads; i++){
        pthread_join(threads[i], NULL);
    }
    double elapsed = now_ns() - start;

    long long total_ops = (long long)nthreads * ITERS;
    double ops_per_sec = total_ops / (elapsed / 1e9);
    printf("%-10d %12lld %15.0f %12.1f\n", nthreads, total_ops, ops_per_sec, elapsed / 1e6);
}

static void bench_multi_all(void){
    int counts[] = {1, 2, 4, 8, 16};
    int n = sizeof(counts) / sizeof(counts[0]);

    printf("\n--- Multi-threaded scaling ---\n");
    printf("%-10s %12s %15s %12s\n", "Threads", "Total ops", "ops/sec", "time(ms)");
    printf("---------- ------------ --------------- ------------\n");

    for (int i = 0; i < n; i++){
        bench_multi(counts[i]);
    }
}

int main(void){
    printf("=== Lethe Allocator Benchmark ===\n");
    bench_single();
    bench_multi_all();
    printf("\ndone.\n");
    return 0;
}
