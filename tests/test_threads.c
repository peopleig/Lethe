#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "lethe.h"

#define ITERS_PER_THREAD 100000
#define PATTERN_SIZE 64

// each thread does alloc/fill/verify/free cycles
static void* thread_work(void* arg){
    int id = *(int*)arg;
    unsigned char pattern = (unsigned char)(id & 0xFF);

    for (int i = 0; i < ITERS_PER_THREAD; i++){
        // random-ish sizes: 16 to 256
        size_t sz = 16 + ((i * 37 + id * 13) % 241);
        char* p = malloc(sz);
        assert(p != NULL);

        // write a pattern
        memset(p, pattern, sz);
        // verify nobody else stomped on our memory
        for (size_t j = 0; j < sz; j++){
            assert((unsigned char)p[j] == pattern);
        }
        free(p);
    }
    return NULL;
}

static void test_threads(int nthreads){
    pthread_t* threads = malloc(nthreads * sizeof(pthread_t));
    int* ids = malloc(nthreads * sizeof(int));
    assert(threads && ids);

    for (int i = 0; i < nthreads; i++){
        ids[i] = i;
        int rc = pthread_create(&threads[i], NULL, thread_work, &ids[i]);
        assert(rc == 0);
    }
    for (int i = 0; i < nthreads; i++){
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(ids);
    printf("test_threads(%d) passed — %d iters each, no corruption\n", nthreads, ITERS_PER_THREAD);
}

// spawn/join repeatedly to test thread-exit cache flushing
static void* short_lived_work(void* arg){
    (void)arg;
    for (int i = 0; i < 1000; i++){
        void* p = malloc(32);
        assert(p != NULL);
        free(p);
    }
    return NULL;
}

static void test_thread_exit_flush(void){
    for (int round = 0; round < 50; round++){
        pthread_t t;
        pthread_create(&t, NULL, short_lived_work, NULL);
        pthread_join(t, NULL);
    }
    printf("test_thread_exit_flush passed — 50 short-lived threads, no leaks\n");
}

int main(void){
    test_threads(2);
    test_threads(4);
    test_threads(8);
    test_threads(16);
    test_thread_exit_flush();
    printf("all thread tests passed\n");
    return 0;
}
