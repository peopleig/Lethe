#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "internals.h"

void* heap_start = NULL;
void* heap_end = NULL;

// cool stuff: Need to do this cuz printf itself uses malloc
// If I used printf for debugging, possible deadlock
static void debug_write(const char* msg) {
    ssize_t ret = write(2, msg, strlen(msg));
    (void)ret; // Cool stuff: Very common method to suppress unused var compiler warnings
}

// sbrk hr
void heap_init(void){
    if (heap_start != NULL) return;
    void* start = sbrk(INITIAL_HEAP_SIZE);
    if (start== (void*)(-1)) {
        debug_write("lethe: heap_init sbrk failed\n");
        return;
    }
    heap_start = start;
    heap_end = (char*)start +INITIAL_HEAP_SIZE;
    // entire slab is a free block
    free_block_t* block =(free_block_t*)heap_start;
    block->size = INITIAL_HEAP_SIZE - sizeof(block_header_t)- sizeof(block_footer_t);
    block->is_free = 1;
    block->next = NULL;
    block->prev = NULL;

    block_footer_t* footer = (block_footer_t*)((char*)heap_end- sizeof(block_footer_t));
    footer->size = block->size;
    footer->is_free = 1;
    freelist_insert(block);
}

void* heap_grow(size_t size){
    size_t grow_size = INITIAL_HEAP_SIZE;
    if(size > INITIAL_HEAP_SIZE)
        grow_size = size;

    void* old_end = sbrk(grow_size);
    if (old_end == (void*)(-1)) {
        debug_write("lethe: heap_grow sbrk failed\n");
        return NULL;
    }

    free_block_t* block = (free_block_t*)old_end;
    block->size = grow_size -sizeof(block_header_t) -sizeof(block_footer_t);
    block->is_free = 1;
    block->next = NULL;
    block->prev = NULL;
    heap_end = (char*)old_end + grow_size;
    block_footer_t* footer = (block_footer_t*)((char*)heap_end -sizeof(block_footer_t));
    footer->size = block->size;
    footer->is_free = 1;
    return block;
}

void* heap_alloc_large(size_t size){
    size_t total = size + sizeof(block_header_t)+ sizeof(block_footer_t);
    void* mem = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED){
        debug_write("lethe: mmap failed\n");
        return NULL;
    }
    block_header_t* header = (block_header_t*)mem;
    header->size = size;
    header->is_free = 0;
    block_footer_t* footer = (block_footer_t*)((char*)mem + sizeof(block_header_t) +size);
    footer->size = size;
    footer->is_free = 0;
    return (char*)mem + sizeof(block_header_t);
}

// Uses munmap (really cool stuff, didn't know abt it)
void heap_free_large(void* ptr,size_t size) {
    void* mem = (char*)ptr -sizeof(block_header_t);
    size_t total = size +sizeof(block_header_t)+ sizeof(block_footer_t);
    munmap(mem, total);
}