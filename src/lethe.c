#include <string.h>
#include <stdint.h>
#include "lethe.h"
#include "internals.h"

static int initialized = 0;
static void ensure_init(void){
    if (!initialized){
        pthread_mutex_lock(&global_lock);
        if (!initialized){
            heap_init();
            tcache_init();
            initialized = 1;
        }
        pthread_mutex_unlock(&global_lock);
    }
}

void* malloc(size_t size){
    if(size==0)
        return NULL;
    ensure_init();
    size_t aligned = ALIGN(size);
    int sc = tcache_size_class_for(aligned);
    size_t alloc_size = (sc >= 0) ? SIZE_CLASSES[sc] : aligned;

    // large allocs bypass everything, go straight to mmap
    if (alloc_size >= LARGE_THRESHOLD){
        return heap_alloc_large(alloc_size);
    }

    // try thread-local cache first (no lock)
    if (sc >= 0){
        void* cached = tcache_get(sc);
        if (cached) return cached;
    }

    // cache miss — go to central freelist under lock
    pthread_mutex_lock(&global_lock);
    free_block_t* block = freelist_search(alloc_size);
    if (block == NULL){
        block = heap_grow(alloc_size);
        if (block == NULL){
            pthread_mutex_unlock(&global_lock);
            return NULL;
        }
        freelist_insert(block);
    }
    block = freelist_split(block, alloc_size);
    block_header_t* header = (block_header_t*)block;
    header->is_free = 0;

    // write footer for the allocated block
    block_footer_t* footer = (block_footer_t*)((char*)header + sizeof(block_header_t) + header->size);
    footer->size = header->size;
    footer->is_free = 0;
    pthread_mutex_unlock(&global_lock);
    return (char*)header + sizeof(block_header_t);
}

void free(void* ptr){
    if(ptr ==NULL) return;
    block_header_t* header = (block_header_t*)((char*)ptr -sizeof(block_header_t));
    // large blocks go straight to munmap
    if(header->size >=LARGE_THRESHOLD){
        heap_free_large(ptr, header->size);
        return;
    }
    // try to return small blocks to thread-local cache (no lock)
    int sc = tcache_size_class_for(header->size);
    if (sc >= 0 && header->size == SIZE_CLASSES[sc]){
        tcache_put(ptr, sc);
        return;
    }
    // larger heap blocks: coalesce and return to central freelist. unez
    pthread_mutex_lock(&global_lock);
    free_block_t* block = (free_block_t*)header;
    block->size = header->size;
    freelist_insert(block);
    coalesce(block);
    pthread_mutex_unlock(&global_lock);
}

void* realloc(void* ptr, size_t new_size){
    // realloc(NULL, n) == malloc(n)
    if(ptr==NULL) 
        return malloc(new_size);
    // realloc(ptr, 0) == free(ptr)
    if(new_size == 0){
        free(ptr);
        return NULL;
    }
    block_header_t* header =(block_header_t*)((char*)ptr -sizeof(block_header_t));
    size_t old_size = header->size;
    size_t aligned = ALIGN(new_size);

    // shrinking or same, just return
    if (aligned <=old_size)
        return ptr;
    // trying inplace expansion if next block in memory is big enuff
    if (old_size < LARGE_THRESHOLD){
        pthread_mutex_lock(&global_lock);
        block_header_t* next = (block_header_t*)((char*)header +sizeof(block_header_t) +old_size +sizeof(block_footer_t));
        if((void*)next<heap_end && next->is_free){
            size_t combined = old_size +BLOCK_OVERHEAD +next->size;
            if(combined>=aligned){
                // absorb next block
                free_block_t* next_free =(free_block_t*)next;
                freelist_remove(next_free);
                header->size = combined;
                header->is_free = 0;
                block_footer_t* footer = (block_footer_t*)((char*)header +sizeof(block_header_t) +combined);
                footer->size = combined;
                footer->is_free = 0;
                pthread_mutex_unlock(&global_lock);
                return ptr;
            }
        }
        pthread_mutex_unlock(&global_lock);
    }

    // fallback: malloc + copy + free
    void* new_ptr = malloc(aligned);
    if(new_ptr==NULL)
        return NULL;
    memcpy(new_ptr, ptr, old_size<new_size ?old_size :new_size);
    free(ptr);
    return new_ptr;
}

void* calloc(size_t nmemb, size_t size){
    // overflow check
    if(nmemb!=0 && size>(SIZE_MAX/nmemb))
        return NULL;
    size_t total = nmemb*size;
    void* ptr = malloc(total);
    if(ptr)
        memset(ptr, 0, total);
    return ptr;
}