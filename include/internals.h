#pragma once
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

//! CONSTS
#define INITIAL_HEAP_SIZE (4ULL*1024*1024)  // 4 mb
#define LARGE_THRESHOLD (1ULL*1024*1024)  // 1 mb
#define ALIGNMENT 16
#define ALIGN(size) (((size)+(ALIGNMENT-1)) & ~((size_t)(ALIGNMENT-1)))

#define NUM_SIZE_CLASSES 5
static const size_t SIZE_CLASSES[NUM_SIZE_CLASSES]={16,32, 64,128,256};
#define TCACHE_MAX_PER_CLASS 64
// Max from the global pool at once to thread 
#define TCACHE_REFILL_BATCH  8

//! Block Structures
typedef struct block_header{
    size_t size;
    int is_free;
} block_header_t;
typedef struct block_footer{
    size_t size;
    int is_free;
} block_footer_t;

typedef struct free_block{
    size_t size;
    int is_free;
    struct free_block* next;
    struct free_block* prev;
} free_block_t;

//! for heap.c
void heap_init(void);
void* heap_grow(size_t size);
void* heap_alloc_large(size_t size); // will use mmap hr
void heap_free_large(void* ptr, size_t size);
extern void* heap_start;
extern void* heap_end;

//! for freelist.c
void freelist_insert(free_block_t* block);
void freelist_remove(free_block_t* block);
free_block_t* freelist_search(size_t size);
free_block_t* freelist_split(free_block_t* block, size_t size);

void coalesce_next(free_block_t* block);
void coalesce_prev(free_block_t* block);
free_block_t* coalesce(free_block_t* block);

//! thread caching
void tcache_init(void);
void* tcache_get(int size_class);
void tcache_put(void* block, int size_class);
void tcache_refill(int size_class);
void tcache_flush(int size_class);
int tcache_size_class_for(size_t size); // -1 if no fit
