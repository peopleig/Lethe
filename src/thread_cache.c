#include <string.h>
#include <pthread.h>
#include "internals.h"

//! Thread-local cache — small allocs avoid the global lock entirely

static pthread_key_t tcache_key;
static pthread_once_t tcache_once = PTHREAD_ONCE_INIT;

// per-thread cache: array of pointers per size class
typedef struct {
    void* blocks[TCACHE_MAX_PER_CLASS];
    int count;
} tcache_bin_t;

static __thread tcache_bin_t local_cache[NUM_SIZE_CLASSES];
static __thread int tcache_armed = 0;

// global pool — fallback when local cache is empty
static struct {
    void* blocks[TCACHE_MAX_PER_CLASS * 4];
    int count;
} global_pool[NUM_SIZE_CLASSES];

// flush everything back to global pool on thread exit
static void tcache_destructor(void* unused){
    (void)unused;
    for (int i = 0; i < NUM_SIZE_CLASSES; i++){
        tcache_flush(i);
    }
}

static void tcache_key_init(void){
    pthread_key_create(&tcache_key, tcache_destructor);
}

void tcache_init(void){
    pthread_once(&tcache_once, tcache_key_init);
}

// arm the destructor for this thread on first use
static void tcache_arm(void){
    if (!tcache_armed){
        tcache_armed = 1;
        pthread_setspecific(tcache_key, (void*)1);
    }
}

// which size class does this size fit into? -1 if none
int tcache_size_class_for(size_t size){
    for (int i = 0; i < NUM_SIZE_CLASSES; i++){
        if (size <= SIZE_CLASSES[i]) return i;
    }
    return -1;
}

// pop a block from the local cache (no lock)
void* tcache_get(int sc){
    tcache_arm();
    if (local_cache[sc].count > 0){
        local_cache[sc].count--;
        return local_cache[sc].blocks[local_cache[sc].count];
    }
    // miss — refill from global pool
    tcache_refill(sc);
    if (local_cache[sc].count > 0){
        local_cache[sc].count--;
        return local_cache[sc].blocks[local_cache[sc].count];
    }
    return NULL;
}

// push a block to the local cache (no lock)
void tcache_put(void* block, int sc){
    tcache_arm();
    if (local_cache[sc].count < TCACHE_MAX_PER_CLASS){
        local_cache[sc].blocks[local_cache[sc].count] = block;
        local_cache[sc].count++;
        return;
    }
    // local cache full — push to global pool under lock
    pthread_mutex_lock(&global_lock);
    if (global_pool[sc].count < TCACHE_MAX_PER_CLASS * 4){
        global_pool[sc].blocks[global_pool[sc].count] = block;
        global_pool[sc].count++;
    }
    pthread_mutex_unlock(&global_lock);
}

// batch refill local cache from global pool
void tcache_refill(int sc){
    pthread_mutex_lock(&global_lock);
    int batch = TCACHE_REFILL_BATCH;
    while (batch > 0 && global_pool[sc].count > 0 && local_cache[sc].count < TCACHE_MAX_PER_CLASS){
        global_pool[sc].count--;
        local_cache[sc].blocks[local_cache[sc].count] = global_pool[sc].blocks[global_pool[sc].count];
        local_cache[sc].count++;
        batch--;
    }
    pthread_mutex_unlock(&global_lock);
}

// flush local cache for a size class back to global pool
void tcache_flush(int sc){
    pthread_mutex_lock(&global_lock);
    while (local_cache[sc].count > 0){
        local_cache[sc].count--;
        if (global_pool[sc].count < TCACHE_MAX_PER_CLASS * 4){
            global_pool[sc].blocks[global_pool[sc].count] = local_cache[sc].blocks[local_cache[sc].count];
            global_pool[sc].count++;
        }
    }
    pthread_mutex_unlock(&global_lock);
}
